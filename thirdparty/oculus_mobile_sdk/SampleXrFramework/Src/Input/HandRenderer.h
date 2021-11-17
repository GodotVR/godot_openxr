/************************************************************************************

Filename    :   HandRenderer.h
Content     :   A one stop for rendering hands
Created     :   April 2020
Authors     :   Federico Schliemann

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

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

#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include <openxr/openxr.h>
#include <openxr/openxr_oculus.h>
#include <openxr/openxr_oculus_helpers.h>
#include <openxr/openxr_platform.h>

#ifndef XR_FB_hand_tracking_mesh
// Hands
#include <openxr/fb_hand_tracking_mesh.h>
#endif

namespace OVRFW {

class HandRenderer {
   public:
    HandRenderer() = default;
    ~HandRenderer() = default;

    bool Init(const XrHandTrackingMeshFB* mesh, bool leftHand);
    void Shutdown();
    void Update(const XrHandJointLocationEXT* joints, const float scale = 1.0f);
    void Render(std::vector<ovrDrawSurface>& surfaceList);

    bool IsLeftHand() const {
        return isLeftHand;
    }
    const std::vector<OVR::Matrix4f>& Transforms() const {
        return TransformMatrices;
    }

   public:
    OVR::Vector3f SpecularLightDirection;
    OVR::Vector3f SpecularLightColor;
    OVR::Vector3f AmbientLightColor;
    OVR::Vector3f GlowColor;
    float Confidence;

   private:
    bool isLeftHand;
    GlProgram ProgHand;
    ovrSurfaceDef HandSurfaceDef;
    ovrDrawSurface HandSurface;
    std::vector<OVR::Matrix4f> TransformMatrices;
    std::vector<OVR::Matrix4f> BindMatrices;
    std::vector<OVR::Matrix4f> SkinMatrices;
    GlBuffer SkinUniformBuffer;
};

} // namespace OVRFW
