/*
Copyright (c) 2016 Oculus VR, LLC.
*/

// This file is a cut down version of gfxwrapper_opengl.c from the OpenXR SDK.
// It is originally based on the Oculus multiplatform GL wrapper but all of the
// code for platforms other than WIN32 has been removed.

#include "GlWrapperWin32.h"

#if defined(WIN32)

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h> // for memset
#include <errno.h> // for EBUSY, ETIMEDOUT etc.
#include <ctype.h> // for isspace, isdigit

#define APPLICATION_NAME "OpenGL SI"
#define WINDOW_TITLE "OpenGL SI"

#include "GL/Glu.h"
#pragma comment(lib, "glu32.lib")

static void Error(const char* format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf_s(buffer, 4096, _TRUNCATE, format, args);
    va_end(args);

    OutputDebugStringA(buffer);

    // MessageBoxA(NULL, buffer, "ERROR", MB_OK | MB_ICONINFORMATION);
    // Without exiting, the application will likely crash.
    if (format != NULL) {
        exit(1);
    }
}

/*
================================================================================================================================

OpenGL error checking.

================================================================================================================================
*/

#if !defined(NDEBUG)
#define GL(func) \
    func;        \
    GlCheckErrors(#func);
#else
#define GL(func) func;
#endif

#if !defined(NDEBUG)
#define EGL(func)                                                  \
    if (func == EGL_FALSE) {                                       \
        Error(#func " failed: %s", EglErrorString(eglGetError())); \
    }
#else
#define EGL(func)                                                  \
    if (func == EGL_FALSE) {                                       \
        Error(#func " failed: %s", EglErrorString(eglGetError())); \
    }
#endif

/*
================================
Get proc address / extensions
================================
*/

PROC GetExtension(const char* functionName) {
    return wglGetProcAddress(functionName);
}

PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
PFNGLISRENDERBUFFERPROC glIsRenderbuffer;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC glCheckNamedFramebufferStatus;

PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBINDBUFFERBASEPROC glBindBufferBase;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
PFNGLBUFFERSTORAGEPROC glBufferStorage;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;

PFNGLACTIVETEXTUREPROC glActiveTexture;
PFNGLTEXIMAGE3DPROC glTexImage3D;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D;
PFNGLCOMPRESSEDTEXIMAGE3DPROC glCompressedTexImage3D;
PFNGLTEXSUBIMAGE3DPROC glTexSubImage3D;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glCompressedTexSubImage3D;
PFNGLTEXSTORAGE2DPROC glTexStorage2D;
PFNGLTEXSTORAGE3DPROC glTexStorage3D;
PFNGLTEXIMAGE2DMULTISAMPLEPROC glTexImage2DMultisample;
PFNGLTEXIMAGE3DMULTISAMPLEPROC glTexImage3DMultisample;
PFNGLTEXSTORAGE2DMULTISAMPLEPROC glTexStorage2DMultisample;
PFNGLTEXSTORAGE3DMULTISAMPLEPROC glTexStorage3DMultisample;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
PFNGLBINDIMAGETEXTUREPROC glBindImageTexture;

PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex;
PFNGLGETPROGRAMRESOURCEINDEXPROC glGetProgramResourceIndex;
PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding;
PFNGLSHADERSTORAGEBLOCKBINDINGPROC glShaderStorageBlockBinding;
PFNGLPROGRAMUNIFORM1IPROC glProgramUniform1i;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM1IVPROC glUniform1iv;
PFNGLUNIFORM2IVPROC glUniform2iv;
PFNGLUNIFORM3IVPROC glUniform3iv;
PFNGLUNIFORM4IVPROC glUniform4iv;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM1FVPROC glUniform1fv;
PFNGLUNIFORM2FVPROC glUniform2fv;
PFNGLUNIFORM3FVPROC glUniform3fv;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv;
PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv;
PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv;
PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv;
PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv;
PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;

PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;
PFNGLDISPATCHCOMPUTEPROC glDispatchCompute;
PFNGLMEMORYBARRIERPROC glMemoryBarrier;

PFNGLGENQUERIESPROC glGenQueries;
PFNGLDELETEQUERIESPROC glDeleteQueries;
PFNGLISQUERYPROC glIsQuery;
PFNGLBEGINQUERYPROC glBeginQuery;
PFNGLENDQUERYPROC glEndQuery;
PFNGLQUERYCOUNTERPROC glQueryCounter;
PFNGLGETQUERYIVPROC glGetQueryiv;
PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv;
PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuiv;
PFNGLGETQUERYOBJECTI64VPROC glGetQueryObjecti64v;
PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v;

PFNGLFENCESYNCPROC glFenceSync;
PFNGLCLIENTWAITSYNCPROC glClientWaitSync;
PFNGLDELETESYNCPROC glDeleteSync;
PFNGLISSYNCPROC glIsSync;

PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
PFNGLBLENDEQUATIONSEPARATEPROC glBlendEquationSeparate;

PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl;
PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;

PFNGLBLENDCOLORPROC glBlendColor;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
PFNWGLDELAYBEFORESWAPNVPROC wglDelayBeforeSwapNV;

PFNGLDEPTHRANGEFPROC glDepthRangef;
PFNGLBLENDEQUATIONPROC glBlendEquation;
PFNGLINVALIDATEFRAMEBUFFERPROC glInvalidateFramebuffer;

void GlBootstrapExtensions() {
    wglChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC)GetExtension("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC)GetExtension("wglCreateContextAttribsARB");
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)GetExtension("wglSwapIntervalEXT");
    wglDelayBeforeSwapNV = (PFNWGLDELAYBEFORESWAPNVPROC)GetExtension("wglDelayBeforeSwapNV");
}

void GlInitExtensions() {
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)GetExtension("glGenFramebuffers");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)GetExtension("glDeleteFramebuffers");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)GetExtension("glBindFramebuffer");
    glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)GetExtension("glBlitFramebuffer");
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)GetExtension("glGenRenderbuffers");
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)GetExtension("glDeleteRenderbuffers");
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)GetExtension("glBindRenderbuffer");
    glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)GetExtension("glIsRenderbuffer");
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)GetExtension("glRenderbufferStorage");
    glRenderbufferStorageMultisample =
        (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)GetExtension("glRenderbufferStorageMultisample");
    glFramebufferRenderbuffer =
        (PFNGLFRAMEBUFFERRENDERBUFFERPROC)GetExtension("glFramebufferRenderbuffer");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)GetExtension("glFramebufferTexture2D");
    glFramebufferTextureLayer =
        (PFNGLFRAMEBUFFERTEXTURELAYERPROC)GetExtension("glFramebufferTextureLayer");
    glCheckFramebufferStatus =
        (PFNGLCHECKFRAMEBUFFERSTATUSPROC)GetExtension("glCheckFramebufferStatus");
    glCheckNamedFramebufferStatus =
        (PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC)GetExtension("glCheckNamedFramebufferStatus");

    glGenBuffers = (PFNGLGENBUFFERSPROC)GetExtension("glGenBuffers");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)GetExtension("glDeleteBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)GetExtension("glBindBuffer");
    glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)GetExtension("glBindBufferBase");
    glBufferData = (PFNGLBUFFERDATAPROC)GetExtension("glBufferData");
    glBufferSubData = (PFNGLBUFFERSUBDATAPROC)GetExtension("glBufferSubData");
    glBufferStorage = (PFNGLBUFFERSTORAGEPROC)GetExtension("glBufferStorage");
    glMapBuffer = (PFNGLMAPBUFFERPROC)GetExtension("glMapBuffer");
    glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)GetExtension("glMapBufferRange");
    glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)GetExtension("glUnmapBuffer");

    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)GetExtension("glGenVertexArrays");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)GetExtension("glDeleteVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)GetExtension("glBindVertexArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)GetExtension("glVertexAttribPointer");
    glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)GetExtension("glVertexAttribDivisor");
    glDisableVertexAttribArray =
        (PFNGLDISABLEVERTEXATTRIBARRAYPROC)GetExtension("glDisableVertexAttribArray");
    glEnableVertexAttribArray =
        (PFNGLENABLEVERTEXATTRIBARRAYPROC)GetExtension("glEnableVertexAttribArray");

    glActiveTexture = (PFNGLACTIVETEXTUREPROC)GetExtension("glActiveTexture");
    glTexImage3D = (PFNGLTEXIMAGE3DPROC)GetExtension("glTexImage3D");
    glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)GetExtension("glCompressedTexImage2D");
    glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)GetExtension("glCompressedTexImage3D");
    glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)GetExtension("glTexSubImage3D");
    glCompressedTexSubImage2D =
        (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)GetExtension("glCompressedTexSubImage2D");
    glCompressedTexSubImage3D =
        (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)GetExtension("glCompressedTexSubImage3D");
    glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)GetExtension("glTexStorage2D");
    glTexStorage3D = (PFNGLTEXSTORAGE3DPROC)GetExtension("glTexStorage3D");
    glTexImage2DMultisample =
        (PFNGLTEXIMAGE2DMULTISAMPLEPROC)GetExtension("glTexImage2DMultisample");
    glTexImage3DMultisample =
        (PFNGLTEXIMAGE3DMULTISAMPLEPROC)GetExtension("glTexImage3DMultisample");
    glTexStorage2DMultisample =
        (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)GetExtension("glTexStorage2DMultisample");
    glTexStorage3DMultisample =
        (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)GetExtension("glTexStorage3DMultisample");
    glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)GetExtension("glGenerateMipmap");
    glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)GetExtension("glBindImageTexture");

    glCreateProgram = (PFNGLCREATEPROGRAMPROC)GetExtension("glCreateProgram");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)GetExtension("glDeleteProgram");
    glCreateShader = (PFNGLCREATESHADERPROC)GetExtension("glCreateShader");
    glDeleteShader = (PFNGLDELETESHADERPROC)GetExtension("glDeleteShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)GetExtension("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)GetExtension("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)GetExtension("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)GetExtension("glGetShaderInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)GetExtension("glUseProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)GetExtension("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)GetExtension("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)GetExtension("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)GetExtension("glGetProgramInfoLog");
    glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)GetExtension("glGetAttribLocation");
    glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)GetExtension("glBindAttribLocation");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GetExtension("glGetUniformLocation");
    glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)GetExtension("glGetUniformBlockIndex");
    glProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)GetExtension("glProgramUniform1i");
    glUniform1i = (PFNGLUNIFORM1IPROC)GetExtension("glUniform1i");
    glUniform1iv = (PFNGLUNIFORM1IVPROC)GetExtension("glUniform1iv");
    glUniform2iv = (PFNGLUNIFORM2IVPROC)GetExtension("glUniform2iv");
    glUniform3iv = (PFNGLUNIFORM3IVPROC)GetExtension("glUniform3iv");
    glUniform4iv = (PFNGLUNIFORM4IVPROC)GetExtension("glUniform4iv");
    glUniform1f = (PFNGLUNIFORM1FPROC)GetExtension("glUniform1f");
    glUniform1fv = (PFNGLUNIFORM1FVPROC)GetExtension("glUniform1fv");
    glUniform2fv = (PFNGLUNIFORM2FVPROC)GetExtension("glUniform2fv");
    glUniform3fv = (PFNGLUNIFORM3FVPROC)GetExtension("glUniform3fv");
    glUniform4fv = (PFNGLUNIFORM4FVPROC)GetExtension("glUniform4fv");
    glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)GetExtension("glUniformMatrix3fv");
    glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)GetExtension("glUniformMatrix2x3fv");
    glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)GetExtension("glUniformMatrix2x4fv");
    glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)GetExtension("glUniformMatrix3x2fv");
    glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)GetExtension("glUniformMatrix3fv");
    glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)GetExtension("glUniformMatrix3x4fv");
    glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)GetExtension("glUniformMatrix4x2fv");
    glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)GetExtension("glUniformMatrix4x3fv");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)GetExtension("glUniformMatrix4fv");
    glGetProgramResourceIndex =
        (PFNGLGETPROGRAMRESOURCEINDEXPROC)GetExtension("glGetProgramResourceIndex");
    glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)GetExtension("glUniformBlockBinding");
    glShaderStorageBlockBinding =
        (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)GetExtension("glShaderStorageBlockBinding");

    glDrawElementsInstanced =
        (PFNGLDRAWELEMENTSINSTANCEDPROC)GetExtension("glDrawElementsInstanced");
    glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)GetExtension("glDispatchCompute");
    glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)GetExtension("glMemoryBarrier");

    glGenQueries = (PFNGLGENQUERIESPROC)GetExtension("glGenQueries");
    glDeleteQueries = (PFNGLDELETEQUERIESPROC)GetExtension("glDeleteQueries");
    glIsQuery = (PFNGLISQUERYPROC)GetExtension("glIsQuery");
    glBeginQuery = (PFNGLBEGINQUERYPROC)GetExtension("glBeginQuery");
    glEndQuery = (PFNGLENDQUERYPROC)GetExtension("glEndQuery");
    glQueryCounter = (PFNGLQUERYCOUNTERPROC)GetExtension("glQueryCounter");
    glGetQueryiv = (PFNGLGETQUERYIVPROC)GetExtension("glGetQueryiv");
    glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)GetExtension("glGetQueryObjectiv");
    glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)GetExtension("glGetQueryObjectuiv");
    glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)GetExtension("glGetQueryObjecti64v");
    glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)GetExtension("glGetQueryObjectui64v");

    glFenceSync = (PFNGLFENCESYNCPROC)GetExtension("glFenceSync");
    glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)GetExtension("glClientWaitSync");
    glDeleteSync = (PFNGLDELETESYNCPROC)GetExtension("glDeleteSync");
    glIsSync = (PFNGLISSYNCPROC)GetExtension("glIsSync");

    glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)GetExtension("glBlendFuncSeparate");
    glBlendEquationSeparate =
        (PFNGLBLENDEQUATIONSEPARATEPROC)GetExtension("glBlendEquationSeparate");

    glBlendColor = (PFNGLBLENDCOLORPROC)GetExtension("glBlendColor");

    glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)GetExtension("glDebugMessageControl");
    glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)GetExtension("glDebugMessageCallback");

    glDepthRangef = (PFNGLDEPTHRANGEFPROC)GetExtension("glDepthRangef");
    glBlendEquation = (PFNGLBLENDEQUATIONPROC)GetExtension("glBlendEquation");
    glInvalidateFramebuffer =
        (PFNGLINVALIDATEFRAMEBUFFERPROC)GetExtension("glInvalidateFramebuffer");
}

