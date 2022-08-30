/************************************************************************************

Filename	:	SceneModelGl.cpp
Content		:	This sample is derived from VrCubeWorld_SurfaceView.
                When used in room scale mode, it draws a "carpet" under the
                user to indicate where it is safe to walk around.
Created		:	July, 2020
Authors		:	Cass Everitt

Copyright	:	Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h> // for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/native_window_jni.h> // for native window JNI
#include <android/input.h>

#include <atomic>
#include <thread>

#include <sys/system_properties.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "SceneModelGl.h"
#include "SceneModelHelpers.h"
#include "SceneModelShaders.h"

using namespace OVR;

// EXT_texture_border_clamp
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif

#ifndef GL_FRAMEBUFFER_SRGB_EXT
#define GL_FRAMEBUFFER_SRGB_EXT 0x8DB9
#endif

#if !defined(GL_EXT_multisampled_render_to_texture)
typedef void(GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)(
    GLenum target,
    GLsizei samples,
    GLenum internalformat,
    GLsizei width,
    GLsizei height);
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)(
    GLenum target,
    GLenum attachment,
    GLenum textarget,
    GLuint texture,
    GLint level,
    GLsizei samples);
#endif

#if !defined(GL_OVR_multiview)
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)(
    GLenum target,
    GLenum attachment,
    GLuint texture,
    GLint level,
    GLint baseViewIndex,
    GLsizei numViews);
#endif

#if !defined(GL_OVR_multiview_multisampled_render_to_texture)
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)(
    GLenum target,
    GLenum attachment,
    GLuint texture,
    GLint level,
    GLsizei samples,
    GLint baseViewIndex,
    GLsizei numViews);
#endif

#define DEBUG 1
#define OVR_LOG_TAG "SceneModelGl"

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, OVR_LOG_TAG, __VA_ARGS__)
#if DEBUG
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, OVR_LOG_TAG, __VA_ARGS__)
#else
#define ALOGV(...)
#endif

/*
================================================================================

OpenGL-ES Utility Functions

================================================================================
*/

namespace {
struct OpenGLExtensions_t {
    bool multi_view; // GL_OVR_multiview, GL_OVR_multiview2
    bool EXT_texture_border_clamp; // GL_EXT_texture_border_clamp, GL_OES_texture_border_clamp
    bool EXT_sRGB_write_control;
};

OpenGLExtensions_t glExtensions;

Matrix4f ZOffsetTransform(const float zOffset) {
    Matrix4f transform = Matrix4f::Identity();
    transform(2, 3) = zOffset;
    return transform;
}

} // namespace

static void EglInitExtensions() {
    glExtensions = {};
    const char* allExtensions = (const char*)glGetString(GL_EXTENSIONS);
    if (allExtensions != nullptr) {
        glExtensions.multi_view = strstr(allExtensions, "GL_OVR_multiview2") &&
            strstr(allExtensions, "GL_OVR_multiview_multisampled_render_to_texture");

        glExtensions.EXT_texture_border_clamp =
            strstr(allExtensions, "GL_EXT_texture_border_clamp") ||
            strstr(allExtensions, "GL_OES_texture_border_clamp");
        glExtensions.EXT_sRGB_write_control = strstr(allExtensions, "GL_EXT_sRGB_write_control");
    }
}

static const char* GlFrameBufferStatusString(GLenum status) {
    switch (status) {
        case GL_FRAMEBUFFER_UNDEFINED:
            return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_UNSUPPORTED:
            return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        default:
            return "unknown";
    }
}

#ifdef CHECK_GL_ERRORS

static const char* GlErrorString(GLenum error) {
    switch (error) {
        case GL_NO_ERROR:
            return "GL_NO_ERROR";
        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";
        default:
            return "unknown";
    }
}

static void GLCheckErrors(int line) {
    for (int i = 0; i < 10; i++) {
        const GLenum error = glGetError();
        if (error == GL_NO_ERROR) {
            break;
        }
        ALOGE("GL error on line %d: %s", line, GlErrorString(error));
    }
}

#define GL(func) \
    func;        \
    GLCheckErrors(__LINE__);

#else // CHECK_GL_ERRORS

#define GL(func) func;

#endif // CHECK_GL_ERRORS

/*
================================================================================

ovrGeometry

================================================================================
*/

enum VertexAttributeLocation {
    VERTEX_ATTRIBUTE_LOCATION_POSITION,
    VERTEX_ATTRIBUTE_LOCATION_COLOR,
    VERTEX_ATTRIBUTE_LOCATION_UV
};

struct ovrVertexAttribute {
    enum VertexAttributeLocation location;
    const char* name;
};

static ovrVertexAttribute ProgramVertexAttributes[] = {
    {VERTEX_ATTRIBUTE_LOCATION_POSITION, "vertexPosition"},
    {VERTEX_ATTRIBUTE_LOCATION_COLOR, "vertexColor"},
    {VERTEX_ATTRIBUTE_LOCATION_UV, "vertexUv"}};

void ovrGeometry::Clear() {
    VertexBuffer_ = 0;
    IndexBuffer_ = 0;
    VertexArrayObject_ = 0;
    for (int i = 0; i < MAX_VERTEX_ATTRIB_POINTERS; i++) {
        memset(&VertexAttribs_[i], 0, sizeof(VertexAttribs_[i]));
        VertexAttribs_[i].Index = -1;
    }

    IsRenderable_ = false;
}

