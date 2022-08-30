/*******************************************************************************

Filename    :   Main.cpp
Content     :   Simple test app to test openxr hands
Created     :   Sept 2020
Authors     :   Federico Schliemann
Language    :   C++
Copyright:  Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*******************************************************************************/

#include <cstdint>
#include <cstdio>

#include "XrApp.h"

#include "Input/SkeletonRenderer.h"
#include "Input/ControllerRenderer.h"
#include "Input/TinyUI.h"
#include "Input/AxisRenderer.h"
#include "Input/HandRenderer.h"
#include "Render/SimpleBeamRenderer.h"
#include "Render/GeometryRenderer.h"

#define FORCE_ONLY_SIMPLE_CONTROLLER_PROFILE

// Hands
#ifndef XR_FB_hand_tracking_mesh
#include <openxr/fb_hand_tracking_mesh.h>
#endif
#ifndef XR_FB_hand_tracking_capsules
#include <openxr/fb_hand_tracking_capsules.h>
#endif
#ifndef XR_FB_hand_tracking_aim
#include <openxr/fb_hand_tracking_pointer.h>
#endif

class XrHandsApp : public OVRFW::XrApp {
   public:
    XrHandsApp() : OVRFW::XrApp() {
        BackgroundColor = OVR::Vector4f(0.60f, 0.95f, 0.4f, 1.0f);
    }

