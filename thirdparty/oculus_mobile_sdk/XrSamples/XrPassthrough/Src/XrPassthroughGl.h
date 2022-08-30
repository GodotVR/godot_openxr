#pragma once

#if defined(ANDROID)
#include <GLES3/gl3.h>
#else
#include "Render/GlWrapperWin32.h"
#endif // defined(ANDROID)

#include "OVR_Math.h"

#ifndef NUM_EYES
#define NUM_EYES 2
#endif

struct Geometry {
    void Clear();
    void CreateAxes();
    void CreateStage();
    void CreateBox();
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

struct Program {
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

struct Framebuffer {
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

struct TrackedController {
    void Clear();
    bool Active;
    OVR::Posef Pose;
};

struct Scene {
    void Clear();
    void Create();
    void Destroy();
    bool IsCreated();
    void SetClearColor(const float* c);
    void CreateVAOs();
    void DestroyVAOs();
    bool CreatedScene;
    bool CreatedVAOs;
    GLuint SceneMatrices;
    Program StageProgram;
    Geometry Stage;
    Program AxesProgram;
    Geometry Axes;
    Program BoxProgram;
    Geometry Box;
    Program CircleProgram;
    float ClearColor[4];
    TrackedController trackedController[4]; // left aim, left grip, right aim, right grip
};

struct AppRenderer {
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

    Framebuffer framebuffer;
    Scene scene;
};
