// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/************************************************************************************

Filename    :   AxisRenderer.h
Content     :   A rendering component for axis
Created     :   September 2020
Authors     :   Federico Schliemann

************************************************************************************/

#pragma once

#include <vector>

#include "OVR_Math.h"
#include "FrameParams.h"
#include "Render/SurfaceRender.h"
#include "Render/GlProgram.h"

namespace OVRFW {

class ovrAxisRenderer {
   public:
    ovrAxisRenderer() = default;
    ~ovrAxisRenderer() = default;

    bool Init(size_t count = 64, float size = 0.025f);
    void Shutdown();
    void Update(const std::vector<OVR::Posef>& points);
    void Update(const OVR::Posef* points, size_t count);
    void Render(
        const OVR::Matrix4f& worldMatrix,
        const OVRFW::ovrApplFrameIn& in,
        OVRFW::ovrRendererOutput& out);

   private:
    float AxisSize;
    GlProgram ProgAxis;
    ovrSurfaceDef AxisSurfaceDef;
    ovrDrawSurface AxisSurface;
    std::vector<OVR::Matrix4f> TransformMatrices;
    GlBuffer InstancedBoneUniformBuffer;
    size_t Count;
};

} // namespace OVRFW
