/*******************************************************************************

Filename    :   Main.cpp
Content     :   Simple test app for XrColorSpaceFB
Created     :
Authors     :   Andreas Selvik
Language    :   C++
Copyright:  Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*******************************************************************************/

#include <string>
#include <string_view>
#include <unordered_map>

#include <openxr/openxr.h>

#include "GUI/VRMenuObject.h"
#include "Render/BitmapFont.h"
#include "Render/GeometryBuilder.h"
#include "openxr_reflection_macro.h"
#include "XrApp.h"

#include "OVR_Math.h"
#include "Input/ControllerRenderer.h"
#include "Input/TinyUI.h"
#include "Render/GeometryRenderer.h"
#include "Render/SimpleBeamRenderer.h"

class XrAppBaseApp : public OVRFW::XrApp {
   public:
    static constexpr std::string_view sampleExplanation =
        "XR devices may display RGB color triplets differently from many   \n"
        "monitors used in development, that is, they may use a different   \n"
        "color space than what is common for computer monitors.            \n"
        "\n"
        "XrColorSpaceFB is an extension that allow a developer to specify  \n"
        "the color space in which they have authored their application so  \n"
        "the correct colors are shown when the application is running on   \n"
        "the XR device.                                                    \n"
        "\n"
        "For the majority of developers, the recommendation for Meta Quest \n"
        "devices is to author your content in sRGB (XR_COLOR_SPACE_REC_709)\n"
        "and leave the color space conversion to the default value (which  \n"
        "is \"XR_COLOR_SPACE_RIFT_CV1_FB\" on Meta Quest 2).               \n"
        "For more details and recommendations for color management, see:   \n"
        "https://developer.oculus.com/resources/color-management-guide/    \n";

    XrAppBaseApp() : OVRFW::XrApp() {
        BackgroundColor = OVR::Vector4f(0.55f, 0.35f, 0.1f, 1.0f);
    }

    // Returns a list of OpenXR extensions needed for this app
    virtual std::vector<const char*> GetExtensions() override {
        std::vector<const char*> extensions = XrApp::GetExtensions();

        // Add "XR_FB_color_space" to the list of extensions
        // The sample framework will add this to enabledExtensionNames
        // when calling xrCreateInstance()
        extensions.push_back(XR_FB_COLOR_SPACE_EXTENSION_NAME);
        return extensions;
    }

    // Before this function, OVRFW::XrApp::Init() calls, among other things;
    //  - xrInitializeLoaderKHR(...)
    //  - xrCreateInstance with the extensions from GetExtensions(...),
    //  - xrSuggestInteractionProfileBindings(...) to set up action bindings
    // Before calling this function:
    virtual bool AppInit(const xrJava* context) override {
        // Init UI system
        if (false == ui_.Init(context, GetFileSys())) {
            ALOG("TinyUI::Init FAILED.");
            return false;
        }

        // Bind extension functions so we can call them
        OXR(xrGetInstanceProcAddr(
            GetInstance(),
            "xrEnumerateColorSpacesFB",
            reinterpret_cast<PFN_xrVoidFunction*>(&xrEnumerateColorSpacesFB)));
        OXR(xrGetInstanceProcAddr(
            GetInstance(),
            "xrSetColorSpaceFB",
            reinterpret_cast<PFN_xrVoidFunction*>(&xrSetColorSpaceFB)));

        return true;
    }