void ovrGeometry::BindVAO() const {
    GL(glBindVertexArray(VertexArrayObject_));
}

void ovrGeometry::CreateIndexBuffer(const std::vector<unsigned short>& indices) {
    if (IndexBuffer_ == 0) {
        GL(glGenBuffers(1, &IndexBuffer_));
    }
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer_));
    GL(glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned short),
        indices.data(),
        GL_STATIC_DRAW));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    IndexCount_ = indices.size();
}

void ovrGeometry::CreateAxes() {
    struct AxesVertices {
        float positions[6][3];
        unsigned char colors[6][4];
    };

    static const AxesVertices kAxesVertices = {
        // positions
        {{0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 1}},
        // colors
        {{255, 0, 0, 255},
         {255, 0, 0, 255},
         {0, 255, 0, 255},
         {0, 255, 0, 255},
         {0, 0, 255, 255},
         {0, 0, 255, 255}},
    };

    VertexAttribs_[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
    VertexAttribs_[0].Size = 3;
    VertexAttribs_[0].Type = GL_FLOAT;
    VertexAttribs_[0].Normalized = false;
    VertexAttribs_[0].Stride = sizeof(kAxesVertices.positions[0]);
    VertexAttribs_[0].Pointer = (const GLvoid*)offsetof(AxesVertices, positions);

    VertexAttribs_[1].Index = VERTEX_ATTRIBUTE_LOCATION_COLOR;
    VertexAttribs_[1].Size = 4;
    VertexAttribs_[1].Type = GL_UNSIGNED_BYTE;
    VertexAttribs_[1].Normalized = true;
    VertexAttribs_[1].Stride = sizeof(kAxesVertices.colors[0]);
    VertexAttribs_[1].Pointer = (const GLvoid*)offsetof(AxesVertices, colors);

    GL(glGenBuffers(1, &VertexBuffer_));
    GL(glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer_));
    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(kAxesVertices), &kAxesVertices, GL_STATIC_DRAW));
    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));

    static const std::vector<unsigned short> indices = {
        0,
        1, // x axis - red
        2,
        3, // y axis - green
        4,
        5 // z axis - blue
    };
    CreateIndexBuffer(indices);

    IsRenderable_ = true;
}

void ovrGeometry::CreateStage() {
    static const float stageVertices[12] = {
        -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f};

    VertexAttribs_[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
    VertexAttribs_[0].Size = 3;
    VertexAttribs_[0].Type = GL_FLOAT;
    VertexAttribs_[0].Normalized = false;
    VertexAttribs_[0].Stride = 3 * sizeof(float);
    VertexAttribs_[0].Pointer = (const GLvoid*)0;

    GL(glGenBuffers(1, &VertexBuffer_));
    GL(glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer_));
    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(stageVertices), stageVertices, GL_STATIC_DRAW));
    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));

    static const std::vector<unsigned short> indices = {0, 1, 2, 2, 1, 3};
    CreateIndexBuffer(indices);

    IsRenderable_ = true;
}

void ovrGeometry::CreatePlane(const std::vector<XrVector3f>& vertices, const XrColor4f& color) {
    if (vertices.size() < 3) {
        IsRenderable_ = false;
        return;
    }

    struct PlaneVertex {
        XrVector3f position;
        XrColor4f color;
    };

    std::vector<PlaneVertex> planeVertices;
    planeVertices.reserve(vertices.size());
    for (const auto& vertex : vertices) {
        planeVertices.emplace_back(PlaneVertex{vertex, color});
    }
    VertexAttribs_[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
    VertexAttribs_[0].Size = 3;
    VertexAttribs_[0].Type = GL_FLOAT;
    VertexAttribs_[0].Normalized = false;
    VertexAttribs_[0].Stride = sizeof(PlaneVertex);
    VertexAttribs_[0].Pointer = (const GLvoid*)offsetof(PlaneVertex, position);

    VertexAttribs_[1].Index = VERTEX_ATTRIBUTE_LOCATION_COLOR;
    VertexAttribs_[1].Size = 4;
    VertexAttribs_[1].Type = GL_FLOAT;
    VertexAttribs_[1].Normalized = false;
    VertexAttribs_[1].Stride = sizeof(PlaneVertex);
    VertexAttribs_[1].Pointer = (const GLvoid*)offsetof(PlaneVertex, color);

    if (VertexBuffer_ == 0) {
        GL(glGenBuffers(1, &VertexBuffer_));
    }
    GL(glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer_));
    GL(glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(PlaneVertex) * planeVertices.size(),
        planeVertices.data(),
        GL_STATIC_DRAW));
    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    // Render only front side of the plane to make sure plane direction is correct.
    std::vector<unsigned short> indices;
    const int numTriangles = vertices.size() - 2;
    indices.reserve(numTriangles * 3);
    for (int i = 0; i < numTriangles; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
    }
    CreateIndexBuffer(indices);
    // VAO buffer needs to be created per each plane.
    CreateVAO();

    IsRenderable_ = true;
}