    // Returns a list of OpenXr extensions needed for this app
    virtual std::vector<const char*> GetExtensions() override {
        std::vector<const char*> extensions = XrApp::GetExtensions();
        extensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
        extensions.push_back(XR_FB_HAND_TRACKING_MESH_EXTENSION_NAME);
        extensions.push_back(XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
        extensions.push_back(XR_FB_HAND_TRACKING_CAPSULES_EXTENSION_NAME);
        return extensions;
    }

#ifdef FORCE_ONLY_SIMPLE_CONTROLLER_PROFILE
    // Returns a map from interaction profile paths to vectors of suggested bindings.
    // xrSuggestInteractionProfileBindings() is called once for each interaction profile path in the
    // returned map.
    // Apps are encouraged to suggest bindings for every device/interaction profile they support.
    // Override this for custom action bindings, or modify the default bindings.
    std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> GetSuggestedBindings(
        XrInstance instance) override {
        // Get base suggested bindings
        std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> allSuggestedBindings =
            XrApp::GetSuggestedBindings(instance);

        std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>>
            onlySimpleSuggestedBindings{};

        XrPath simpleInteractionProfile = XR_NULL_PATH;
        OXR(xrStringToPath(
            instance, "/interaction_profiles/khr/simple_controller", &simpleInteractionProfile));

        // Only copy over suggested bindings for the simple interaction profile
        onlySimpleSuggestedBindings[simpleInteractionProfile] =
            allSuggestedBindings[simpleInteractionProfile];

        return onlySimpleSuggestedBindings;
    }
#endif

    // Must return true if the application initializes successfully.
    virtual bool AppInit(const xrJava* context) override {
        if (false == ui_.Init(context, GetFileSys())) {
            ALOG("TinyUI::Init FAILED.");
            return false;
        }
        /// Build UI
        ui_.AddLabel(
            "OpenXR Hands + FB extensions Sample", {0.1f, 1.25f, -2.0f}, {1300.0f, 100.0f});
        ui_.AddButton("Red", {0.0f, 1.75f, -2.0f}, {200.0f, 100.0f}, [=]() {
            BackgroundColor = OVR::Vector4f(0.8f, 0.05f, 0.05f, 1.0f);
        });
        ui_.AddButton("Green", {0.0f, 2.0f, -2.0f}, {200.0f, 100.0f}, [=]() {
            BackgroundColor = OVR::Vector4f(0.0f, 0.65f, 0.1f, 1.0f);
        });
        ui_.AddButton("Blue", {0.0f, 2.25f, -2.0f}, {200.0f, 100.0f}, [=]() {
            BackgroundColor = OVR::Vector4f(0.0f, 0.25f, 1.0f, 1.0f);
        });

        ui_.AddButton("Mesh L", {-1.0f, 1.75f, -2.0f}, {200.0f, 100.0f}, [=]() {
            renderMeshL_ = !renderMeshL_;
        });
        ui_.AddButton("Joints L", {-1.0f, 2.0f, -2.0f}, {200.0f, 100.0f}, [=]() {
            renderJointsL_ = !renderJointsL_;
        });
        ui_.AddButton("Capsules L", {-1.0f, 2.25f, -2.0f}, {200.0f, 100.0f}, [=]() {
            renderCapsulesL_ = !renderCapsulesL_;
        });

        ui_.AddButton("Mesh R", {1.20f, 1.75f, -2.0f}, {200.0f, 100.0f}, [=]() {
            renderMeshR_ = !renderMeshR_;
        });
        ui_.AddButton("Joints R", {1.20f, 2.0f, -2.0f}, {200.0f, 100.0f}, [=]() {
            renderJointsR_ = !renderJointsR_;
        });
        ui_.AddButton("Capsules R", {1.20f, 2.25f, -2.0f}, {200.0f, 100.0f}, [=]() {
            renderCapsulesR_ = !renderCapsulesR_;
        });

        // Inspect hand tracking system properties
        XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties{
            XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
        XrSystemProperties systemProperties{
            XR_TYPE_SYSTEM_PROPERTIES, &handTrackingSystemProperties};
        OXR(xrGetSystemProperties(GetInstance(), GetSystemId(), &systemProperties));
        if (!handTrackingSystemProperties.supportsHandTracking) {
            // The system does not support hand tracking
            ALOG("xrGetSystemProperties XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT FAILED.");
            return false;
        } else {
            ALOG(
                "xrGetSystemProperties XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT OK - initiallizing hand tracking...");
        }

        /// Hook up extensions for hand tracking
        OXR(xrGetInstanceProcAddr(
            GetInstance(),
            "xrCreateHandTrackerEXT",
            (PFN_xrVoidFunction*)(&xrCreateHandTrackerEXT_)));
        OXR(xrGetInstanceProcAddr(
            GetInstance(),
            "xrDestroyHandTrackerEXT",
            (PFN_xrVoidFunction*)(&xrDestroyHandTrackerEXT_)));
        OXR(xrGetInstanceProcAddr(
            GetInstance(),
            "xrLocateHandJointsEXT",
            (PFN_xrVoidFunction*)(&xrLocateHandJointsEXT_)));

        /// Hook up extensions for hand rendering
        OXR(xrGetInstanceProcAddr(
            GetInstance(), "xrGetHandMeshFB", (PFN_xrVoidFunction*)(&xrGetHandMeshFB_)));

        return true;
    }

    virtual void AppShutdown(const xrJava* context) override {
        /// unhook extensions for hand tracking
        xrCreateHandTrackerEXT_ = nullptr;
        xrDestroyHandTrackerEXT_ = nullptr;
        xrLocateHandJointsEXT_ = nullptr;
        xrGetHandMeshFB_ = nullptr;

        OVRFW::XrApp::AppShutdown(context);
        ui_.Shutdown();
    }

    virtual bool SessionInit() override {
        /// Disable scene navitgation
        GetScene().SetFootPos({0.0f, 0.0f, 0.0f});
        this->FreeMove = false;
        /// Init session bound objects
        if (false == controllerRenderL_.Init(true)) {
            ALOG("AppInit::Init L controller renderer FAILED.");
            return false;
        }
        if (false == controllerRenderR_.Init(false)) {
            ALOG("AppInit::Init R controller renderer FAILED.");
            return false;
        }
        beamRenderer_.Init(GetFileSys(), nullptr, OVR::Vector4f(1.0f), 1.0f);

        /// Hand rendering
        axisRendererL_.Init();
        axisRendererR_.Init();

        /// Hand Trackers
        if (xrCreateHandTrackerEXT_) {
            XrHandTrackerCreateInfoEXT createInfo{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
            createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
            createInfo.hand = XR_HAND_LEFT_EXT;
            OXR(xrCreateHandTrackerEXT_(GetSession(), &createInfo, &handTrackerL_));
            createInfo.hand = XR_HAND_RIGHT_EXT;
            OXR(xrCreateHandTrackerEXT_(GetSession(), &createInfo, &handTrackerR_));

            ALOG("xrCreateHandTrackerEXT handTrackerL_=%llx", (long long)handTrackerL_);
            ALOG("xrCreateHandTrackerEXT handTrackerR_=%llx", (long long)handTrackerR_);

            /// Setup skinning meshes for both hands
            if (xrGetHandMeshFB_) {
                for (int handIndex = 0; handIndex < 2; ++handIndex) {
                    /// Alias everything for initialization
                    const bool isLeft = (handIndex == 0);
                    auto& handTracker = isLeft ? handTrackerL_ : handTrackerR_;
                    auto& handRenderer = isLeft ? handRendererL_ : handRendererR_;
                    auto& handJointRenderers = isLeft ? handJointRenderersL_ : handJointRenderersR_;
                    auto* jointLocations = isLeft ? jointLocationsL_ : jointLocationsR_;
                    auto& handCapsuleRenderers =
                        isLeft ? handCapsuleRenderersL_ : handCapsuleRenderersR_;

                    /// two-call pattern for mesh data
                    /// call 1 - figure out sizes

                    /// mesh
                    XrHandTrackingMeshFB mesh{XR_TYPE_HAND_TRACKING_MESH_FB};
                    mesh.next = nullptr;
                    /// mesh - skeleton
                    mesh.jointCapacityInput = 0;
                    mesh.jointCountOutput = 0;
                    mesh.jointBindPoses = nullptr;
                    mesh.jointRadii = nullptr;
                    mesh.jointParents = nullptr;
                    /// mesh - vertex
                    mesh.vertexCapacityInput = 0;
                    mesh.vertexCountOutput = 0;
                    mesh.vertexPositions = nullptr;
                    mesh.vertexNormals = nullptr;
                    mesh.vertexUVs = nullptr;
                    mesh.vertexBlendIndices = nullptr;
                    mesh.vertexBlendWeights = nullptr;
                    /// mesh - index
                    mesh.indexCapacityInput = 0;
                    mesh.indexCountOutput = 0;
                    mesh.indices = nullptr;
                    /// get mesh sizes
                    OXR(xrGetHandMeshFB_(handTracker, &mesh));

                    /// mesh storage - update sizes
                    mesh.jointCapacityInput = mesh.jointCountOutput;
                    mesh.vertexCapacityInput = mesh.vertexCountOutput;
                    mesh.indexCapacityInput = mesh.indexCountOutput;
                    /// skeleton
                    std::vector<XrPosef> jointBindLocations;
                    std::vector<XrHandJointEXT> parentData;
                    std::vector<float> jointRadii;
                    jointBindLocations.resize(mesh.jointCountOutput);
                    parentData.resize(mesh.jointCountOutput);
                    jointRadii.resize(mesh.jointCountOutput);
                    mesh.jointBindPoses = jointBindLocations.data();
                    mesh.jointParents = parentData.data();
                    mesh.jointRadii = jointRadii.data();
                    /// vertex
                    std::vector<XrVector3f> vertexPositions;
                    std::vector<XrVector3f> vertexNormals;
                    std::vector<XrVector2f> vertexUVs;
                    std::vector<XrVector4sFB> vertexBlendIndices;
                    std::vector<XrVector4f> vertexBlendWeights;
                    vertexPositions.resize(mesh.vertexCountOutput);
                    vertexNormals.resize(mesh.vertexCountOutput);
                    vertexUVs.resize(mesh.vertexCountOutput);
                    vertexBlendIndices.resize(mesh.vertexCountOutput);
                    vertexBlendWeights.resize(mesh.vertexCountOutput);
                    mesh.vertexPositions = vertexPositions.data();
                    mesh.vertexNormals = vertexNormals.data();
                    mesh.vertexUVs = vertexUVs.data();
                    mesh.vertexBlendIndices = vertexBlendIndices.data();
                    mesh.vertexBlendWeights = vertexBlendWeights.data();
                    /// index
                    std::vector<int16_t> indices;
                    indices.resize(mesh.indexCountOutput);
                    mesh.indices = indices.data();

                    /// call 2 - fill in the data
                    /// chain capsules
                    XrHandTrackingCapsulesStateFB capsuleState{
                        XR_TYPE_HAND_TRACKING_CAPSULES_STATE_FB};
                    capsuleState.next = nullptr;
                    mesh.next = &capsuleState;

                    /// get mesh data
                    OXR(xrGetHandMeshFB_(handTracker, &mesh));
                    /// init renderer
                    handRenderer.Init(&mesh, true);
                    /// Render jointRadius for all left hand joints
                    {
                        handJointRenderers.resize(XR_HAND_JOINT_COUNT_EXT);
                        for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i) {
                            const OVR::Posef pose = FromXrPosef(jointLocations[i].pose);
                            OVRFW::GeometryRenderer& gr = handJointRenderers[i];
                            gr.Init(OVRFW::BuildTesselatedCapsuleDescriptor(
                                mesh.jointRadii[i], 0.0f, 7, 7));
                            gr.SetPose(pose);
                            gr.DiffuseColor = jointColor_;
                        }
                    }
                    /// One time init for capsules
                    {
                        handCapsuleRenderers.resize(XR_FB_HAND_TRACKING_CAPSULE_COUNT);
                        for (int i = 0; i < XR_FB_HAND_TRACKING_CAPSULE_COUNT; ++i) {
                            const OVR::Vector3f p0 =
                                FromXrVector3f(capsuleState.capsules[i].points[0]);
                            const OVR::Vector3f p1 =
                                FromXrVector3f(capsuleState.capsules[i].points[1]);
                            const OVR::Vector3f d = (p1 - p0);
                            const float h = d.Length();
                            const float r = capsuleState.capsules[i].radius;
                            const OVR::Quatf look = OVR::Quatf::LookRotation(d, {0, 1, 0});
                            OVRFW::GeometryRenderer& gr = handCapsuleRenderers[i];
                            gr.Init(OVRFW::BuildTesselatedCapsuleDescriptor(r, h, 7, 7));
                            gr.SetPose(OVR::Posef(look, p0));
                            gr.DiffuseColor = capsuleColor_;
                        }
                    }
                    /// Print hierarchy
                    {
                        for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i) {
                            const OVR::Posef pose = FromXrPosef(jointLocations[i].pose);
                            ALOG(
                                " { {%.6f, %.6f, %.6f},  {%.6f, %.6f, %.6f, %.6f} } // joint = %d, parent = %d",
                                pose.Translation.x,
                                pose.Translation.y,
                                pose.Translation.z,
                                pose.Rotation.x,
                                pose.Rotation.y,
                                pose.Rotation.z,
                                pose.Rotation.w,
                                i,
                                (int)parentData[i]);
                        }
                    }
                }
            }
        }

        return true;
    }

    virtual void SessionEnd() override {
        /// Hand Tracker
        if (xrDestroyHandTrackerEXT_) {
            OXR(xrDestroyHandTrackerEXT_(handTrackerL_));
            OXR(xrDestroyHandTrackerEXT_(handTrackerR_));
        }
        controllerRenderL_.Shutdown();
        controllerRenderR_.Shutdown();
        beamRenderer_.Shutdown();
        axisRendererL_.Shutdown();
        axisRendererR_.Shutdown();
        handRendererL_.Shutdown();
        handRendererR_.Shutdown();
    }

    // Update state
    virtual void Update(const OVRFW::ovrApplFrameIn& in) override {
        ui_.HitTestDevices().clear();

        if ((in.AllButtons & OVRFW::ovrApplFrameIn::kButtonY) != 0) {
            ALOG("Y button is pressed!");
        }
        if ((in.AllButtons & OVRFW::ovrApplFrameIn::kButtonMenu) != 0) {
            ALOG("Menu button is pressed!");
        }
        if ((in.AllButtons & OVRFW::ovrApplFrameIn::kButtonA) != 0) {
            ALOG("A button is pressed!");
        }
        if ((in.AllButtons & OVRFW::ovrApplFrameIn::kButtonB) != 0) {
            ALOG("B button is pressed!");
        }
        if ((in.AllButtons & OVRFW::ovrApplFrameIn::kButtonX) != 0) {
            ALOG("X button is pressed!");
        }

        /// Hands
        if (xrLocateHandJointsEXT_) {
            /// L
            XrHandTrackingScaleFB scaleL{XR_TYPE_HAND_TRACKING_SCALE_FB};
            scaleL.next = nullptr;
            scaleL.sensorOutput = 1.0f;
            scaleL.currentOutput = 1.0f;
            scaleL.overrideValueInput = 1.00f;
            scaleL.overrideHandScale = XR_FALSE; // XR_TRUE;
            XrHandTrackingCapsulesStateFB capsuleStateL{XR_TYPE_HAND_TRACKING_CAPSULES_STATE_FB};
            capsuleStateL.next = &scaleL;
            XrHandTrackingAimStateFB aimStateL{XR_TYPE_HAND_TRACKING_AIM_STATE_FB};
            aimStateL.next = &capsuleStateL;
            XrHandJointVelocitiesEXT velocitiesL{XR_TYPE_HAND_JOINT_VELOCITIES_EXT};
            velocitiesL.next = &aimStateL;
            velocitiesL.jointCount = XR_HAND_JOINT_COUNT_EXT;
            velocitiesL.jointVelocities = jointVelocitiesL_;
            XrHandJointLocationsEXT locationsL{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
            locationsL.next = &velocitiesL;
            locationsL.jointCount = XR_HAND_JOINT_COUNT_EXT;
            locationsL.jointLocations = jointLocationsL_;
            /// R
            XrHandTrackingScaleFB scaleR{XR_TYPE_HAND_TRACKING_SCALE_FB};
            scaleR.next = nullptr;
            scaleR.sensorOutput = 1.0f;
            scaleR.currentOutput = 1.0f;
            scaleR.overrideValueInput = 1.00f;
            scaleR.overrideHandScale = XR_FALSE; // XR_TRUE;
            XrHandTrackingCapsulesStateFB capsuleStateR{XR_TYPE_HAND_TRACKING_CAPSULES_STATE_FB};
            capsuleStateR.next = &scaleR;
            XrHandTrackingAimStateFB aimStateR{XR_TYPE_HAND_TRACKING_AIM_STATE_FB};
            aimStateR.next = &capsuleStateR;
            XrHandJointVelocitiesEXT velocitiesR{XR_TYPE_HAND_JOINT_VELOCITIES_EXT};
            velocitiesR.next = &aimStateR;
            velocitiesR.jointCount = XR_HAND_JOINT_COUNT_EXT;
            velocitiesR.jointVelocities = jointVelocitiesR_;
            XrHandJointLocationsEXT locationsR{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
            locationsR.next = &velocitiesR;
            locationsR.jointCount = XR_HAND_JOINT_COUNT_EXT;
            locationsR.jointLocations = jointLocationsR_;

            XrHandJointsLocateInfoEXT locateInfoL{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
            locateInfoL.baseSpace = GetStageSpace();
            locateInfoL.time = ToXrTime(in.PredictedDisplayTime);
            OXR(xrLocateHandJointsEXT_(handTrackerL_, &locateInfoL, &locationsL));

            XrHandJointsLocateInfoEXT locateInfoR{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
            locateInfoR.baseSpace = GetStageSpace();
            locateInfoR.time = ToXrTime(in.PredictedDisplayTime);
            OXR(xrLocateHandJointsEXT_(handTrackerR_, &locateInfoR, &locationsR));

            std::vector<OVR::Posef> handJointsL;
            std::vector<OVR::Posef> handJointsR;

            // Determine which joints are actually tracked
            // XrSpaceLocationFlags isTracked = XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT
            //    | XR_SPACE_LOCATION_POSITION_TRACKED_BIT;

            // Tracked joints and computed joints can all be valid
            XrSpaceLocationFlags isValid =
                XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;

            handTrackedL_ = false;
            handTrackedR_ = false;

            if (locationsL.isActive) {
                for (int i = 0; i < XR_FB_HAND_TRACKING_CAPSULE_COUNT; ++i) {
                    const OVR::Vector3f p0 = FromXrVector3f(capsuleStateL.capsules[i].points[0]);
                    const OVR::Vector3f p1 = FromXrVector3f(capsuleStateL.capsules[i].points[1]);
                    const OVR::Vector3f d = (p1 - p0);
                    const OVR::Quatf look = OVR::Quatf::LookRotation(d, {0, 1, 0});
                    /// apply inverse scale here
                    const float h = d.Length() / scaleL.currentOutput;
                    const OVR::Vector3f start = p0 + look.Rotate(OVR::Vector3f(0, 0, -h / 2));
                    OVRFW::GeometryRenderer& gr = handCapsuleRenderersL_[i];
                    gr.SetScale(OVR::Vector3f(scaleL.currentOutput));
                    gr.SetPose(OVR::Posef(look, start));
                    gr.Update();
                }
                for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i) {
                    if ((jointLocationsL_[i].locationFlags & isValid) != 0) {
                        const auto p = FromXrPosef(jointLocationsL_[i].pose);
                        handJointsL.push_back(p);
                        handTrackedL_ = true;
                        OVRFW::GeometryRenderer& gr = handJointRenderersL_[i];
                        gr.SetScale(OVR::Vector3f(scaleL.currentOutput));
                        gr.SetPose(p);
                        gr.Update();
                    }
                }
                handRendererL_.Update(&jointLocationsL_[0]);
                const bool didPinch =
                    (aimStateL.status & XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB) != 0;
                ui_.AddHitTestRay(FromXrPosef(aimStateL.aimPose), didPinch && !lastFrameClickedL_);
                lastFrameClickedL_ = didPinch;
            }
            if (locationsR.isActive) {
                for (int i = 0; i < XR_FB_HAND_TRACKING_CAPSULE_COUNT; ++i) {
                    const OVR::Vector3f p0 = FromXrVector3f(capsuleStateR.capsules[i].points[0]);
                    const OVR::Vector3f p1 = FromXrVector3f(capsuleStateR.capsules[i].points[1]);
                    const OVR::Vector3f d = (p1 - p0);
                    const OVR::Quatf look = OVR::Quatf::LookRotation(d, {0, 1, 0});
                    /// apply inverse scale here
                    const float h = d.Length() / scaleR.currentOutput;
                    const OVR::Vector3f start = p0 + look.Rotate(OVR::Vector3f(0, 0, -h / 2));
                    OVRFW::GeometryRenderer& gr = handCapsuleRenderersR_[i];
                    gr.SetScale(OVR::Vector3f(scaleR.currentOutput));
                    gr.SetPose(OVR::Posef(look, start));
                    gr.Update();
                }
                for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i) {
                    if ((jointLocationsR_[i].locationFlags & isValid) != 0) {
                        const auto p = FromXrPosef(jointLocationsR_[i].pose);
                        handJointsR.push_back(p);
                        handTrackedR_ = true;
                        OVRFW::GeometryRenderer& gr = handJointRenderersR_[i];
                        gr.SetScale(OVR::Vector3f(scaleR.currentOutput));
                        gr.SetPose(p);
                        gr.Update();
                    }
                }
                handRendererR_.Update(&jointLocationsR_[0]);
                const bool didPinch =
                    (aimStateR.status & XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB) != 0;

                ui_.AddHitTestRay(FromXrPosef(aimStateR.aimPose), didPinch && !lastFrameClickedR_);
                lastFrameClickedR_ = didPinch;
            }
            axisRendererL_.Update(handJointsL);
            axisRendererR_.Update(handJointsR);
        }

        if (in.LeftRemoteTracked) {
            controllerRenderL_.Update(in.LeftRemotePose);
            const bool didPinch = in.LeftRemoteIndexTrigger > 0.5f;
            ui_.AddHitTestRay(in.LeftRemotePointPose, didPinch);
        }
        if (in.RightRemoteTracked) {
            controllerRenderR_.Update(in.RightRemotePose);
            const bool didPinch = in.RightRemoteIndexTrigger > 0.5f;
            ui_.AddHitTestRay(in.RightRemotePointPose, didPinch);
        }

        ui_.Update(in);
        beamRenderer_.Update(in, ui_.HitTestDevices());
    }

    // Render eye buffers while running
    virtual void Render(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) override {
        /// Render UI
        ui_.Render(in, out);

        /// Render controllers
        if (in.LeftRemoteTracked) {
            controllerRenderL_.Render(out.Surfaces);
        }
        if (in.RightRemoteTracked) {
            controllerRenderR_.Render(out.Surfaces);
        }

        /// Render hand axes
        if (handTrackedL_) {
            axisRendererL_.Render(OVR::Matrix4f(), in, out);

            if (renderJointsL_) {
                for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i) {
                    OVRFW::GeometryRenderer& gr = handJointRenderersL_[i];
                    gr.Render(out.Surfaces);
                }
            }

            if (renderCapsulesL_) {
                for (int i = 0; i < XR_FB_HAND_TRACKING_CAPSULE_COUNT; ++i) {
                    OVRFW::GeometryRenderer& gr = handCapsuleRenderersL_[i];
                    gr.Render(out.Surfaces);
                }
            }

            if (renderMeshL_) {
                handRendererL_.Render(out.Surfaces);
            }
        }
        if (handTrackedR_) {
            axisRendererR_.Render(OVR::Matrix4f(), in, out);

            if (renderJointsR_) {
                for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i) {
                    OVRFW::GeometryRenderer& gr = handJointRenderersR_[i];
                    gr.Render(out.Surfaces);
                }
            }

            if (renderCapsulesR_) {
                for (int i = 0; i < XR_FB_HAND_TRACKING_CAPSULE_COUNT; ++i) {
                    OVRFW::GeometryRenderer& gr = handCapsuleRenderersR_[i];
                    gr.Render(out.Surfaces);
                }
            }

            if (renderMeshR_) {
                handRendererR_.Render(out.Surfaces);
            }
        }

        /// Render beams
        beamRenderer_.Render(in, out);
    }

   public:
    /// Hands - extension functions
    PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT_ = nullptr;
    PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT_ = nullptr;
    PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT_ = nullptr;
    /// Hands - FB mesh rendering extensions
    PFN_xrGetHandMeshFB xrGetHandMeshFB_ = nullptr;
    /// Hands - tracker handles
    XrHandTrackerEXT handTrackerL_ = XR_NULL_HANDLE;
    XrHandTrackerEXT handTrackerR_ = XR_NULL_HANDLE;
    /// Hands - data buffers
    XrHandJointLocationEXT jointLocationsL_[XR_HAND_JOINT_COUNT_EXT];
    XrHandJointLocationEXT jointLocationsR_[XR_HAND_JOINT_COUNT_EXT];
    XrHandJointVelocityEXT jointVelocitiesL_[XR_HAND_JOINT_COUNT_EXT];
    XrHandJointVelocityEXT jointVelocitiesR_[XR_HAND_JOINT_COUNT_EXT];

   private:
    OVRFW::ControllerRenderer controllerRenderL_;
    OVRFW::ControllerRenderer controllerRenderR_;
    OVRFW::HandRenderer handRendererL_;
    OVRFW::HandRenderer handRendererR_;
    OVRFW::TinyUI ui_;
    OVRFW::SimpleBeamRenderer beamRenderer_;
    std::vector<OVRFW::ovrBeamRenderer::handle_t> beams_;
    OVRFW::ovrAxisRenderer axisRendererL_;
    OVRFW::ovrAxisRenderer axisRendererR_;
    bool handTrackedL_ = false;
    bool handTrackedR_ = false;

    bool lastFrameClickedL_ = false;
    bool lastFrameClickedR_ = false;

    OVR::Vector4f jointColor_{0.4, 0.5, 0.2, 0.5};
    OVR::Vector4f capsuleColor_{0.4, 0.2, 0.2, 0.5};

    bool renderMeshL_ = true;
    bool renderMeshR_ = true;
    bool renderJointsL_ = true;
    bool renderJointsR_ = true;
    bool renderCapsulesL_ = true;
    bool renderCapsulesR_ = true;
    std::vector<OVRFW::GeometryRenderer> handJointRenderersL_;
    std::vector<OVRFW::GeometryRenderer> handJointRenderersR_;
    std::vector<OVRFW::GeometryRenderer> handCapsuleRenderersL_;
    std::vector<OVRFW::GeometryRenderer> handCapsuleRenderersR_;
};

ENTRY_POINT(XrHandsApp)
