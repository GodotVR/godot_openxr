/************************************************************************************************
Filename    :   KeyboardRenderer.h
Content     :   A one stop for rendering keyboards
Created     :   April 2021
Authors     :   Federico Schliemann
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.
************************************************************************************************/
#pragma once

#include <memory>
#include <string>
#include <vector>

/// Sample Framework
#include "Misc/Log.h"
#include "Model/SceneView.h"
#include "Render/GlProgram.h"
#include "Render/SurfaceRender.h"
#include "OVR_FileSys.h"
#include "OVR_Math.h"

#if defined(ANDROID)
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#else
#include "unknwn.h"
#define XR_USE_GRAPHICS_API_OPENGL 1
#define XR_USE_PLATFORM_WIN32 1
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_oculus.h>
#include <openxr/openxr_oculus_helpers.h>
#include <openxr/openxr_platform.h>

namespace OVRFW {

class KeyboardRenderer {
   public:
    KeyboardRenderer() = default;
    ~KeyboardRenderer() = default;

    bool Init(std::vector<uint8_t>& keyboardBuffer);
    void Shutdown();
    void Update(const OVR::Posef& pose, const OVR::Vector3f& scale = OVR::Vector3f(1.0, 1.0, 1.0f));
    void Render(std::vector<ovrDrawSurface>& surfaceList);

   public:
    OVR::Vector3f SpecularLightDirection;
    OVR::Vector3f SpecularLightColor;
    OVR::Vector3f AmbientLightColor;
    bool UseSolidTexture = true;
    float Opacity = 1.0f;

   private:
    float AlphaBlendFactor = 1.0f;
    GlProgram ProgKeyboard;
    ModelFile* KeyboardModel = nullptr;
    GlTexture KeyboardTextureSolid;
    OVR::Matrix4f Transform;
};

} // namespace OVRFW