void ovrGeometry::CreateVolume(const std::array<XrVector3f, 8>& vertices, const XrColor4f& color) {
    struct VolumeVertices {
        XrVector3f positions[8];
        XrColor4f colors[8];
    };

    const VolumeVertices volumeVertices = {
        {vertices[0],
         vertices[1],
         vertices[2],
         vertices[3],
         vertices[4],
         vertices[5],
         vertices[6],
         vertices[7]},
        {color, color, color, color, color, color, color, color}};
    VertexAttribs_[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
    VertexAttribs_[0].Size = 3;
    VertexAttribs_[0].Type = GL_FLOAT;
    VertexAttribs_[0].Normalized = false;
    VertexAttribs_[0].Stride = sizeof(volumeVertices.positions[0]);
    VertexAttribs_[0].Pointer = (const GLvoid*)offsetof(VolumeVertices, positions);

    VertexAttribs_[1].Index = VERTEX_ATTRIBUTE_LOCATION_COLOR;
    VertexAttribs_[1].Size = 4;
    VertexAttribs_[1].Type = GL_FLOAT;
    VertexAttribs_[1].Normalized = false;
    VertexAttribs_[1].Stride = sizeof(volumeVertices.colors[0]);
    VertexAttribs_[1].Pointer = (const GLvoid*)offsetof(VolumeVertices, colors);

    GL(glGenBuffers(1, &VertexBuffer_));
    GL(glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer_));
    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(VolumeVertices), &volumeVertices, GL_STATIC_DRAW));
    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    const std::vector<unsigned short> indices = {
        0, 2, 1, 2, 0, 3, // bottom
        4, 6, 5, 6, 4, 7, // top
        0, 1, 4, 1, 5, 4, // front
        1, 2, 5, 2, 6, 5, // right
        2, 3, 6, 3, 7, 6, // back
        3, 0, 7, 0, 4, 7 // left
    };
    CreateIndexBuffer(indices);
    CreateVAO();

    IsRenderable_ = true;
}

void ovrGeometry::Destroy() {
    if (IndexBuffer_ != 0) {
        GL(glDeleteBuffers(1, &IndexBuffer_));
    }
    if (VertexBuffer_ != 0) {
        GL(glDeleteBuffers(1, &VertexBuffer_));
    }
    Clear();
}

void ovrGeometry::CreateVAO() {
    if (VertexArrayObject_ == 0) {
        GL(glGenVertexArrays(1, &VertexArrayObject_));
    }
    GL(glBindVertexArray(VertexArrayObject_));

    GL(glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer_));

    for (int i = 0; i < MAX_VERTEX_ATTRIB_POINTERS; i++) {
        if (VertexAttribs_[i].Index != -1) {
            GL(glEnableVertexAttribArray(VertexAttribs_[i].Index));
            GL(glVertexAttribPointer(
                VertexAttribs_[i].Index,
                VertexAttribs_[i].Size,
                VertexAttribs_[i].Type,
                VertexAttribs_[i].Normalized,
                VertexAttribs_[i].Stride,
                VertexAttribs_[i].Pointer));
        }
    }

    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer_));

    GL(glBindVertexArray(0));
}

void ovrGeometry::DestroyVAO() {
    GL(glDeleteVertexArrays(1, &VertexArrayObject_));
}

/*
================================================================================

ovrProgram

================================================================================
*/

struct ovrUniform {
    enum Index {
        MODEL_MATRIX,
        VIEW_ID,
        SCENE_MATRICES,
    };
    enum Type {
        VECTOR4,
        MATRIX4X4,
        INTEGER,
        BUFFER,
    };

    Index index;
    Type type;
    const char* name;
};

static ovrUniform ProgramUniforms[] = {
    {ovrUniform::Index::MODEL_MATRIX, ovrUniform::Type::MATRIX4X4, "ModelMatrix"},
    {ovrUniform::Index::VIEW_ID, ovrUniform::Type::INTEGER, "ViewID"},
    {ovrUniform::Index::SCENE_MATRICES, ovrUniform::Type::BUFFER, "SceneMatrices"},
};

void ovrProgram::Clear() {
    Program = 0;
    VertexShader = 0;
    FragmentShader = 0;
    memset(UniformLocation, 0, sizeof(UniformLocation));
    memset(UniformBinding, 0, sizeof(UniformBinding));
    memset(Textures, 0, sizeof(Textures));
}

static const char* programVersion = "#version 300 es\n";

