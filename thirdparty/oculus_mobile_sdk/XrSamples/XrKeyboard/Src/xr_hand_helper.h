/************************************************************************************************
Filename    :   xr_hand_helper.h
Content     :   Helper Inteface for openxr hand extensions
Created     :   April 2021
Authors     :   Federico Schliemann
Language    :   C++
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.
************************************************************************************************/
#pragma once

#include "xr_helper.h"

class XrHandHelper : public XrHelper {
   public:
    XrHandHelper(XrInstance instance, bool isLeft) : XrHelper(instance), isLeft_(isLeft) {
        /// Hook up extensions for hand tracking
        oxr(xrGetInstanceProcAddr(
            instance, "xrCreateHandTrackerEXT", (PFN_xrVoidFunction*)(&xrCreateHandTrackerEXT_)));
        oxr(xrGetInstanceProcAddr(
            instance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction*)(&xrDestroyHandTrackerEXT_)));
        oxr(xrGetInstanceProcAddr(
            instance, "xrLocateHandJointsEXT", (PFN_xrVoidFunction*)(&xrLocateHandJointsEXT_)));
        /// Hook up extensions for hand rendering
        oxr(xrGetInstanceProcAddr(
            instance, "xrGetHandMeshFB", (PFN_xrVoidFunction*)(&xrGetHandMeshFB_)));
    }

    ~XrHandHelper() override {
        xrCreateHandTrackerEXT_ = nullptr;
        xrDestroyHandTrackerEXT_ = nullptr;
        xrLocateHandJointsEXT_ = nullptr;
        xrGetHandMeshFB_ = nullptr;
    };

    /// XrHelper Interface
    virtual bool SessionInit(XrSession session) override {
        if (xrCreateHandTrackerEXT_) {
            XrHandTrackerCreateInfoEXT createInfo{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
            createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
            createInfo.hand = isLeft_ ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
            if (!oxr(xrCreateHandTrackerEXT_(session, &createInfo, &handTracker_))) {
                return false;
            }

            if (xrGetHandMeshFB_) {
                mesh_.type = XR_TYPE_HAND_TRACKING_MESH_FB;
                mesh_.next = nullptr;
                mesh_.jointCapacityInput = 0;
                mesh_.jointCountOutput = 0;
                mesh_.vertexCapacityInput = 0;
                mesh_.vertexCountOutput = 0;
                mesh_.indexCapacityInput = 0;
                mesh_.indexCountOutput = 0;
                if (!oxr(xrGetHandMeshFB_(handTracker_, &mesh_))) {
                    return false;
                }
                /// update sizes
                mesh_.jointCapacityInput = mesh_.jointCountOutput;
                mesh_.vertexCapacityInput = mesh_.vertexCountOutput;
                mesh_.indexCapacityInput = mesh_.indexCountOutput;
                vertexPositions_.resize(mesh_.vertexCapacityInput);
                vertexNormals_.resize(mesh_.vertexCapacityInput);
                vertexUVs_.resize(mesh_.vertexCapacityInput);
                vertexBlendIndices_.resize(mesh_.vertexCapacityInput);
                vertexBlendWeights_.resize(mesh_.vertexCapacityInput);
                indices_.resize(mesh_.indexCapacityInput);

                /// skeleton
                mesh_.jointBindPoses = jointBindPoses_;
                mesh_.jointParents = jointParents_;
                mesh_.jointRadii = jointRadii_;
                /// mesh
                mesh_.vertexPositions = vertexPositions_.data();
                mesh_.vertexNormals = vertexNormals_.data();
                mesh_.vertexUVs = vertexUVs_.data();
                mesh_.vertexBlendIndices = vertexBlendIndices_.data();
                mesh_.vertexBlendWeights = vertexBlendWeights_.data();
                mesh_.indices = indices_.data();
                /// get mesh
                if (!oxr(xrGetHandMeshFB_(handTracker_, &mesh_))) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    virtual bool SessionEnd() override {
        if (xrDestroyHandTrackerEXT_) {
            return oxr(xrDestroyHandTrackerEXT_(handTracker_));
        }
        return false;
    }

    virtual bool Update(XrSpace currentSpace, XrTime predictedDisplayTime) override {
        if (xrLocateHandJointsEXT_) {
            /// aim
            aimState_.next = nullptr;
            /// scale
            scale_.next = &aimState_;
            scale_.sensorOutput = 1.0f;
            scale_.currentOutput = 1.0f;
            scale_.overrideHandScale = XR_TRUE;
            scale_.overrideValueInput = 1.00f;
            /// locations
            locations_.next = &scale_;
            locations_.jointCount = XR_HAND_JOINT_COUNT_EXT;
            locations_.jointLocations = jointLocations_;
            XrHandJointsLocateInfoEXT locateInfo{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
            locateInfo.baseSpace = currentSpace;
            locateInfo.time = predictedDisplayTime;
            return oxr(xrLocateHandJointsEXT_(handTracker_, &locateInfo, &locations_));
        }
        return false;
    }

    static std::vector<const char*> RequiredExtensionNames() {
        return {
            XR_EXT_HAND_TRACKING_EXTENSION_NAME,
            XR_FB_HAND_TRACKING_MESH_EXTENSION_NAME,
            XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME};
    }

   public:
    /// Own interface
    bool IsLeft() const {
        return isLeft_;
    }
    const XrHandTrackingMeshFB& Mesh() const {
        return mesh_;
    }
    const XrHandTrackingScaleFB& Scale() const {
        return scale_;
    }
    const XrPosef* JointBindPoses() const {
        return jointBindPoses_;
    }
    const XrHandJointEXT* JointParents() const {
        return jointParents_;
    }
    const XrHandJointLocationEXT* Joints() const {
        return jointLocations_;
    }
    float RenderScale() const {
        return scale_.sensorOutput;
    }
    bool IsTracking() const {
        return (handTracker_ != XR_NULL_HANDLE);
    }
    bool AreLocationsActive() const {
        return IsTracking() && (locations_.isActive);
    }
    bool IsPositionValid() const {
        return jointLocations_[XR_HAND_JOINT_PALM_EXT].locationFlags &
            XR_SPACE_LOCATION_POSITION_VALID_BIT;
    }
    const XrPosef& AimPose() const {
        return aimState_.aimPose;
    }
    bool IndexPinching() const {
        return (aimState_.status & XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB) != 0;
    }
    const XrHandTrackingAimStateFB& AimState() const {
        return aimState_;
    }

   private:
    bool isLeft_;
    /// Hands - extension functions
    PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT_ = nullptr;
    PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT_ = nullptr;
    PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT_ = nullptr;
    /// Hands - FB mesh rendering extensions
    PFN_xrGetHandMeshFB xrGetHandMeshFB_ = nullptr;
    /// Hands - tracker handles
    XrHandTrackerEXT handTracker_ = XR_NULL_HANDLE;
    /// Hands - data buffers
    XrHandJointLocationEXT jointLocations_[XR_HAND_JOINT_COUNT_EXT];
    XrPosef jointBindPoses_[XR_HAND_JOINT_COUNT_EXT];
    XrHandJointEXT jointParents_[XR_HAND_JOINT_COUNT_EXT];
    float jointRadii_[XR_HAND_JOINT_COUNT_EXT];
    /// mesh storage
    XrHandTrackingMeshFB mesh_{XR_TYPE_HAND_TRACKING_MESH_FB};
    std::vector<XrVector3f> vertexPositions_;
    std::vector<XrVector3f> vertexNormals_;
    std::vector<XrVector2f> vertexUVs_;
    std::vector<XrVector4sFB> vertexBlendIndices_;
    std::vector<XrVector4f> vertexBlendWeights_;
    std::vector<int16_t> indices_;
    /// extension nodes
    /// scale
    XrHandTrackingScaleFB scale_{XR_TYPE_HAND_TRACKING_SCALE_FB};
    /// aim
    XrHandTrackingAimStateFB aimState_{XR_TYPE_HAND_TRACKING_AIM_STATE_FB};
    /// location
    XrHandJointLocationsEXT locations_{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
};
