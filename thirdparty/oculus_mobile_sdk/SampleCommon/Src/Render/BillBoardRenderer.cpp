// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/************************************************************************************

Filename    :   BillBoardRenderer.cpp
Content     :   Class that manages and renders view-oriented billboards.
Created     :   October 23; 2015
Authors     :   Jonathan E. Wright

*************************************************************************************/

#include "BillBoardRenderer.h"
#include "TextureAtlas.h"

#include "Misc/Log.h"

using OVR::Matrix4f;
using OVR::Posef;
using OVR::Quatf;
using OVR::Vector2f;
using OVR::Vector3f;
using OVR::Vector4f;

inline Vector3f GetViewMatrixPosition(Matrix4f const& m) {
    return m.Inverted().GetTranslation();
}

inline Vector3f GetViewMatrixUp(Matrix4f const& m) {
    return m.Inverted().GetYBasis();
}

inline Vector3f GetViewMatrixRight(Matrix4f const& m) {
    return m.Inverted().GetXBasis();
}

inline Vector3f GetViewMatrixForward(Matrix4f const& m) {
    return m.Inverted().GetZBasis();
}

namespace OVRFW {

static const char* BillBoardVertexSrc = R"glsl(
attribute vec4 Position;
attribute vec4 VertexColor;
attribute vec2 TexCoord;

varying lowp vec4 outColor;
varying highp vec2 oTexCoord;

void main()
{
	gl_Position = TransformVertex( Position );
	oTexCoord = TexCoord;
   	outColor = VertexColor;
}
)glsl";

static const char* TextureFragmentSrc = R"glsl(
uniform sampler2D Texture0;

varying lowp vec4 outColor;
varying highp vec2 oTexCoord;

void main()
{
	gl_FragColor = outColor * texture2D( Texture0, oTexCoord );
}
)glsl";

static const char* ParametricFragmentSrc = R"glsl(
varying lowp vec4 outColor;
varying highp vec2 oTexCoord;

void main()
{
    gl_FragColor = outColor;
}
)glsl";

float ovrBillBoardRenderer::LIFETIME_INFINITE = FLT_MAX;

//==============================
// ovrBillBoardRenderer::ovrBillBoardRenderer
ovrBillBoardRenderer::ovrBillBoardRenderer() : MaxBillBoards(0) {}

//==============================
// ovrBillBoardRenderer::ovrBillBoardRenderer
ovrBillBoardRenderer::~ovrBillBoardRenderer() {
    Shutdown();
}

//==============================
// ovrBillBoardRenderer::Init
void ovrBillBoardRenderer::Init(const int maxBillBoards, const bool depthTest) {
    Shutdown();

    MaxBillBoards = maxBillBoards;

    if (TextureProgram.VertexShader == 0 || TextureProgram.FragmentShader == 0) {
        OVRFW::ovrProgramParm uniformParms[] = {
            /// Vertex
            /// Fragment
            {"Texture0", OVRFW::ovrProgramParmType::TEXTURE_SAMPLED},
        };
        const int uniformCount = sizeof(uniformParms) / sizeof(OVRFW::ovrProgramParm);
        TextureProgram = OVRFW::GlProgram::Build(
            BillBoardVertexSrc, TextureFragmentSrc, uniformParms, uniformCount);
    }
    if (ParametricProgram.VertexShader == 0 || ParametricProgram.FragmentShader == 0) {
        ParametricProgram =
            OVRFW::GlProgram::Build(BillBoardVertexSrc, ParametricFragmentSrc, nullptr, 0);
    }

    const int numVerts = maxBillBoards * 4;

    VertexAttribs attr;
    attr.position.resize(numVerts);
    attr.uv0.resize(numVerts);
    attr.color.resize(numVerts);

    // the indices will never change once we've set them up; we just won't necessarily
    // use all of the index buffer to render.
    std::vector<TriangleIndex> indices;
    indices.resize(MaxBillBoards * 6);

    for (int i = 0; i < MaxBillBoards; i++) {
        indices[i * 6 + 0] = static_cast<TriangleIndex>(i * 4 + 0);
        indices[i * 6 + 1] = static_cast<TriangleIndex>(i * 4 + 1);
        indices[i * 6 + 2] = static_cast<TriangleIndex>(i * 4 + 3);
        indices[i * 6 + 3] = static_cast<TriangleIndex>(i * 4 + 0);
        indices[i * 6 + 4] = static_cast<TriangleIndex>(i * 4 + 3);
        indices[i * 6 + 5] = static_cast<TriangleIndex>(i * 4 + 2);
    }

    Surf.surfaceName = "billboards";
    Surf.geo.Create(attr, indices);
    Surf.geo.primitiveType = GL_TRIANGLES;
    Surf.geo.indexCount = 0;

    ovrGraphicsCommand& gc = Surf.graphicsCommand;
    gc.GpuState.depthEnable = gc.GpuState.depthMaskEnable = depthTest;
    gc.GpuState.blendEnable = ovrGpuState::BLEND_DISABLE;
    gc.GpuState.blendSrc = GL_ONE;
    gc.Program = TextureProgram;
    gc.GpuState.lineWidth = 2.0f;
}

