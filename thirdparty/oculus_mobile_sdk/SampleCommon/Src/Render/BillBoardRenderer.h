// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/************************************************************************************

Filename    :   BillBoardRenderer.h
Content     :   Class that manages and renders view-oriented billboards.
Created     :   October 23, 2015
Authors     :   Jonathan E. Wright

*************************************************************************************/

#pragma once

#include <vector>

#include "OVR_Math.h"
#include "OVR_TypesafeNumber.h"

#include "FrameParams.h"
#include "Render/SurfaceRender.h"
#include "Render/GlProgram.h"

#include "TextureAtlas.h"
#include "EaseFunctions.h"

namespace OVRFW {

//==============================================================
// ovrBillBoardRenderer
class ovrBillBoardRenderer {
   public:
    static const int MAX_BILLBOARDS = (1ULL << (sizeof(uint16_t) * 8)) - 1;
    enum ovrBillBoardHandle { INVALID_BILLBOARD_HANDLE = MAX_BILLBOARDS };
    typedef OVR::TypesafeNumberT<uint16_t, ovrBillBoardHandle, INVALID_BILLBOARD_HANDLE> handle_t;

    static float LIFETIME_INFINITE;

    ovrBillBoardRenderer();
    ~ovrBillBoardRenderer();

    void Init(const int maxBillBoards, const bool depthTest);
    void Shutdown();

    void Frame(
        const OVRFW::ovrApplFrameIn& frame,
        const OVR::Matrix4f& centerViewMatrix,
        const class ovrTextureAtlas& atlas);
    void Frame(const OVRFW::ovrApplFrameIn& frame, const OVR::Matrix4f& centerViewMatrix);

    void SetPose(const OVR::Posef& pose);

    void RenderEyeView(
        const OVR::Matrix4f& viewMatrix,
        const OVR::Matrix4f& projMatrix,
        std::vector<ovrDrawSurface>& surfaceList);
    void Render(std::vector<ovrDrawSurface>& surfaceList);

    // If lifeTime == LIFETIME_INFINITE, then the billboard will never be automatically removed and
    // it can be referenced by handle. The handle will be returned from this function.
    // If the lifeTime != LIFETIME_INFINITE, then this function will still add the billboard (if the
    // max billboards has not been reached) but return a handle == MAX_BILLBOARDS
    handle_t AddBillBoard(
        const OVRFW::ovrApplFrameIn& frame,
        const ovrTextureAtlas& atlas,
        const int atlasIndex,
        const float width,
        const float height,
        const OVR::Vector3f& pos,
        const OVR::Vector4f& initialColor,
        const float lifeTime);

    // Updates the properties of the billboard with the specified handle
    void UpdateBillBoard(
        const OVRFW::ovrApplFrameIn& frame,
        const handle_t handle,
        const ovrTextureAtlas& atlas,
        const int atlasIndex,
        const float width,
        const float height,
        const OVR::Vector3f& pos,
        const OVR::Vector4f& initialColor);

    handle_t AddBillBoard(
        const OVRFW::ovrApplFrameIn& frame,
        const float width,
        const float height,
        const OVR::Vector3f& pos,
        const OVR::Vector4f& initialColor);
    void UpdateBillBoard(
        const OVRFW::ovrApplFrameIn& frame,
        const handle_t handle,
        const float width,
        const float height,
        const OVR::Vector3f& pos,
        const OVR::Vector4f& initialColor);

    // removes the billboard with the specified handle
    void RemoveBillBoard(handle_t const handle);

   private:
    void FrameInternal(
        const OVRFW::ovrApplFrameIn& frame,
        const OVR::Matrix4f& centerViewMatrix,
        const class ovrTextureAtlas* atlas);
    void UpdateBillBoardInternal(
        const OVRFW::ovrApplFrameIn& frame,
        const handle_t handle,
        const ovrTextureAtlas* atlas,
        const int atlasIndex,
        const float width,
        const float height,
        const OVR::Vector3f& pos,
        const OVR::Vector4f& initialColor,
        const float lifeTime);

    struct ovrBillBoardInfo {
        ovrBillBoardInfo()
            : StartTime(0.0),
              LifeTime(0.0f),
              Width(0.0f),
              Height(0.0f),
              Pos(0.0f),
              InitialColor(0.0f),
              TexCoords(),
              AtlasIndex(0),
              Handle(MAX_BILLBOARDS),
              EaseFunc(ovrEaseFunc::NONE) {}

        double StartTime;
        float LifeTime;
        float Width;
        float Height;
        OVR::Vector3f Pos;
        OVR::Vector4f InitialColor;
        OVR::Vector2f TexCoords[2]; // tex coords are in the space of the atlas entry
        uint16_t AtlasIndex; // index in the texture atlas
        handle_t Handle;
        ovrEaseFunc EaseFunc;
    };

    ovrSurfaceDef Surf;

    std::vector<ovrBillBoardInfo> BillBoardInfos;
    std::vector<handle_t> ActiveBillBoards;
    std::vector<handle_t> FreeBillBoards;

    int MaxBillBoards;

    GlProgram TextureProgram;
    GlProgram ParametricProgram;
    OVR::Matrix4f ModelMatrix;
};

} // namespace OVRFW