typedef enum {
    KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5,
    KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5,
    KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8,
    KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8,
    KS_GPU_SURFACE_COLOR_FORMAT_MAX
} ksGpuSurfaceColorFormat;

typedef enum {
    KS_GPU_SURFACE_DEPTH_FORMAT_NONE,
    KS_GPU_SURFACE_DEPTH_FORMAT_D16,
    KS_GPU_SURFACE_DEPTH_FORMAT_D24,
    KS_GPU_SURFACE_DEPTH_FORMAT_MAX
} ksGpuSurfaceDepthFormat;

typedef enum {
    KS_GPU_SAMPLE_COUNT_1 = 1,
    KS_GPU_SAMPLE_COUNT_2 = 2,
    KS_GPU_SAMPLE_COUNT_4 = 4,
    KS_GPU_SAMPLE_COUNT_8 = 8,
    KS_GPU_SAMPLE_COUNT_16 = 16,
    KS_GPU_SAMPLE_COUNT_32 = 32,
    KS_GPU_SAMPLE_COUNT_64 = 64,
} ksGpuSampleCount;

typedef struct {
    HDC hDC;
    HGLRC hGLRC;
} ksGpuContext;

typedef struct {
    unsigned char redBits;
    unsigned char greenBits;
    unsigned char blueBits;
    unsigned char alphaBits;
    unsigned char colorBits;
    unsigned char depthBits;
} ksGpuSurfaceBits;