//==============================
// ovrBillBoardRenderer::Shutdown
void ovrBillBoardRenderer::Shutdown() {
    Surf.geo.Free();
    OVRFW::GlProgram::Free(TextureProgram);
    OVRFW::GlProgram::Free(ParametricProgram);

    MaxBillBoards = 0;
    FreeBillBoards.resize(0);
    ActiveBillBoards.resize(0);
    BillBoardInfos.resize(0);
}

//==============================
// ovrBillBoardRenderer::AddBillBoard
ovrBillBoardRenderer::handle_t ovrBillBoardRenderer::AddBillBoard(
    const OVRFW::ovrApplFrameIn& frame,
    const ovrTextureAtlas& atlas,
    const int atlasIndex,
    const float width,
    const float height,
    const Vector3f& pos,
    const Vector4f& initialColor,
    const float lifeTime) {
    handle_t handle;

    // ALOG( "ovrBillBoardRenderer::AddDebugLine" );
    if (FreeBillBoards.size() > 0) {
        handle = FreeBillBoards[static_cast<int>(FreeBillBoards.size()) - 1];
        FreeBillBoards.pop_back();
    } else {
        handle = handle_t(static_cast<uint16_t>(BillBoardInfos.size()));
        if (handle.Get() >= MaxBillBoards || handle.Get() >= MAX_BILLBOARDS) {
            return handle_t();
        }
        BillBoardInfos.push_back(ovrBillBoardInfo());
    }

    assert(handle.IsValid());
    assert(handle.Get() < static_cast<int>(BillBoardInfos.size()));
    assert(handle.Get() < MAX_BILLBOARDS);

    ActiveBillBoards.push_back(handle);

    UpdateBillBoardInternal(
        frame, handle, &atlas, atlasIndex, width, height, pos, initialColor, lifeTime);

    return (lifeTime == LIFETIME_INFINITE) ? handle : handle_t();
}

//==============================
// ovrBillBoardRenderer::AddBillBoard
ovrBillBoardRenderer::handle_t ovrBillBoardRenderer::AddBillBoard(
    const OVRFW::ovrApplFrameIn& frame,
    const float width,
    const float height,
    const Vector3f& pos,
    const OVR::Vector4f& initialColor) {
    handle_t handle;

    // ALOG( "ovrBillBoardRenderer::AddDebugLine" );
    if (FreeBillBoards.size() > 0) {
        handle = FreeBillBoards[static_cast<int>(FreeBillBoards.size()) - 1];
        FreeBillBoards.pop_back();
    } else {
        handle = handle_t(static_cast<uint16_t>(BillBoardInfos.size()));
        if (handle.Get() >= MaxBillBoards || handle.Get() >= MAX_BILLBOARDS) {
            return handle_t();
        }
        BillBoardInfos.push_back(ovrBillBoardInfo());
    }

    assert(handle.IsValid());
    assert(handle.Get() < static_cast<int>(BillBoardInfos.size()));
    assert(handle.Get() < MAX_BILLBOARDS);
    ActiveBillBoards.push_back(handle);

    UpdateBillBoardInternal(
        frame, handle, nullptr, 0, width, height, pos, initialColor, LIFETIME_INFINITE);

    return handle;
}