    // Before this function, and after AppInit, XrApp::InitSession() calls:
    // - xrCreateSession(...) to create our Session
    // - xrCreateReferenceSpace(...) for local and stage space
    // - Create swapchain with xrCreateSwapchain(...)
    // - xrAttachSessionActionSets(...)
    // Before calling this function:
    virtual bool SessionInit() override {
        {
            // Get native HMD color space by chaining XrSystemColorSpacePropertiesFB
            // to the next pointer of XrSystemProperties
            XrSystemColorSpacePropertiesFB colorSpaceProps{
                XR_TYPE_SYSTEM_COLOR_SPACE_PROPERTIES_FB};
            XrSystemProperties sysprops{XR_TYPE_SYSTEM_PROPERTIES};
            sysprops.next = &colorSpaceProps;
            OXR(xrGetSystemProperties(GetInstance(), GetSystemId(), &sysprops));

            // The sample just displays the native HMD color space,
            // but an app could use this to load different assets
            // and to inform which colorspace it wants to select.
            std::string str =
                "Native HMD color space: " + std::string(XrEnumStr(colorSpaceProps.colorSpace));
            ui_.AddLabel(str, {-1.0f, 2.4f, -2.0f}, {700.0f, 45.0f});
        }

        // The default source color space is not something that's
        // provided by XrColorSpaceFB OpenXR extension, so just initialize
        // the label to say [default]
        declaredLabel_ = ui_.AddLabel(
            "App declared color space: [default]", {-1.0f, 2.3f, -2.0f}, {700.0f, 45.0f});

        { // Enumerate supported source color spaces with xrEnumerateColorSpaceFB

            // xrEnumerateColorSpacesFB uses the two-call idiom
            // First call with nullptr to get the required size:
            uint32_t colorSpaceCount{0};
            OXR(xrEnumerateColorSpacesFB(GetSession(), 0, &colorSpaceCount, nullptr));

            // Allocate array with enough space for the data
            auto colorSpaces = std::vector<XrColorSpaceFB>(colorSpaceCount);

            // Second call gets the actual data
            OXR(xrEnumerateColorSpacesFB(
                GetSession(), (uint32_t)colorSpaces.size(), &colorSpaceCount, colorSpaces.data()));

            // Header
            ui_.AddLabel("Compatible color spaces:", {-1.0f, 2.15f, -2.0f}, {700.0f, 45.0f});

            // Create one button for each supported color space conversion
            float posY = 2.0f;
            for (auto colorSpace : colorSpaces) {
                ui_.AddButton(XrEnumStr(colorSpace), {-1.0f, posY, -2.0f}, {600.0f, 60.0f}, [=]() {
                    // Set source color space with xrSetColorSpaceFB when button is pressed
                    // Remember that this is the source colorspace and no the target colorspace
                    OXR(xrSetColorSpaceFB(GetSession(), colorSpace));

                    declaredColorSpace_ = colorSpace; // For display in the UI
                });
                posY -= 0.15f;
            }
        }

        // Create the rest of the UI
        CreateColorCheckerCubes();
        CreateSampleDescriptionPanel();

        /// Disable scene navitgation
        GetScene().SetFootPos({0.0f, 0.0f, 0.0f});
        this->FreeMove = false;

        // Init objects that need OpenXR Session
        if (false == controllerRenderL_.Init(true)) {
            ALOG("SessionInit::Init L controller renderer FAILED.");
            return false;
        }
        if (false == controllerRenderR_.Init(false)) {
            ALOG("SessionInit::Init R controller renderer FAILED.");
            return false;
        }
        cursorBeamRenderer_.Init(GetFileSys(), nullptr, OVR::Vector4f(1.0f), 1.0f);

        return true;
    }

    // Called every frame
    virtual void Update(const OVRFW::ovrApplFrameIn& in) override {
        // Update the UI with the latest declared color space
        if (declaredColorSpace_ != XR_COLOR_SPACE_MAX_ENUM_FB) {
            std::string label = "App declared color space: ";
            label += XrEnumStr(declaredColorSpace_);
            declaredLabel_->SetText(label.c_str());
        }

        colorCheckerRenderer_.Update();

        // Clear the intersection rays from last frame:
        ui_.HitTestDevices().clear();

        if (in.LeftRemoteTracked) {
            controllerRenderL_.Update(in.LeftRemotePose);
            const bool didTrigger = in.LeftRemoteIndexTrigger > 0.5f;
            ui_.AddHitTestRay(in.LeftRemotePointPose, didTrigger);
        }

        if (in.RightRemoteTracked) {
            controllerRenderR_.Update(in.RightRemotePose);
            const bool didTrigger = in.RightRemoteIndexTrigger > 0.5f;
            ui_.AddHitTestRay(in.RightRemotePointPose, didTrigger);
        }

        ui_.Update(in);
        cursorBeamRenderer_.Update(in, ui_.HitTestDevices());
    }

    virtual void Render(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) override {
        colorCheckerRenderer_.Render(out.Surfaces);

        ui_.Render(in, out);

        if (in.LeftRemoteTracked) {
            controllerRenderL_.Render(out.Surfaces);
        }
        if (in.RightRemoteTracked) {
            controllerRenderR_.Render(out.Surfaces);
        }

        /// Render beams last, since they render with transparency
        cursorBeamRenderer_.Render(in, out);
    }

    virtual void SessionEnd() override {
        controllerRenderL_.Shutdown();
        controllerRenderR_.Shutdown();
        cursorBeamRenderer_.Shutdown();
        colorCheckerRenderer_.Shutdown();
    }

    virtual void AppShutdown(const xrJava* context) override {
        OVRFW::XrApp::AppShutdown(context);
        ui_.Shutdown();
    }

