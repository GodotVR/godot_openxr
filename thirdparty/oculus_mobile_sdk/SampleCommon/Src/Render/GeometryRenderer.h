// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/*******************************************************************************

Filename    :   GeometryRenderer.h
Content     :   Simple rendering for geometry-based types
Created     :   Mar 2021
Authors     :   Federico Schliemann
Language    :   C++

*******************************************************************************/

#pragma once
#include <vector>

#include "OVR_Math.h"
#include "SurfaceRender.h"
#include "GlProgram.h"
#include "GeometryBuilder.h"

namespace OVRFW {

class GeometryRenderer {
   public:
    GeometryRenderer() = default;
    virtual ~GeometryRenderer() = default;

    virtual void Init(const GlGeometry::Descriptor& d);
    virtual void Shutdown();
    virtual void Update();
    virtual void Render(std::vector<ovrDrawSurface>& surfaceList);

    void SetPose(const OVR::Posef& pose) {
        ModelPose_ = pose;
    }
    OVR::Posef GetPose() {
        return ModelPose_;
    }
    void SetScale(OVR::Vector3f v) {
        ModelScale_ = v;
    }
    OVR::Vector3f GetScale() {
        return ModelScale_;
    }

   public:
    OVR::Vector4f ChannelControl = {1, 1, 1, 1};
    OVR::Vector4f DiffuseColor = {0.4, 1.0, 0.2, 1.0};
    OVR::Vector3f SpecularLightDirection = {1, 1, 1};
    OVR::Vector3f SpecularLightColor = {1, 1, 1};
    OVR::Vector3f AmbientLightColor = {.1, .1, .1};
    GLenum BlendSrc = GL_SRC_ALPHA;
    GLenum BlendDst = GL_ONE_MINUS_SRC_ALPHA;
    GLenum BlendMode = GL_FUNC_ADD;

   private:
    ovrSurfaceDef SurfaceDef_;
    GlProgram Program_;
    OVR::Matrix4f ModelMatrix_ = OVR::Matrix4f::Identity();
    OVR::Vector3f ModelScale_ = {1, 1, 1};
    OVR::Posef ModelPose_ = OVR::Posef::Identity();
};

} // namespace OVRFW

//// --------------------------------
