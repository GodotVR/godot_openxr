/************************************************************************************

Filename	:	SpatialAnchor.cpp
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

#if defined(ANDROID)
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h> // for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/native_window_jni.h> // for native window JNI
#include <android/input.h>
#endif

#include <atomic>
#include <thread>

#if defined(ANDROID)
#include <sys/system_properties.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#endif

#include "SpatialAnchorGl.h"

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

#if defined(ANDROID)
#define OVR_LOG_TAG "SpatialAnchorGl"

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, OVR_LOG_TAG, __VA_ARGS__)
#if DEBUG
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, OVR_LOG_TAG, __VA_ARGS__)
#else
#define ALOGV(...)
#endif

#else
#define ALOGE(...)       \
    printf("ERROR: ");   \
    printf(__VA_ARGS__); \
    printf("\n")
#define ALOGV(...)       \
    printf("VERBOSE: "); \
    printf(__VA_ARGS__); \
    printf("\n")
#endif // defined(ANDROID)

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
    VertexBuffer = 0;
    IndexBuffer = 0;
    VertexArrayObject = 0;
    VertexCount = 0;
    IndexCount = 0;
    for (int i = 0; i < MAX_VERTEX_ATTRIB_POINTERS; i++) {
        memset(&VertexAttribs[i], 0, sizeof(VertexAttribs[i]));
        VertexAttribs[i].Index = -1;
    }
}

void ovrGeometry::CreateCube() {
    struct ovrCubeVertices {
        signed char positions[8][4];
        unsigned char colors[8][4];
    };

    static const ovrCubeVertices cubeVertices = {// positions
                                                 {{-127, -127, -127, +127},
                                                  {+127, -127, -127, +127},
                                                  {-127, +127, -127, +127},
                                                  {+127, +127, -127, +127},
                                                  {-127, -127, +127, +127},
                                                  {+127, -127, +127, +127},
                                                  {-127, +127, +127, +127},
                                                  {+127, +127, +127, +127}},
                                                 // colors
                                                 {{0x00, 0x00, 0x00, 0xff},
                                                  {0xff, 0x00, 0x00, 0xff},
                                                  {0x00, 0xff, 0x00, 0xff},
                                                  {0xff, 0xff, 0x00, 0xff},
                                                  {0x00, 0x00, 0xff, 0xff},
                                                  {0xff, 0x00, 0xff, 0xff},
                                                  {0x00, 0xff, 0xff, 0xff},
                                                  {0xff, 0xff, 0xff, 0xff}}};

    static const unsigned short cubeIndices[36] = {
        0, 2, 1, 2, 3, 1, // back
        4, 5, 6, 6, 5, 7, // front
        6, 7, 2, 2, 7, 3, // top
        4, 0, 5, 5, 0, 1, // bottom
        0, 4, 2, 2, 4, 6, // left
        5, 1, 7, 7, 1, 3 // right
    };

    VertexCount = 8;
    IndexCount = 36;

    VertexAttribs[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
    VertexAttribs[0].Size = 4;
    VertexAttribs[0].Type = GL_BYTE;
    VertexAttribs[0].Normalized = true;
    VertexAttribs[0].Stride = sizeof(cubeVertices.positions[0]);
    VertexAttribs[0].Pointer = (const GLvoid*)offsetof(ovrCubeVertices, positions);

    VertexAttribs[1].Index = VERTEX_ATTRIBUTE_LOCATION_COLOR;
    VertexAttribs[1].Size = 4;
    VertexAttribs[1].Type = GL_UNSIGNED_BYTE;
    VertexAttribs[1].Normalized = true;
    VertexAttribs[1].Stride = sizeof(cubeVertices.colors[0]);
    VertexAttribs[1].Pointer = (const GLvoid*)offsetof(ovrCubeVertices, colors);

    GL(glGenBuffers(1, &VertexBuffer));
    GL(glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer));
    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW));
    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));

    GL(glGenBuffers(1, &IndexBuffer));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer));
    GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void ovrGeometry::CreateAxes() {
    struct ovrAxesVertices {
        float positions[6][3];
        unsigned char colors[6][4];
    };

    static const ovrAxesVertices axesVertices = {
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

    static const unsigned short axesIndices[6] = {
        0,
        1, // x axis - red
        2,
        3, // y axis - green
        4,
        5 // z axis - blue
    };

    VertexCount = 6;
    IndexCount = 6;

    VertexAttribs[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
    VertexAttribs[0].Size = 3;
    VertexAttribs[0].Type = GL_FLOAT;
    VertexAttribs[0].Normalized = false;
    VertexAttribs[0].Stride = sizeof(axesVertices.positions[0]);
    VertexAttribs[0].Pointer = (const GLvoid*)offsetof(ovrAxesVertices, positions);

    VertexAttribs[1].Index = VERTEX_ATTRIBUTE_LOCATION_COLOR;
    VertexAttribs[1].Size = 4;
    VertexAttribs[1].Type = GL_UNSIGNED_BYTE;
    VertexAttribs[1].Normalized = true;
    VertexAttribs[1].Stride = sizeof(axesVertices.colors[0]);
    VertexAttribs[1].Pointer = (const GLvoid*)offsetof(ovrAxesVertices, colors);

    GL(glGenBuffers(1, &VertexBuffer));
    GL(glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer));
    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(axesVertices), &axesVertices, GL_STATIC_DRAW));
    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));

    GL(glGenBuffers(1, &IndexBuffer));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer));
    GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(axesIndices), axesIndices, GL_STATIC_DRAW));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void ovrGeometry::CreateStage() {
    static const float stageVertices[12] = {
        -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f};

    static const unsigned short stageIndices[6] = {0, 1, 2, 2, 1, 3};

    VertexCount = 4;
    IndexCount = 6;

    VertexAttribs[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
    VertexAttribs[0].Size = 3;
    VertexAttribs[0].Type = GL_FLOAT;
    VertexAttribs[0].Normalized = false;
    VertexAttribs[0].Stride = 3 * sizeof(float);
    VertexAttribs[0].Pointer = (const GLvoid*)0;

    GL(glGenBuffers(1, &VertexBuffer));
    GL(glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer));
    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(stageVertices), stageVertices, GL_STATIC_DRAW));
    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));

    GL(glGenBuffers(1, &IndexBuffer));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer));
    GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(stageIndices), stageIndices, GL_STATIC_DRAW));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void ovrGeometry::Destroy() {
    GL(glDeleteBuffers(1, &IndexBuffer));
    GL(glDeleteBuffers(1, &VertexBuffer));

    Clear();
}

void ovrGeometry::CreateVAO() {
    GL(glGenVertexArrays(1, &VertexArrayObject));
    GL(glBindVertexArray(VertexArrayObject));

    GL(glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer));

    for (int i = 0; i < MAX_VERTEX_ATTRIB_POINTERS; i++) {
        if (VertexAttribs[i].Index != -1) {
            GL(glEnableVertexAttribArray(VertexAttribs[i].Index));
            GL(glVertexAttribPointer(
                VertexAttribs[i].Index,
                VertexAttribs[i].Size,
                VertexAttribs[i].Type,
                VertexAttribs[i].Normalized,
                VertexAttribs[i].Stride,
                VertexAttribs[i].Pointer));
        }
    }

    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer));

    GL(glBindVertexArray(0));
}

void ovrGeometry::DestroyVAO() {
    GL(glDeleteVertexArrays(1, &VertexArrayObject));
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
        COLOR_SCALE,
        COLOR_BIAS,
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
    {ovrUniform::Index::COLOR_SCALE, ovrUniform::Type::VECTOR4, "ColorScale"},
    {ovrUniform::Index::COLOR_BIAS, ovrUniform::Type::VECTOR4, "ColorBias"},
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

static const char CUBE_VERTEX_SHADER[] =
    "#define NUM_VIEWS 2\n"
    "#define VIEW_ID gl_ViewID_OVR\n"
    "#extension GL_OVR_multiview2 : require\n"
    "layout(num_views=NUM_VIEWS) in;\n"
    "in vec3 vertexPosition;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 ModelMatrix;\n"
    "uniform vec4 ColorScale;\n"
    "uniform vec4 ColorBias;\n"
    "uniform SceneMatrices\n"
    "{\n"
    "	uniform mat4 ViewMatrix[NUM_VIEWS];\n"
    "	uniform mat4 ProjectionMatrix[NUM_VIEWS];\n"
    "} sm;\n"
    "out vec4 fragmentColor;\n"
    "void main()\n"
    "{\n"
    "	gl_Position = sm.ProjectionMatrix[VIEW_ID] * ( sm.ViewMatrix[VIEW_ID] * ( ModelMatrix * vec4( vertexPosition, 1.0 ) ) );\n"
    "	fragmentColor = vertexColor * ColorScale + ColorBias;\n"
    "}\n";

static const char CUBE_FRAGMENT_SHADER[] =
    "in lowp vec4 fragmentColor;\n"
    "out lowp vec4 outColor;\n"
    "void main()\n"
    "{\n"
    "	outColor = fragmentColor;\n"
    "}\n";

static const char STAGE_VERTEX_SHADER[] =
    "#define NUM_VIEWS 2\n"
    "#define VIEW_ID gl_ViewID_OVR\n"
    "#extension GL_OVR_multiview2 : require\n"
    "layout(num_views=NUM_VIEWS) in;\n"
    "in vec3 vertexPosition;\n"
    "uniform mat4 ModelMatrix;\n"
    "uniform SceneMatrices\n"
    "{\n"
    "	uniform mat4 ViewMatrix[NUM_VIEWS];\n"
    "	uniform mat4 ProjectionMatrix[NUM_VIEWS];\n"
    "} sm;\n"
    "void main()\n"
    "{\n"
    "	gl_Position = sm.ProjectionMatrix[VIEW_ID] * ( sm.ViewMatrix[VIEW_ID] * ( ModelMatrix * ( vec4( vertexPosition, 1.0 ) ) ) );\n"
    "}\n";

static const char STAGE_FRAGMENT_SHADER[] =
    "out lowp vec4 outColor;\n"
    "void main()\n"
    "{\n"
    "	outColor = vec4( 0.5, 0.5, 1.0, 0.5 );\n"
    "}\n";

static const char AXES_VERTEX_SHADER[] =
    "#define NUM_VIEWS 2\n"
    "#define VIEW_ID gl_ViewID_OVR\n"
    "#extension GL_OVR_multiview2 : require\n"
    "layout(num_views=NUM_VIEWS) in;\n"
    "in vec3 vertexPosition;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 ModelMatrix;\n"
    "uniform SceneMatrices\n"
    "{\n"
    "	uniform mat4 ViewMatrix[NUM_VIEWS];\n"
    "	uniform mat4 ProjectionMatrix[NUM_VIEWS];\n"
    "} sm;\n"
    "out vec4 fragmentColor;\n"
    "void main()\n"
    "{\n"
    "	gl_Position = sm.ProjectionMatrix[VIEW_ID] * ( sm.ViewMatrix[VIEW_ID] * ( ModelMatrix * ( vec4( vertexPosition, 1.0 ) ) ) );\n"
    "	fragmentColor = vertexColor;\n"
    "}\n";

static const char AXES_FRAGMENT_SHADER[] =
    "in lowp vec4 fragmentColor;\n"
    "out lowp vec4 outColor;\n"
    "void main()\n"
    "{\n"
    "	outColor = fragmentColor;\n"
    "}\n";

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

static void* GlGetExtensionProc(const char* functionName) {
#if defined(ANDROID)
    return (void*)eglGetProcAddress(functionName);
#elif defined(WIN32)
    return (void*)wglGetProcAddress(functionName);
#else
    static_assert(false);
#endif
}

bool ovrFramebuffer::Create(
    const GLenum colorFormat,
    const int width,
    const int height,
    const int multisamples,
    const int swapChainLength,
    GLuint* colorTextures) {
    PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR =
        (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)GlGetExtensionProc(
            "glFramebufferTextureMultiviewOVR");
    PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC glFramebufferTextureMultisampleMultiviewOVR =
        (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)GlGetExtensionProc(
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
        } else if (glFramebufferTextureMultiviewOVR) {
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
#if defined(ANDROID)
    // Discard the depth buffer, so the tiler won't need to write it back out to memory.
    const GLenum depthAttachment[1] = {GL_DEPTH_ATTACHMENT};
    glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, depthAttachment);
#endif // defined(ANDROID)

    // We now let the resolve happen implicitly.
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

    CubeProgram.Clear();
    Cube.Clear();
    StageProgram.Clear();
    Stage.Clear();
    AxesProgram.Clear();
    Axes.Clear();
}

bool ovrScene::IsCreated() {
    return CreatedScene;
}

void ovrScene::CreateVAOs() {
    if (!CreatedVAOs) {
        // Cube
        Cube.CreateVAO();

        // Stage
        Stage.CreateVAO();

        // Axes
        Axes.CreateVAO();

        CreatedVAOs = true;
    }
}

void ovrScene::DestroyVAOs() {
    if (CreatedVAOs) {
        Cube.DestroyVAO();
        Stage.DestroyVAO();
        Axes.DestroyVAO();

        CreatedVAOs = false;
    }
}

void ovrScene::Create() {
    // Cube
    CubeProgram.Create(CUBE_VERTEX_SHADER, CUBE_FRAGMENT_SHADER);
    Cube.CreateCube();

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
    if (!AxesProgram.Create(AXES_VERTEX_SHADER, AXES_FRAGMENT_SHADER)) {
        ALOGE("Failed to compile axes program");
    }
    Axes.CreateAxes();

    CreatedScene = true;

    CreateVAOs();
    float c[] = {0.0, 0.0, 0.0, 0.0};
    SetClearColor(c);
}

void ovrScene::Destroy() {
    DestroyVAOs();

    CubeProgram.Destroy();
    Cube.Destroy();
    GL(glDeleteBuffers(1, &SceneMatrices));
    StageProgram.Destroy();
    Stage.Destroy();
    AxesProgram.Destroy();
    Axes.Destroy();
    CreatedScene = false;
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

void ovrAppRenderer::RenderFrame(ovrAppRenderer::FrameIn frameIn) {
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

    // Cubes
    GL(glUseProgram(Scene.CubeProgram.Program));
    GL(glBindBufferBase(
        GL_UNIFORM_BUFFER,
        Scene.CubeProgram.UniformBinding[ovrUniform::Index::SCENE_MATRICES],
        Scene.SceneMatrices));
    if (Scene.CubeProgram.UniformLocation[ovrUniform::Index::VIEW_ID] >=
        0) // NOTE: will not be present when multiview path is enabled.
    {
        GL(glUniform1i(Scene.CubeProgram.UniformLocation[ovrUniform::Index::VIEW_ID], 0));
    }
    for (auto c : Scene.CubeData) {
        GLint loc = Scene.CubeProgram.UniformLocation[ovrUniform::Index::MODEL_MATRIX];
        if (loc >= 0) {
            GL(glUniformMatrix4fv(loc, 1, GL_TRUE, &c.Model.M[0][0]));
        }
        loc = Scene.CubeProgram.UniformLocation[ovrUniform::Index::COLOR_SCALE];
        if (loc >= 0) {
            GL(glUniform4fv(loc, 1, &c.ColorScale.x));
        }
        loc = Scene.CubeProgram.UniformLocation[ovrUniform::Index::COLOR_BIAS];
        if (loc >= 0) {
            GL(glUniform4fv(loc, 1, &c.ColorBias.x));
        }
        GL(glBindVertexArray(Scene.Cube.VertexArrayObject));
        GL(glDrawElements(GL_TRIANGLES, Scene.Cube.IndexCount, GL_UNSIGNED_SHORT, nullptr));
    }

    GL(glBindVertexArray(0));
    GL(glUseProgram(0));

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
    GL(glBindVertexArray(Scene.Axes.VertexArrayObject));
    GL(glDrawElements(GL_LINES, Scene.Axes.IndexCount, GL_UNSIGNED_SHORT, nullptr));
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
        GL(glBindVertexArray(Scene.Axes.VertexArrayObject));
        GL(glDrawElements(GL_LINES, Scene.Axes.IndexCount, GL_UNSIGNED_SHORT, nullptr));
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
        GL(glBindVertexArray(Scene.Stage.VertexArrayObject));
        GL(glDrawElements(GL_TRIANGLES, Scene.Stage.IndexCount, GL_UNSIGNED_SHORT, nullptr));
        GL(glDepthMask(GL_TRUE));
        GL(glDisable(GL_BLEND));
        GL(glBindVertexArray(0));
        GL(glUseProgram(0));

        Framebuffer.Resolve();
    }

    Framebuffer.Unbind();
}