typedef struct {
    ksGpuContext context;
    ksGpuSurfaceColorFormat colorFormat;
    ksGpuSurfaceDepthFormat depthFormat;
    ksGpuSampleCount sampleCount;
    int windowWidth;
    int windowHeight;

    HINSTANCE hInstance;
    HDC hDC;
    HWND hWnd;
} ksGpuWindow;

/*
================================================================================================================================

GPU Context.

================================================================================================================================
*/

ksGpuSurfaceBits ksGpuContext_BitsForSurfaceFormat(
    const ksGpuSurfaceColorFormat colorFormat,
    const ksGpuSurfaceDepthFormat depthFormat) {
    ksGpuSurfaceBits bits;
    bits.redBits =
        ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
             ? 8
             : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                    ? 8
                    : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                           ? 5
                           : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5) ? 5 : 8))));
    bits.greenBits =
        ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
             ? 8
             : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                    ? 8
                    : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                           ? 6
                           : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5) ? 6 : 8))));
    bits.blueBits =
        ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
             ? 8
             : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                    ? 8
                    : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                           ? 5
                           : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5) ? 5 : 8))));
    bits.alphaBits =
        ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
             ? 8
             : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                    ? 8
                    : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                           ? 0
                           : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5) ? 0 : 8))));
    bits.colorBits = bits.redBits + bits.greenBits + bits.blueBits + bits.alphaBits;
    bits.depthBits =
        ((depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D16)
             ? 16
             : ((depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D24) ? 24 : 0));
    return bits;
}