//==============================
// ovrBillBoardRenderer::UpdateBillBoard
void ovrBillBoardRenderer::UpdateBillBoard(
    const OVRFW::ovrApplFrameIn& frame,
    const handle_t handle,
    const ovrTextureAtlas& atlas,
    const int atlasIndex,
    const float width,
    const float height,
    const Vector3f& pos,
    const Vector4f& initialColor) {
    assert(BillBoardInfos[handle.Get()].Handle.IsValid());
    UpdateBillBoardInternal(
        frame, handle, &atlas, atlasIndex, width, height, pos, initialColor, LIFETIME_INFINITE);
}

void ovrBillBoardRenderer::UpdateBillBoard(
    const OVRFW::ovrApplFrameIn& frame,
    const handle_t handle,
    const float width,
    const float height,
    const Vector3f& pos,
    const Vector4f& initialColor) {
    assert(BillBoardInfos[handle.Get()].Handle.IsValid());
    UpdateBillBoardInternal(
        frame, handle, nullptr, 0, width, height, pos, initialColor, LIFETIME_INFINITE);
}

void ovrBillBoardRenderer::RemoveBillBoard(const handle_t handle) {
    if (!handle.IsValid() || handle.Get() >= BillBoardInfos.size()) {
        return;
    }
    BillBoardInfos[handle.Get()].StartTime = -1.0;
    BillBoardInfos[handle.Get()].LifeTime = -1.0f;
}

//==============================
// ovrBillBoardRenderer::UpdateBillBoardInternal
void ovrBillBoardRenderer::UpdateBillBoardInternal(
    const OVRFW::ovrApplFrameIn& frame,
    const handle_t handle,
    const ovrTextureAtlas* atlas,
    const int atlasIndex,
    const float width,
    const float height,
    const Vector3f& pos,
    const Vector4f& initialColor,
    float const lifeTime) {
    if (!handle.IsValid()) {
        assert(handle.IsValid());
        return;
    }

    ovrBillBoardInfo& billboard = BillBoardInfos[handle.Get()];

    billboard.Handle = handle;
    billboard.StartTime = frame.PredictedDisplayTime;
    billboard.LifeTime = lifeTime;
    billboard.Width = width;
    billboard.Height = height;
    billboard.AtlasIndex = static_cast<uint16_t>(atlasIndex);
    billboard.Pos = pos;
    billboard.InitialColor = initialColor;
    if (atlas == nullptr) {
        billboard.TexCoords[0] = {0.0f, 0.0f}; // min tex coords
        billboard.TexCoords[1] = {1.0f, 1.0f}; // max tex coords
    } else {
        const ovrTextureAtlas::ovrSpriteDef& sd = atlas->GetSpriteDef(atlasIndex);
        billboard.TexCoords[0] = sd.uvMins; // min tex coords
        billboard.TexCoords[1] = sd.uvMaxs; // max tex coords
    }
}

//==============================
// ovrBillBoardRenderer::Frame
void ovrBillBoardRenderer::Frame(
    const OVRFW::ovrApplFrameIn& frame,
    const Matrix4f& centerViewMatrix,
    const ovrTextureAtlas& atlas) {
    FrameInternal(frame, centerViewMatrix, &atlas);
}
void ovrBillBoardRenderer::Frame(
    const OVRFW::ovrApplFrameIn& frame,
    const Matrix4f& centerViewMatrix) {
    FrameInternal(frame, centerViewMatrix, nullptr);
}

