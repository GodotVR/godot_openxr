// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/************************************************************************************

Filename    :   ControllerRenderer.h
Content     :   A one stop for rendering controllers
Created     :   July 2020
Authors     :   Federico Schliemann

************************************************************************************/

#pragma once

#include <vector>
#include <string>
#include <memory>

/// Sample Framework
#include "Model/SceneView.h"
#include "Model/ModelFile.h"
#include "Render/GlProgram.h"
#include "Render/SurfaceRender.h"

#include "OVR_Math.h"

namespace OVRFW {

class ControllerRenderer {
   public:
    ControllerRenderer() = default;
    ~ControllerRenderer() = default;

    bool Init(
        bool leftController,
        OVRFW::ovrFileSys* fileSys = nullptr,
        const char* controllerModelFile = nullptr);
    void Shutdown();
    void Update(const OVR::Posef& pose);
    void Render(std::vector<ovrDrawSurface>& surfaceList);

    bool IsLeft() const {
        return isLeftController;
    }

    void LoadModelFromResource(OVRFW::ovrFileSys* fileSys, const char* controllerModelFile);

   public:
    OVR::Vector3f SpecularLightDirection;
    OVR::Vector3f SpecularLightColor;
    OVR::Vector3f AmbientLightColor;

   private:
    bool isLeftController;
    OVRFW::GlProgram ProgControllerTexture;
    OVRFW::GlProgram ProgControllerColor;
    OVRFW::ovrSurfaceDef ControllerSurfaceDef;
    OVRFW::ovrDrawSurface ControllerSurface;
    OVRFW::ModelFile* Model;
};

} // namespace OVRFW