static bool ksGpuContext_CreateForSurface(
    ksGpuContext* context,
    const ksGpuSurfaceColorFormat colorFormat,
    const ksGpuSurfaceDepthFormat depthFormat,
    const ksGpuSampleCount sampleCount,
    HINSTANCE hInstance,
    HDC hDC) {
    const ksGpuSurfaceBits bits = ksGpuContext_BitsForSurfaceFormat(colorFormat, depthFormat);

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1, // version
        PFD_DRAW_TO_WINDOW | // must support windowed
            PFD_SUPPORT_OPENGL | // must support OpenGL
            PFD_DOUBLEBUFFER, // must support double buffering
        PFD_TYPE_RGBA, // iPixelType
        bits.colorBits, // cColorBits
        0,
        0, // cRedBits, cRedShift
        0,
        0, // cGreenBits, cGreenShift
        0,
        0, // cBlueBits, cBlueShift
        0,
        0, // cAlphaBits, cAlphaShift
        0, // cAccumBits
        0, // cAccumRedBits
        0, // cAccumGreenBits
        0, // cAccumBlueBits
        0, // cAccumAlphaBits
        bits.depthBits, // cDepthBits
        0, // cStencilBits
        0, // cAuxBuffers
        PFD_MAIN_PLANE, // iLayerType
        0, // bReserved
        0, // dwLayerMask
        0, // dwVisibleMask
        0 // dwDamageMask
    };

    HWND localWnd = NULL;
    HDC localDC = hDC;

    if (sampleCount > KS_GPU_SAMPLE_COUNT_1) {
        // A valid OpenGL context is needed to get OpenGL extensions including
        // wglChoosePixelFormatARB and wglCreateContextAttribsARB. A device context with a valid
        // pixel format is needed to create an OpenGL context. However, once a pixel format is set
        // on a device context it is final. Therefore a pixel format is set on the device context of
        // a temporary window to create a context to get the extensions for multi-sampling.
        localWnd =
            CreateWindowA(APPLICATION_NAME, "temp", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
        localDC = GetDC(localWnd);
    }

    int pixelFormat = ChoosePixelFormat(localDC, &pfd);
    if (pixelFormat == 0) {
        Error("Failed to find a suitable pixel format.");
        return false;
    }

    if (!SetPixelFormat(localDC, pixelFormat, &pfd)) {
        Error("Failed to set the pixel format.");
        return false;
    }

    // Now that the pixel format is set, create a temporary context to get the extensions.
    {
        HGLRC hGLRC = wglCreateContext(localDC);
        wglMakeCurrent(localDC, hGLRC);

        GlBootstrapExtensions();

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hGLRC);
    }

    if (sampleCount > KS_GPU_SAMPLE_COUNT_1) {
        // Release the device context and destroy the window that were created to get extensions.
        ReleaseDC(localWnd, localDC);
        DestroyWindow(localWnd);

        int pixelFormatAttribs[] = {
            WGL_DRAW_TO_WINDOW_ARB,
            GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,
            GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,
            GL_TRUE,
            WGL_PIXEL_TYPE_ARB,
            WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,
            bits.colorBits,
            WGL_DEPTH_BITS_ARB,
            bits.depthBits,
            WGL_SAMPLE_BUFFERS_ARB,
            1,
            WGL_SAMPLES_ARB,
            sampleCount,
            0};

        unsigned int numPixelFormats = 0;

        if (!wglChoosePixelFormatARB(
                hDC, pixelFormatAttribs, NULL, 1, &pixelFormat, &numPixelFormats) ||
            numPixelFormats == 0) {
            Error("Failed to find MSAA pixel format.");
            return false;
        }

        memset(&pfd, 0, sizeof(pfd));

        if (!DescribePixelFormat(hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd)) {
            Error("Failed to describe the pixel format.");
            return false;
        }

        if (!SetPixelFormat(hDC, pixelFormat, &pfd)) {
            Error("Failed to set the pixel format.");
            return false;
        }
    }

    int contextAttribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB,
        OPENGL_VERSION_MAJOR,
        WGL_CONTEXT_MINOR_VERSION_ARB,
        OPENGL_VERSION_MINOR,
        WGL_CONTEXT_PROFILE_MASK_ARB,
        WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB,
        WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
        0};

    context->hDC = hDC;
    context->hGLRC = wglCreateContextAttribsARB(hDC, NULL, contextAttribs);
    if (!context->hGLRC) {
        Error("Failed to create GL context.");
        return false;
    }

    wglMakeCurrent(hDC, context->hGLRC);

    GlInitExtensions();

    return true;
}

