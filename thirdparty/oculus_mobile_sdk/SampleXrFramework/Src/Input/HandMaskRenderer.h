// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/************************************************************************************

Filename    :   HandMaskRenderer.h
Content     :   A one stop for rendering hand masks
Created     :   03/24/2020
Authors     :   Federico Schliemann

************************************************************************************/

#pragma once

#include <vector>
#include <string>
#include <memory>

/// Sample Framework
#include "Misc/Log.h"
#include "Model/SceneView.h"
#include "Render/GlProgram.h"
#include "Render/SurfaceRender.h"

#include "OVR_Math.h"

#if defined(ANDROID)
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#elif defined(WIN32)
#include <unknwn.h>
#define XR_USE_GRAPHICS_API_OPENGL 1
#define XR_USE_PLATFORM_WIN32 1
#endif // defined(ANDROID)

#include <openxr/openxr.h>
#include <openxr/openxr_oculus.h>
#include <openxr/openxr_oculus_helpers.h>
#include <openxr/openxr_platform.h>

#ifndef XR_FB_hand_tracking_mesh
// Hands
#include <openxr/fb_hand_tracking_mesh.h>
#endif

namespace OVRFW {

class HandMaskRenderer {
   public:
    HandMaskRenderer() = default;
    ~HandMaskRenderer() = default;

    void Init(bool leftHand);
    void Shutdown();
    void Update(
        const OVR::Posef& headPose,
        const OVR::Posef& handPose,
        const std::vector<OVR::Matrix4f>& jointTransforms,
        const float handSize = 1.0f);
    void Render(std::vector<ovrDrawSurface>& surfaceList);

   public:
    float LayerBlend;
    float Falloff;
    float Intensity;
    float FadeIntensity;
    bool UseBorderFade;
    float BorderFadeSize;
    float AlphaMaskSize;
    bool RenderInverseSubtract;
    ovrSurfaceDef HandMaskSurfaceDef;

   private:
    GlProgram ProgHandMaskAlphaGradient;
    GlProgram ProgHandMaskBorderFade;
    ovrDrawSurface HandMaskSurface;
    std::vector<OVR::Matrix4f> HandMaskMatrices;
    std::vector<OVR::Vector3f> HandMaskColors;
    GlBuffer HandMaskUniformBuffer;
    GlBuffer HandColorUniformBuffer;
    bool IsLeftHand;
};

} // namespace OVRFW
