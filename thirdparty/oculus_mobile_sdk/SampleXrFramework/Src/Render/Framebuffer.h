// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/************************************************************************************

Filename  	: 	Framebuffer.h
Content		: 	Frame buffer utilities. Originally part of the VrCubeWorld_NativeActivity
                sample.
Created		: 	March, 2015
Authors		:	J.M.P. van Waveren

 *************************************************************************************/

#pragma once

#include "Render/Egl.h"

#if !defined(EGL_OPENGL_ES3_BIT_KHR)
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

// EXT_texture_border_clamp
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
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

// GL_EXT_texture_cube_map_array
#if !defined(GL_TEXTURE_CUBE_MAP_ARRAY)
#define GL_TEXTURE_CUBE_MAP_ARRAY 0x9009
#endif

#if defined(ANDROID)
#include <jni.h>
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#elif defined(WIN32)
#include <unknwn.h>
#define XR_USE_GRAPHICS_API_OPENGL 1
#define XR_USE_PLATFORM_WIN32 1
#endif // defined(ANDROID)

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

struct ovrSwapChain {
    XrSwapchain Handle;
    uint32_t Width;
    uint32_t Height;
};

typedef struct ovrFramebuffer_s {
    int Width;
    int Height;
    int Multisamples;
    uint32_t TextureSwapChainLength;
    uint32_t TextureSwapChainIndex;
    struct ovrSwapChain ColorSwapChain;
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    XrSwapchainImageOpenGLESKHR* ColorSwapChainImage;
#elif defined(XR_USE_GRAPHICS_API_OPENGL)
    XrSwapchainImageOpenGLKHR* ColorSwapChainImage;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    GLuint* DepthBuffers;
    GLuint* FrameBuffers;
} ovrFramebuffer;

void ovrFramebuffer_Clear(ovrFramebuffer* frameBuffer);
bool ovrFramebuffer_Create(
    XrSession session,
    ovrFramebuffer* frameBuffer,
    const GLenum colorFormat,
    const int width,
    const int height,
    const int multisamples);
void ovrFramebuffer_Destroy(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_SetCurrent(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_SetNone();
void ovrFramebuffer_Resolve(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_Acquire(ovrFramebuffer* frameBuffer);
void ovrFramebuffer_Release(ovrFramebuffer* frameBuffer);

/// convenience
class scope_frame_buffer {
   public:
    scope_frame_buffer(ovrFramebuffer* fb) : fb_(fb) {
        ovrFramebuffer_Acquire(fb_);
        ovrFramebuffer_SetCurrent(fb_);
    }
    ~scope_frame_buffer() {
        ovrFramebuffer_Resolve(fb_);
        ovrFramebuffer_Release(fb_);
        ovrFramebuffer_SetNone();
    }

   private:
    ovrFramebuffer* fb_;
};