void ksGpuContext_Destroy(ksGpuContext* context) {
    if (context->hGLRC) {
        if (!wglMakeCurrent(NULL, NULL)) {
            DWORD error = GetLastError();
            Error("Failed to release context error code (%d).", error);
        }

        if (!wglDeleteContext(context->hGLRC)) {
            DWORD error = GetLastError();
            Error("Failed to delete context error code (%d).", error);
        }
        context->hGLRC = NULL;
    }
    context->hDC = NULL;
}

/*
================================================================================================================================

GPU Window.

================================================================================================================================
*/

void ksGpuWindow_Destroy(ksGpuWindow* window) {
    ksGpuContext_Destroy(&window->context);

    if (window->hDC) {
        if (!ReleaseDC(window->hWnd, window->hDC)) {
            Error("Failed to release device context.");
        }
        window->hDC = NULL;
    }

    if (window->hWnd) {
        if (!DestroyWindow(window->hWnd)) {
            Error("Failed to destroy the window.");
        }
        window->hWnd = NULL;
    }

    if (window->hInstance) {
        if (!UnregisterClassA(APPLICATION_NAME, window->hInstance)) {
            Error("Failed to unregister window class.");
        }
        window->hInstance = NULL;
    }
}

static LRESULT APIENTRY WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CLOSE: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcA(hWnd, message, wParam, lParam);
}