//==============================
// ovrBillBoardRenderer::Frame
void ovrBillBoardRenderer::FrameInternal(
    const OVRFW::ovrApplFrameIn& frame,
    const OVR::Matrix4f& centerViewMatrix,
    const class ovrTextureAtlas* atlas) {
    /// we
    if (atlas) {
        Surf.geo.indexCount = 0;
        Surf.graphicsCommand.Textures[0] = atlas->GetTexture();
        Surf.graphicsCommand.BindUniformTextures();
        Surf.graphicsCommand.Program = TextureProgram;
    } else {
        Surf.graphicsCommand.Program = ParametricProgram;
    }

    VertexAttribs attr;
    attr.position.resize(ActiveBillBoards.size() * 4);
    attr.color.resize(ActiveBillBoards.size() * 4);
    attr.uv0.resize(ActiveBillBoards.size() * 4);

    const Vector3f viewPos = GetViewMatrixPosition(centerViewMatrix);
    const Vector3f viewUp = Vector3f(0.0f, 1.0f, 0.0f);

    int quadIndex = 0;
    for (int i = 0; i < static_cast<int>(ActiveBillBoards.size()); ++i) {
        const handle_t billboardHandle = ActiveBillBoards[i];
        if (!billboardHandle.IsValid()) {
            continue;
        }

        const ovrBillBoardInfo& cur = BillBoardInfos[billboardHandle.Get()];
        double const timeAlive = frame.PredictedDisplayTime - cur.StartTime;
        if (timeAlive > cur.LifeTime) {
            BillBoardInfos[billboardHandle.Get()].Handle = handle_t();
            FreeBillBoards.push_back(billboardHandle);
            ActiveBillBoards[i] = ActiveBillBoards.back();
            ActiveBillBoards.pop_back();
            i--;
            continue;
        }

        const Vector3f viewForward = (cur.Pos - viewPos).Normalized();
        const Vector3f viewRight = viewForward.Cross(viewUp).Normalized();

        const Vector3f up = viewUp * cur.Height * 0.5f;
        const Vector3f right = viewRight * cur.Width * 0.5f;

        const float t = static_cast<float>(frame.PredictedDisplayTime - cur.StartTime);

        const Vector4f color = EaseFunctions[cur.EaseFunc](cur.InitialColor, t / cur.LifeTime);
        const Vector2f uvOfs(0.0f);

        attr.position[quadIndex * 4 + 0] = cur.Pos + up - right;
        attr.position[quadIndex * 4 + 1] = cur.Pos - up - right;
        attr.position[quadIndex * 4 + 2] = cur.Pos + up + right;
        attr.position[quadIndex * 4 + 3] = cur.Pos - up + right;
        attr.color[quadIndex * 4 + 0] = color;
        attr.color[quadIndex * 4 + 1] = color;
        attr.color[quadIndex * 4 + 2] = color;
        attr.color[quadIndex * 4 + 3] = color;
        attr.uv0[quadIndex * 4 + 0] = Vector2f(cur.TexCoords[0].x, cur.TexCoords[0].y) + uvOfs;
        attr.uv0[quadIndex * 4 + 1] = Vector2f(cur.TexCoords[1].x, cur.TexCoords[0].y) + uvOfs;
        attr.uv0[quadIndex * 4 + 2] = Vector2f(cur.TexCoords[0].x, cur.TexCoords[1].y) + uvOfs;
        attr.uv0[quadIndex * 4 + 3] = Vector2f(cur.TexCoords[1].x, cur.TexCoords[1].y) + uvOfs;

        quadIndex++;
    }

    // Surf.graphicsCommand.GpuState.polygonMode = GL_LINE;
    Surf.graphicsCommand.GpuState.cullEnable = false;
    Surf.geo.indexCount = quadIndex * 6;
    Surf.geo.Update(attr);
}

//==============================
// ovrBillBoardRenderer::RenderEyeView
void ovrBillBoardRenderer::RenderEyeView(
    const Matrix4f& /*viewMatrix*/,
    const Matrix4f& /*projMatrix*/,
    std::vector<ovrDrawSurface>& surfaceList) {
    if (Surf.geo.indexCount > 0) {
        surfaceList.push_back(ovrDrawSurface(ModelMatrix, &Surf));
    }
}

void ovrBillBoardRenderer::Render(std::vector<ovrDrawSurface>& surfaceList) {
    if (Surf.geo.indexCount > 0) {
        surfaceList.push_back(ovrDrawSurface(ModelMatrix, &Surf));
    }
}

//==============================
// ovrBillBoardRenderer::SetPose
void ovrBillBoardRenderer::SetPose(const Posef& pose) {
    ModelMatrix = Matrix4f(pose);
}

} // namespace OVRFW