bool ovrProgram::Create(const char* vertexSource, const char* fragmentSource) {
    GLint r;

    GL(VertexShader = glCreateShader(GL_VERTEX_SHADER));

    const char* vertexSources[3] = {programVersion, "", vertexSource};
    GL(glShaderSource(VertexShader, 3, vertexSources, 0));
    GL(glCompileShader(VertexShader));
    GL(glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &r));
    if (r == GL_FALSE) {
        GLchar msg[4096];
        GL(glGetShaderInfoLog(VertexShader, sizeof(msg), 0, msg));
        ALOGE("vertex shader compile failed");
        ALOGE("%s\n%s\n", vertexSource, msg);
        return false;
    }

    const char* fragmentSources[2] = {programVersion, fragmentSource};
    GL(FragmentShader = glCreateShader(GL_FRAGMENT_SHADER));
    GL(glShaderSource(FragmentShader, 2, fragmentSources, 0));
    GL(glCompileShader(FragmentShader));
    GL(glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &r));
    if (r == GL_FALSE) {
        GLchar msg[4096];
        GL(glGetShaderInfoLog(FragmentShader, sizeof(msg), 0, msg));
        ALOGE("fragment shader compile failed");
        ALOGE("%s\n%s\n", fragmentSource, msg);
        return false;
    }

    GL(Program = glCreateProgram());
    GL(glAttachShader(Program, VertexShader));
    GL(glAttachShader(Program, FragmentShader));

    // Bind the vertex attribute locations.
    for (size_t i = 0; i < sizeof(ProgramVertexAttributes) / sizeof(ProgramVertexAttributes[0]);
         i++) {
        GL(glBindAttribLocation(
            Program, ProgramVertexAttributes[i].location, ProgramVertexAttributes[i].name));
    }

    GL(glLinkProgram(Program));
    GL(glGetProgramiv(Program, GL_LINK_STATUS, &r));
    if (r == GL_FALSE) {
        GLchar msg[4096];
        GL(glGetProgramInfoLog(Program, sizeof(msg), 0, msg));
        ALOGE("Linking program failed: %s\n", msg);
        return false;
    }

    int numBufferBindings = 0;

    memset(UniformLocation, -1, sizeof(UniformLocation));
    for (size_t i = 0; i < sizeof(ProgramUniforms) / sizeof(ProgramUniforms[0]); i++) {
        const int uniformIndex = ProgramUniforms[i].index;
        if (ProgramUniforms[i].type == ovrUniform::Type::BUFFER) {
            GL(UniformLocation[uniformIndex] =
                   glGetUniformBlockIndex(Program, ProgramUniforms[i].name));
            UniformBinding[uniformIndex] = numBufferBindings++;
            GL(glUniformBlockBinding(
                Program, UniformLocation[uniformIndex], UniformBinding[uniformIndex]));
        } else {
            GL(UniformLocation[uniformIndex] =
                   glGetUniformLocation(Program, ProgramUniforms[i].name));
            UniformBinding[uniformIndex] = UniformLocation[uniformIndex];
        }
    }

    GL(glUseProgram(Program));

    // Get the texture locations.
    for (int i = 0; i < MAX_PROGRAM_TEXTURES; i++) {
        char name[32];
        sprintf(name, "Texture%i", i);
        Textures[i] = glGetUniformLocation(Program, name);
        if (Textures[i] != -1) {
            GL(glUniform1i(Textures[i], i));
        }
    }

    GL(glUseProgram(0));

    return true;
}

void ovrProgram::Destroy() {
    if (Program != 0) {
        GL(glDeleteProgram(Program));
        Program = 0;
    }
    if (VertexShader != 0) {
        GL(glDeleteShader(VertexShader));
        VertexShader = 0;
    }
    if (FragmentShader != 0) {
        GL(glDeleteShader(FragmentShader));
        FragmentShader = 0;
    }
}

/*
================================================================================

ovrFramebuffer

================================================================================
*/

void ovrFramebuffer::Clear() {
    Width = 0;
    Height = 0;
    Multisamples = 0;
    SwapChainLength = 0;
    Elements = nullptr;
}

bool ovrFramebuffer::Create(
    const GLenum colorFormat,
    const int width,
    const int height,
    const int multisamples,
    const int swapChainLength,
    GLuint* colorTextures) {
    PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR =
        (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)eglGetProcAddress(
            "glFramebufferTextureMultiviewOVR");
    PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC glFramebufferTextureMultisampleMultiviewOVR =
        (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)eglGetProcAddress(
            "glFramebufferTextureMultisampleMultiviewOVR");

    Width = width;
    Height = height;
    Multisamples = multisamples;
    SwapChainLength = swapChainLength;

    Elements = new Element[SwapChainLength];

    for (int i = 0; i < SwapChainLength; i++) {
        Element& el = Elements[i];
        // Create the color buffer texture.
        el.ColorTexture = colorTextures[i];
        GLenum colorTextureTarget = GL_TEXTURE_2D_ARRAY;
        GL(glBindTexture(colorTextureTarget, el.ColorTexture));
        GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
        GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
        GLfloat borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
        GL(glTexParameterfv(colorTextureTarget, GL_TEXTURE_BORDER_COLOR, borderColor));
        GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL(glBindTexture(colorTextureTarget, 0));

        // Create the depth buffer texture.
        GL(glGenTextures(1, &el.DepthTexture));
        GL(glBindTexture(GL_TEXTURE_2D_ARRAY, el.DepthTexture));
        GL(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, width, height, 2));
        GL(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));

        // Create the frame buffer.
        GL(glGenFramebuffers(1, &el.FrameBufferObject));
        GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, el.FrameBufferObject));
        if (multisamples > 1 && (glFramebufferTextureMultisampleMultiviewOVR != nullptr)) {
            GL(glFramebufferTextureMultisampleMultiviewOVR(
                GL_DRAW_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT,
                el.DepthTexture,
                0 /* level */,
                multisamples /* samples */,
                0 /* baseViewIndex */,
                2 /* numViews */));
            GL(glFramebufferTextureMultisampleMultiviewOVR(
                GL_DRAW_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                el.ColorTexture,
                0 /* level */,
                multisamples /* samples */,
                0 /* baseViewIndex */,
                2 /* numViews */));
        } else {
            GL(glFramebufferTextureMultiviewOVR(
                GL_DRAW_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT,
                el.DepthTexture,
                0 /* level */,
                0 /* baseViewIndex */,
                2 /* numViews */));
            GL(glFramebufferTextureMultiviewOVR(
                GL_DRAW_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                el.ColorTexture,
                0 /* level */,
                0 /* baseViewIndex */,
                2 /* numViews */));
        }

        GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
        GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
        if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
            ALOGE(
                "Incomplete frame buffer object: %s",
                GlFrameBufferStatusString(renderFramebufferStatus));
            return false;
        }
    }

    return true;
}