bool ksGpuWindow_Create(
    ksGpuWindow* window,
    ksGpuSurfaceColorFormat colorFormat,
    ksGpuSurfaceDepthFormat depthFormat,
    ksGpuSampleCount sampleCount,
    int width,
    int height) {
    memset(window, 0, sizeof(ksGpuWindow));

    window->colorFormat = colorFormat;
    window->depthFormat = depthFormat;
    window->sampleCount = sampleCount;
    window->windowWidth = width;
    window->windowHeight = height;

    DEVMODEA lpDevMode;
    memset(&lpDevMode, 0, sizeof(DEVMODEA));
    lpDevMode.dmSize = sizeof(DEVMODEA);
    lpDevMode.dmDriverExtra = 0;

    window->hInstance = GetModuleHandleA(NULL);

    WNDCLASSA wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = window->hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = APPLICATION_NAME;

    if (!RegisterClassA(&wc)) {
        Error("Failed to register window class.");
        return false;
    }

    // Fixed size window.
    DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT windowRect;
    windowRect.left = (long)0;
    windowRect.right = (long)width;
    windowRect.top = (long)0;
    windowRect.bottom = (long)height;

    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

    RECT desktopRect;
    GetWindowRect(GetDesktopWindow(), &desktopRect);

    const int offsetX = (desktopRect.right - (windowRect.right - windowRect.left)) / 2;
    const int offsetY = (desktopRect.bottom - (windowRect.bottom - windowRect.top)) / 2;

    windowRect.left += offsetX;
    windowRect.right += offsetX;
    windowRect.top += offsetY;
    windowRect.bottom += offsetY;

    window->hWnd = CreateWindowExA(
        dwExStyle, // Extended style for the window
        APPLICATION_NAME, // Class name
        WINDOW_TITLE, // Window title
        dwStyle | // Defined window style
            WS_CLIPSIBLINGS | // Required window style
            WS_CLIPCHILDREN, // Required window style
        windowRect.left, // Window X position
        windowRect.top, // Window Y position
        windowRect.right - windowRect.left, // Window width
        windowRect.bottom - windowRect.top, // Window height
        NULL, // No parent window
        NULL, // No menu
        window->hInstance, // Instance
        NULL); // No WM_CREATE parameter
    if (!window->hWnd) {
        ksGpuWindow_Destroy(window);
        Error("Failed to create window.");
        return false;
    }

    SetWindowLongPtrA(window->hWnd, GWLP_USERDATA, (LONG_PTR)window);

    window->hDC = GetDC(window->hWnd);
    if (!window->hDC) {
        ksGpuWindow_Destroy(window);
        Error("Failed to acquire device context.");
        return false;
    }

    ksGpuContext_CreateForSurface(
        &window->context, colorFormat, depthFormat, sampleCount, window->hInstance, window->hDC);

    wglMakeCurrent(window->context.hDC, window->context.hGLRC);

    ShowWindow(window->hWnd, SW_SHOW);
    SetForegroundWindow(window->hWnd);
    SetFocus(window->hWnd);

    return true;
}

ksGpuWindow window;

void ovrGl_CreateContext_Windows(HDC* hDC, HGLRC* hGLRC) {
    ksGpuSurfaceColorFormat colorFormat = KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8;
    ksGpuSurfaceDepthFormat depthFormat = KS_GPU_SURFACE_DEPTH_FORMAT_D24;
    ksGpuSampleCount sampleCount = KS_GPU_SAMPLE_COUNT_1;
    if (!ksGpuWindow_Create(&window, colorFormat, depthFormat, sampleCount, 640, 480)) {
        Error("Unable to create GL context");
    }

    *hDC = window.context.hDC;
    *hGLRC = window.context.hGLRC;
}

void ovrGl_DestroyContext_Windows(HDC* hDC, HGLRC* hGLRC) {
    ksGpuWindow_Destroy(&window);
}

const char* ovrGl_ErrorString_Windows(const GLint err) {
    return (const char*)gluErrorString(err);
}

#endif // defined(OS_WINDOWS)