    void CreateColorCheckerCubes() {
        // From https://en.wikipedia.org/wiki/ColorChecker
        // These values are authored for the sRGB (Rec. 709) color space
        std::vector<std::string> Rec709ColorChecker = {
            // clang-format off
            "#735244", "#c29682", "#627a9d", "#576c43", "#8580b1", "#67bdaa",
            "#d67e2c", "#505ba6", "#c15a63", "#5e3c6c", "#9dbc40", "#e0a32e",
            "#383d96", "#469449", "#af363c", "#e7c71f", "#bb5695", "#0885a1",
            "#f3f3f2", "#c8c8c8", "#a0a0a0", "#7a7a7a", "#555555", "#343434"
            // clang-format on
        };

        auto linearRgbaFromGammaHex = [](std::string_view hex) {
            float r = std::stoul(std::string(hex.substr(1, 2)), nullptr, 16) / 256.0f;
            float g = std::stoul(std::string(hex.substr(3, 2)), nullptr, 16) / 256.0f;
            float b = std::stoul(std::string(hex.substr(5, 2)), nullptr, 16) / 256.0f;

            // Convert colors from gamma 2.2 into linear space for direct shader usage
            // Note: If you were to use a texture with these RGB values, and you configure
            // the texture as an sRGB texture, this conversion would happen in the GPU
            // as your shader reads the texels. But since we are providing the values
            // directly into the fragment shader, we need to do this manually.
            // Try removing this part and see what happens to the colors :)
            r = powf(r, 2.2);
            g = powf(g, 2.2);
            b = powf(b, 2.2);

            return OVR::Vector4f{r, g, b, 1.0f};
        };

        OVR::Vector3f chartPose = {0.5, 1.5f, -2.0f};
        ui_.AddLabel(
            "Color checker authored for sRGB (XR_COLOR_SPACE_REC_709_FB):",
            {chartPose.x, chartPose.y + 0.65f, chartPose.z},
            {750.0f, 45.0f});

        OVRFW::GeometryBuilder colorCheckerCubes;

        constexpr float tileSize = 0.2f;
        constexpr float tileGap = 0.04f;
        constexpr float chartSizeX = 6 * tileSize + 5 * tileGap;
        constexpr float chartSizeY = 4 * tileSize + 3 * tileGap;
        constexpr float chartBorder = 0.1f;

        auto transform = OVR::Matrix4f::Translation(chartPose) *
            OVR::Matrix4f::Scaling(
                             chartSizeX + 2 * chartBorder, chartSizeY + 2 * chartBorder, 0.01);
        colorCheckerCubes.Add(
            OVRFW::BuildUnitCubeDescriptor(),
            OVRFW::GeometryBuilder::kInvalidIndex,
            {0.0f, 0.0f, 0.0f, 1.0f},
            transform);

        int i = 0;
        for (auto tileColor : Rec709ColorChecker) {
            int row = i / 6;
            int col = i % 6;
            i++;

            float tileX = chartPose.x - (chartSizeX - tileSize) / 2.0f + (tileSize + tileGap) * col;
            float tileY = chartPose.y + (chartSizeY - tileSize) / 2.0f - (tileSize + tileGap) * row;
            float tileZ = chartPose.z + 0.005f;

            transform = OVR::Matrix4f::Translation({tileX, tileY, tileZ}) *
                OVR::Matrix4f::Scaling(tileSize, tileSize, 0.1);

            colorCheckerCubes.Add(
                OVRFW::BuildUnitCubeDescriptor(),
                OVRFW::GeometryBuilder::kInvalidIndex,
                linearRgbaFromGammaHex(tileColor),
                transform);

            ui_.AddLabel(tileColor, {tileX, tileY, tileZ + 0.07f}, {100.0f, 45.0f})
                ->SetColor({0, 0, 0, 0}); // Set label background to transparent
        }
        colorCheckerRenderer_.Init(colorCheckerCubes.ToGeometryDescriptor());

        // Tricks to disable shading and lighting of the color checker
        // to ensure the final image contains the exact RGB values we want:
        colorCheckerRenderer_.ChannelControl = {0, 1, 0}; // Only enable ambient color
        // Pass through vertex color to the final image by using the ambient light
        colorCheckerRenderer_.AmbientLightColor = {1, 1, 1};
    }

    void CreateSampleDescriptionPanel() {
        // Panel to provide sample description to the user for context
        auto descriptionLabel = ui_.AddLabel(
            static_cast<std::string>(sampleExplanation), {2.0f, 1.5f, -1.5f}, {950.0f, 600.0f});

        // Align and size the description text for readability
        OVRFW::VRMenuFontParms fontParams{};
        fontParams.Scale = 0.5f;
        fontParams.AlignHoriz = OVRFW::HORIZONTAL_LEFT;
        descriptionLabel->SetFontParms(fontParams);
        descriptionLabel->SetTextLocalPosition({-0.65f, 0, 0});

        // Tilt the description billboard 45 degrees towards the user
        descriptionLabel->SetLocalRotation(
            OVR::Quat<float>::FromRotationVector({0, OVR::DegreeToRad(-45.0f), 0}));
    }

   private:
    // Function pointers to extension functions
    // Remember that you need to call xrGetInstanceProcAddr after
    // instance initiation to get these
    PFN_xrEnumerateColorSpacesFB xrEnumerateColorSpacesFB{nullptr};
    PFN_xrSetColorSpaceFB xrSetColorSpaceFB{nullptr};

    XrColorSpaceFB declaredColorSpace_{XR_COLOR_SPACE_MAX_ENUM_FB};

    OVRFW::VRMenuObject* declaredLabel_;
    OVRFW::GeometryRenderer colorCheckerRenderer_;
    OVRFW::ControllerRenderer controllerRenderL_;
    OVRFW::ControllerRenderer controllerRenderR_;
    OVRFW::TinyUI ui_;

    // Renderer that draws the beam from the controller
    OVRFW::SimpleBeamRenderer cursorBeamRenderer_;
    std::vector<OVRFW::ovrBeamRenderer::handle_t> beams_;
};

ENTRY_POINT(XrAppBaseApp)