void ovrFramebuffer::Destroy() {
    for (int i = 0; i < SwapChainLength; i++) {
        Element& el = Elements[i];
        GL(glDeleteFramebuffers(1, &el.FrameBufferObject));
        GL(glDeleteTextures(1, &el.DepthTexture));
    }
    delete[] Elements;
    Clear();
}

void ovrFramebuffer::Bind(int element) {
    GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Elements[element].FrameBufferObject));
}

void ovrFramebuffer::Unbind() {
    GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
}

void ovrFramebuffer::Resolve() {
    // Discard the depth buffer, so the tiler won't need to write it back out to memory.
    const GLenum depthAttachment[1] = {GL_DEPTH_ATTACHMENT};
    glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, depthAttachment);

    // We now let the resolve happen implicitly.
}

/*
================================================================================

ovrPlane

================================================================================
*/

ovrPlane::ovrPlane(const XrSpace space) : Space(space) {}

void ovrPlane::Update(const XrRect2Df& boundingBox2D, const XrColor4f& color) {
    const auto& offset = boundingBox2D.offset;
    const auto& extent = boundingBox2D.extent;
    const std::vector<XrVector3f> vertices = {
        XrVector3f{offset.x, offset.y, 0.0f},
        XrVector3f{offset.x + extent.width, offset.y, 0.0f},
        XrVector3f{offset.x + extent.width, offset.y + extent.height, 0.0f},
        XrVector3f{offset.x, offset.y + extent.height, 0.0f}};
    Geometry.CreatePlane(vertices, color);
}

void ovrPlane::Update(const XrBoundary2DFB& boundary2D, const XrColor4f& color) {
    std::vector<XrVector3f> vertices;
    vertices.reserve(boundary2D.vertexCountOutput);
    for (uint32_t i = 0; i < boundary2D.vertexCountOutput; ++i) {
        vertices.push_back(XrVector3f{boundary2D.vertices[i].x, boundary2D.vertices[i].y, 0.0f});
    }
    Geometry.CreatePlane(vertices, color);
}

void ovrPlane::SetZOffset(const float zOffset) {
    ZOffset = zOffset;
}

void ovrPlane::SetPose(const XrPosef& T_World_Plane_Xr) {
    T_World_Plane = FromXrPosef(T_World_Plane_Xr);
    IsPoseSet_ = true;
}

/*
================================================================================

ovrVolume

================================================================================
*/

ovrVolume::ovrVolume(const XrSpace space) : Space(space){};

void ovrVolume::Update(const XrRect3DfFB& boundingBox3D, const XrColor4f& color) {
    const auto& offset = boundingBox3D.offset;
    const auto& extent = boundingBox3D.extent;
    const std::array<XrVector3f, 8> vertices = {
        XrVector3f{offset.x, offset.y, offset.z},
        XrVector3f{offset.x + extent.width, offset.y, offset.z},
        XrVector3f{offset.x + extent.width, offset.y + extent.height, offset.z},
        XrVector3f{offset.x, offset.y + extent.height, offset.z},
        XrVector3f{offset.x, offset.y, offset.z + extent.depth},
        XrVector3f{offset.x + extent.width, offset.y, offset.z + extent.depth},
        XrVector3f{offset.x + extent.width, offset.y + extent.height, offset.z + extent.depth},
        XrVector3f{offset.x, offset.y + extent.height, offset.z + extent.depth}};
    Geometry.CreateVolume(vertices, color);
}

void ovrVolume::SetPose(const XrPosef& T_World_Volume_Xr) {
    T_World_Volume = FromXrPosef(T_World_Volume_Xr);
    IsPoseSet_ = true;
}

/*
================================================================================

ovrScene

================================================================================
*/

void ovrScene::SetClearColor(const float* c) {
    for (int i = 0; i < 4; i++) {
        ClearColor[i] = c[i];
    }
}

void ovrScene::Clear() {
    CreatedScene = false;
    CreatedVAOs = false;
    SceneMatrices = 0;

    StageProgram.Clear();
    Stage.Clear();
    AxesProgram.Clear();
    Axes.Clear();
    PlaneProgram.Clear();
    for (auto& plane : Planes) {
        plane.Geometry.Clear();
    }
    Planes.clear();

    VolumeProgram.Clear();
    for (auto& volume : Volumes) {
        volume.Geometry.Clear();
    }
    Volumes.clear();
}

bool ovrScene::IsCreated() {
    return CreatedScene;
}

void ovrScene::CreateVAOs() {
    if (!CreatedVAOs) {
        // Stage
        Stage.CreateVAO();
        // Axes
        Axes.CreateVAO();

        CreatedVAOs = true;
    }
}

void ovrScene::DestroyVAOs() {
    if (CreatedVAOs) {
        Stage.DestroyVAO();
        Axes.DestroyVAO();

        CreatedVAOs = false;
    }
}

