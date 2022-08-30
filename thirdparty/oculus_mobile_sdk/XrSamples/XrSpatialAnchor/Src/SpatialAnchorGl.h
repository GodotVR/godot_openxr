#pragma once

#include <vector>

#if defined(ANDROID)
#include <GLES3/gl3.h>

#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#elif defined(WIN32)
#include "Render/GlWrapperWin32.h"

#include <unknwn.h>
#define XR_USE_GRAPHICS_API_OPENGL 1
#define XR_USE_PLATFORM_WIN32 1
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_oculus.h>
#include <openxr/openxr_oculus_helpers.h>
#include <openxr/openxr_platform.h>

#include "OVR_Math.h"

#define NUM_EYES 2

struct ovrGeometry {
    void Clear();
    void CreateCube();
    void CreateAxes();
    void CreateStage();
    void Destroy();
    void CreateVAO();
    void DestroyVAO();
    static constexpr int MAX_VERTEX_ATTRIB_POINTERS = 3;
    struct VertexAttribPointer {
        GLint Index;
        GLint Size;
        GLenum Type;
        GLboolean Normalized;
        GLsizei Stride;
        const GLvoid* Pointer;
    };
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    GLuint VertexArrayObject;
    int VertexCount;
    int IndexCount;
    VertexAttribPointer VertexAttribs[MAX_VERTEX_ATTRIB_POINTERS];
};

struct ovrProgram {
    static constexpr int MAX_PROGRAM_UNIFORMS = 8;
    static constexpr int MAX_PROGRAM_TEXTURES = 8;

    void Clear();
    bool Create(const char* vertexSource, const char* fragmentSource);
    void Destroy();
    GLuint Program;
    GLuint VertexShader;
    GLuint FragmentShader;
    // These will be -1 if not used by the program.
    GLint UniformLocation[MAX_PROGRAM_UNIFORMS]; // ProgramUniforms[].name
    GLint UniformBinding[MAX_PROGRAM_UNIFORMS]; // ProgramUniforms[].name
    GLint Textures[MAX_PROGRAM_TEXTURES]; // Texture%i
};

struct ovrFramebuffer {
    void Clear();
    bool Create(
        const GLenum colorFormat,
        const int width,
        const int height,
        const int multisamples,
        const int swapChainLength,
        GLuint* colorTextures);
    void Destroy();
    void Bind(int element);
    void Unbind();
    void Resolve();
    int Width;
    int Height;
    int Multisamples;
    int SwapChainLength;
    struct Element {
        GLuint ColorTexture;
        GLuint DepthTexture;
        GLuint FrameBufferObject;
    };
    Element* Elements;
};

struct ovrCubeData {
    ovrCubeData() : ColorScale(1.0f, 1.0f, 1.0f, 1.0f), ColorBias(0.0f, 0.0f, 0.0f, 0.0f) {}
    OVR::Vector4f ColorScale;
    OVR::Vector4f ColorBias;
    OVR::Matrix4f Model;
};

struct ovrScene {
    void Clear();
    void Create();
    void Destroy();
    bool IsCreated();
    void SetClearColor(const float* c);
    void CreateVAOs();
    void DestroyVAOs();
    bool CreatedScene;
    bool CreatedVAOs;
    ovrProgram CubeProgram;
    ovrGeometry Cube;
    GLuint SceneMatrices;
    ovrProgram StageProgram;
    ovrGeometry Stage;
    ovrProgram AxesProgram;
    ovrGeometry Axes;
    float ClearColor[4];

    std::vector<ovrCubeData> CubeData;
    std::vector<XrSpace> SpaceList;
};

struct ovrAppRenderer {
    void Clear();
    void Create(
        GLenum format,
        int width,
        int height,
        int numMultiSamples,
        int swapChainLength,
        GLuint* colorTextures);
    void Destroy();

    struct FrameIn {
        int SwapChainIndex;
        OVR::Matrix4f View[NUM_EYES];
        OVR::Matrix4f Proj[NUM_EYES];
        bool HasStage;
        OVR::Posef StagePose;
        OVR::Vector3f StageScale;
    };

    void RenderFrame(FrameIn frameIn);

    ovrFramebuffer Framebuffer;
    ovrScene Scene;
};