void ovrScene::Create() {
    // Setup the scene matrices.
    GL(glGenBuffers(1, &SceneMatrices));
    GL(glBindBuffer(GL_UNIFORM_BUFFER, SceneMatrices));
    GL(glBufferData(
        GL_UNIFORM_BUFFER,
        2 * sizeof(Matrix4f) /* 2 view matrices */ +
            2 * sizeof(Matrix4f) /* 2 projection matrices */,
        nullptr,
        GL_STATIC_DRAW));
    GL(glBindBuffer(GL_UNIFORM_BUFFER, 0));

    // Stage
    if (!StageProgram.Create(STAGE_VERTEX_SHADER, STAGE_FRAGMENT_SHADER)) {
        ALOGE("Failed to compile stage program");
    }
    Stage.CreateStage();

    // Axes
    if (!AxesProgram.Create(VERTEX_SHADER, FRAGMENT_SHADER)) {
        ALOGE("Failed to compile axes program");
    }
    Axes.CreateAxes();

    // Planes
    if (!PlaneProgram.Create(VERTEX_SHADER, FRAGMENT_SHADER)) {
        ALOGE("Failed to compile plane program!");
    }

    // Volumes
    if (!VolumeProgram.Create(VERTEX_SHADER, FRAGMENT_SHADER)) {
        ALOGE("Failed to compile volume program!");
    }

    CreatedScene = true;

    CreateVAOs();
    float c[] = {0.0, 0.0, 0.0, 1.0};
    SetClearColor(c);
}

void ovrScene::Destroy() {
    DestroyVAOs();

    GL(glDeleteBuffers(1, &SceneMatrices));
    StageProgram.Destroy();
    Stage.Destroy();
    AxesProgram.Destroy();
    Axes.Destroy();

    PlaneProgram.Destroy();
    for (auto& plane : Planes) {
        plane.Geometry.DestroyVAO();
        plane.Geometry.Destroy();
    }
    Planes.clear();

    VolumeProgram.Destroy();
    for (auto& volume : Volumes) {
        volume.Geometry.DestroyVAO();
        volume.Geometry.Destroy();
    }
    Volumes.clear();
}

/*
================================================================================

ovrAppRenderer

================================================================================
*/

void ovrAppRenderer::Clear() {
    Framebuffer.Clear();
    Scene.Clear();
}

void ovrAppRenderer::Create(
    GLenum format,
    int width,
    int height,
    int numMultiSamples,
    int swapChainLength,
    GLuint* colorTextures) {
    EglInitExtensions();
    Framebuffer.Create(format, width, height, numMultiSamples, swapChainLength, colorTextures);
    if (glExtensions.EXT_sRGB_write_control) {
        // This app was originally written with the presumption that
        // its swapchains and compositor front buffer were RGB.
        // In order to have the colors the same now that its compositing
        // to an sRGB front buffer, we have to write to an sRGB swapchain
        // but with the linear->sRGB conversion disabled on write.
        GL(glDisable(GL_FRAMEBUFFER_SRGB_EXT));
    }
}

void ovrAppRenderer::Destroy() {
    Framebuffer.Destroy();
}

void ovrAppRenderer::RenderFrame(const FrameIn& frameIn) {
    // Update the scene matrices.
    GL(glBindBuffer(GL_UNIFORM_BUFFER, Scene.SceneMatrices));
    GL(Matrix4f* sceneMatrices = (Matrix4f*)glMapBufferRange(
           GL_UNIFORM_BUFFER,
           0,
           4 * sizeof(Matrix4f) /* 2 view + 2 proj matrices */,
           GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

    if (sceneMatrices != nullptr) {
        memcpy((char*)sceneMatrices, &frameIn.View, 4 * sizeof(Matrix4f));
    }

    GL(glUnmapBuffer(GL_UNIFORM_BUFFER));
    GL(glBindBuffer(GL_UNIFORM_BUFFER, 0));

    // Render the eye images.
    Framebuffer.Bind(frameIn.SwapChainIndex);

    GL(glEnable(GL_SCISSOR_TEST));
    GL(glDepthMask(GL_TRUE));
    GL(glEnable(GL_DEPTH_TEST));
    GL(glDepthFunc(GL_LEQUAL));
    GL(glEnable(GL_CULL_FACE));
    GL(glCullFace(GL_BACK));
    GL(glDisable(GL_BLEND));
    GL(glViewport(0, 0, Framebuffer.Width, Framebuffer.Height));
    GL(glScissor(0, 0, Framebuffer.Width, Framebuffer.Height));
    GL(glClearColor(
        Scene.ClearColor[0], Scene.ClearColor[1], Scene.ClearColor[2], Scene.ClearColor[3]));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    GL(glLineWidth(3.0));
    // "tracking space" axes (could be LOCAL or LOCAL_FLOOR)
    GL(glUseProgram(Scene.AxesProgram.Program));
    GL(glBindBufferBase(
        GL_UNIFORM_BUFFER,
        Scene.AxesProgram.UniformBinding[ovrUniform::Index::SCENE_MATRICES],
        Scene.SceneMatrices));
    if (Scene.AxesProgram.UniformLocation[ovrUniform::Index::VIEW_ID] >=
        0) // NOTE: will not be present when multiview path is enabled.
    {
        GL(glUniform1i(Scene.AxesProgram.UniformLocation[ovrUniform::Index::VIEW_ID], 0));
    }
    if (Scene.AxesProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX] >= 0) {
        const Matrix4f scale = Matrix4f::Scaling(0.1, 0.1, 0.1);
        GL(glUniformMatrix4fv(
            Scene.AxesProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX],
            1,
            GL_TRUE,
            &scale.M[0][0]));
    }
    Scene.Axes.BindVAO();
    GL(glDrawElements(GL_LINES, Scene.Axes.IndexCount(), GL_UNSIGNED_SHORT, nullptr));
    GL(glBindVertexArray(0));
    GL(glUseProgram(0));

    if (frameIn.HasStage) {
        // stage axes
        GL(glUseProgram(Scene.AxesProgram.Program));
        GL(glBindBufferBase(
            GL_UNIFORM_BUFFER,
            Scene.AxesProgram.UniformBinding[ovrUniform::Index::SCENE_MATRICES],
            Scene.SceneMatrices));
        if (Scene.AxesProgram.UniformLocation[ovrUniform::Index::VIEW_ID] >=
            0) // NOTE: will not be present when multiview path is enabled.
        {
            GL(glUniform1i(Scene.AxesProgram.UniformLocation[ovrUniform::Index::VIEW_ID], 0));
        }
        if (Scene.AxesProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX] >= 0) {
            const Matrix4f scale = Matrix4f::Scaling(0.5, 0.5, 0.5);
            const Matrix4f stagePoseMat = Matrix4f(frameIn.StagePose);
            const Matrix4f m1 = stagePoseMat * scale;
            GL(glUniformMatrix4fv(
                Scene.AxesProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX],
                1,
                GL_TRUE,
                &m1.M[0][0]));
        }
        Scene.Axes.BindVAO();
        GL(glDrawElements(GL_LINES, Scene.Axes.IndexCount(), GL_UNSIGNED_SHORT, nullptr));
        GL(glBindVertexArray(0));
        GL(glUseProgram(0));
    }

    if (frameIn.HasStage) {
        // Stage
        GL(glUseProgram(Scene.StageProgram.Program));
        GL(glBindBufferBase(
            GL_UNIFORM_BUFFER,
            Scene.StageProgram.UniformBinding[ovrUniform::Index::SCENE_MATRICES],
            Scene.SceneMatrices));
        if (Scene.StageProgram.UniformLocation[ovrUniform::Index::VIEW_ID] >=
            0) // NOTE: will not be present when multiview path is enabled.
        {
            GL(glUniform1i(Scene.StageProgram.UniformLocation[ovrUniform::Index::VIEW_ID], 0));
        }
        if (Scene.StageProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX] >= 0) {
            const Matrix4f rotateVtoH = Matrix4f::RotationX(-M_PI / 2.0f);
            const Matrix4f stageScaleMat = Matrix4f::Scaling(frameIn.StageScale);
            const Matrix4f stagePoseMat = Matrix4f(frameIn.StagePose);
            const Matrix4f m2 = stagePoseMat * stageScaleMat * rotateVtoH;
            GL(glUniformMatrix4fv(
                Scene.StageProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX],
                1,
                GL_TRUE,
                &m2.M[0][0]));
        }
        GL(glDepthMask(GL_FALSE));
        GL(glEnable(GL_DEPTH_TEST));
        GL(glDepthFunc(GL_LEQUAL));
        GL(glDisable(GL_CULL_FACE));
        GL(glEnable(GL_BLEND));
        GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        Scene.Stage.BindVAO();
        GL(glDrawElements(GL_TRIANGLES, Scene.Stage.IndexCount(), GL_UNSIGNED_SHORT, nullptr));
        GL(glDepthMask(GL_TRUE));
        GL(glDisable(GL_BLEND));
        GL(glBindVertexArray(0));
        GL(glUseProgram(0));

        Framebuffer.Resolve();
    }

    // Render controllers, reusing axes shaders.
    for (int i = 0; i < 2; ++i) {
        if (frameIn.RenderController[i]) {
            GL(glLineWidth(5.0));
            GL(glUseProgram(Scene.AxesProgram.Program));
            GL(glBindBufferBase(
                GL_UNIFORM_BUFFER,
                Scene.AxesProgram.UniformBinding[ovrUniform::Index::SCENE_MATRICES],
                Scene.SceneMatrices));
            if (Scene.AxesProgram.UniformLocation[ovrUniform::Index::VIEW_ID] >= 0) {
                GL(glUniform1i(Scene.AxesProgram.UniformLocation[ovrUniform::Index::VIEW_ID], 0));
            }
            if (Scene.AxesProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX] >= 0) {
                const Matrix4f transform =
                    Matrix4f(frameIn.ControllerPoses[i]) * Matrix4f::Scaling(0.1, 0.1, 0.1);
                GL(glUniformMatrix4fv(
                    Scene.AxesProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX],
                    1,
                    GL_TRUE,
                    &transform.M[0][0]));
            }
            Scene.Axes.BindVAO();
            GL(glDrawElements(GL_LINES, Scene.Axes.IndexCount(), GL_UNSIGNED_SHORT, nullptr));
            GL(glBindVertexArray(0));
            GL(glUseProgram(0));
        }
    }

    // Render planes
    GL(glUseProgram(Scene.PlaneProgram.Program));
    GL(glBindBufferBase(
        GL_UNIFORM_BUFFER,
        Scene.PlaneProgram.UniformBinding[ovrUniform::Index::SCENE_MATRICES],
        Scene.SceneMatrices));
    if (Scene.PlaneProgram.UniformLocation[ovrUniform::Index::VIEW_ID] >= 0) {
        // NOTE: will not be present when multiview path is enabled.
        GL(glUniform1i(Scene.PlaneProgram.UniformLocation[ovrUniform::Index::VIEW_ID], 0));
    }
    GL(glEnable(GL_BLEND));
    GL(glEnable(GL_CULL_FACE));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    for (const auto& plane : Scene.Planes) {
        if (!plane.IsRenderable()) {
            continue;
        }
        if (Scene.PlaneProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX] >= 0) {
            Matrix4f transform = Matrix4f(plane.T_World_Plane);
            if (plane.ZOffset != 0.0f) {
                transform *= ZOffsetTransform(plane.ZOffset);
            }
            GL(glUniformMatrix4fv(
                Scene.PlaneProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX],
                1,
                GL_TRUE,
                &transform.M[0][0]));
        }
        plane.Geometry.BindVAO();
        GL(glDrawElements(GL_TRIANGLES, plane.Geometry.IndexCount(), GL_UNSIGNED_SHORT, nullptr));
        GL(glBindVertexArray(0));
    }
    GL(glDisable(GL_CULL_FACE));
    GL(glDisable(GL_BLEND));
    GL(glUseProgram(0));

    // Render plane pose as RGB axes if available.
    GL(glLineWidth(5.0));
    GL(glUseProgram(Scene.AxesProgram.Program));
    GL(glBindBufferBase(
        GL_UNIFORM_BUFFER,
        Scene.AxesProgram.UniformBinding[ovrUniform::Index::SCENE_MATRICES],
        Scene.SceneMatrices));
    if (Scene.AxesProgram.UniformLocation[ovrUniform::Index::VIEW_ID] >= 0) {
        GL(glUniform1i(Scene.AxesProgram.UniformLocation[ovrUniform::Index::VIEW_ID], 0));
    }
    Scene.Axes.BindVAO();
    for (const auto& plane : Scene.Planes) {
        if (!plane.IsRenderable()) {
            continue;
        }
        if (Scene.AxesProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX] >= 0) {
            Matrix4f transform = Matrix4f(plane.T_World_Plane);
            if (plane.ZOffset != 0.0f) {
                transform *= ZOffsetTransform(plane.ZOffset);
            }
            transform *= Matrix4f::Scaling(0.1, 0.1, 0.1);
            GL(glUniformMatrix4fv(
                Scene.AxesProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX],
                1,
                GL_TRUE,
                &transform.M[0][0]));
            GL(glDrawElements(GL_LINES, Scene.Axes.IndexCount(), GL_UNSIGNED_SHORT, nullptr));
        }
    }
    GL(glBindVertexArray(0));
    GL(glUseProgram(0));

    // Render volumes
    GL(glUseProgram(Scene.VolumeProgram.Program));
    GL(glBindBufferBase(
        GL_UNIFORM_BUFFER,
        Scene.VolumeProgram.UniformBinding[ovrUniform::Index::SCENE_MATRICES],
        Scene.SceneMatrices));
    if (Scene.VolumeProgram.UniformLocation[ovrUniform::Index::VIEW_ID] >= 0) {
        // NOTE: will not be present when multiview path is enabled.
        GL(glUniform1i(Scene.VolumeProgram.UniformLocation[ovrUniform::Index::VIEW_ID], 0));
    }
    GL(glEnable(GL_BLEND));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    for (const auto& volume : Scene.Volumes) {
        if (!volume.IsRenderable()) {
            continue;
        }
        if (Scene.VolumeProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX] >= 0) {
            const Matrix4f transform = Matrix4f(volume.T_World_Volume);
            GL(glUniformMatrix4fv(
                Scene.VolumeProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX],
                1,
                GL_TRUE,
                &transform.M[0][0]));
        }
        volume.Geometry.BindVAO();
        GL(glDrawElements(GL_TRIANGLES, volume.Geometry.IndexCount(), GL_UNSIGNED_SHORT, nullptr));
        GL(glBindVertexArray(0));
    }
    GL(glDisable(GL_BLEND));
    GL(glUseProgram(0));

    // Render the underlying anchor pose as RGB axes if available.
    GL(glLineWidth(5.0));
    GL(glUseProgram(Scene.AxesProgram.Program));
    GL(glBindBufferBase(
        GL_UNIFORM_BUFFER,
        Scene.AxesProgram.UniformBinding[ovrUniform::Index::SCENE_MATRICES],
        Scene.SceneMatrices));
    if (Scene.AxesProgram.UniformLocation[ovrUniform::Index::VIEW_ID] >= 0) {
        GL(glUniform1i(Scene.AxesProgram.UniformLocation[ovrUniform::Index::VIEW_ID], 0));
    }
    Scene.Axes.BindVAO();
    for (const auto& volume : Scene.Volumes) {
        if (!volume.IsRenderable()) {
            continue;
        }
        if (Scene.AxesProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX] >= 0) {
            const Matrix4f transform =
                Matrix4f(volume.T_World_Volume) * Matrix4f::Scaling(0.1, 0.1, 0.1);
            GL(glUniformMatrix4fv(
                Scene.AxesProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX],
                1,
                GL_TRUE,
                &transform.M[0][0]));
            GL(glDrawElements(GL_LINES, Scene.Axes.IndexCount(), GL_UNSIGNED_SHORT, nullptr));
        }
    }
    GL(glBindVertexArray(0));
    GL(glUseProgram(0));

    Framebuffer.Unbind();
}
