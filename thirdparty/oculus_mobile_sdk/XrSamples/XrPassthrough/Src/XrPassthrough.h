#pragma once

#if defined(ANDROID)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#else
#include "unknwn.h"
#include "Render/GlWrapperWin32.h"
#define XR_USE_GRAPHICS_API_OPENGL 1
#define XR_USE_PLATFORM_WIN32 1
#endif // defined(ANDROID)

#include <openxr/openxr.h>
#include <openxr/openxr_oculus.h>
#include <openxr/openxr_oculus_helpers.h>
#include <openxr/openxr_platform.h>

#include "XrPassthroughGl.h"

void OXR_CheckErrors(XrResult result, const char* function, bool failOnError);
#define OXR(func) OXR_CheckErrors(func, #func, true);

inline OVR::Matrix4f OvrFromXr(const XrMatrix4x4f& x) {
    return OVR::Matrix4f(
        x.m[0x0],
        x.m[0x1],
        x.m[0x2],
        x.m[0x3],
        x.m[0x4],
        x.m[0x5],
        x.m[0x6],
        x.m[0x7],
        x.m[0x8],
        x.m[0x9],
        x.m[0xa],
        x.m[0xb],
        x.m[0xc],
        x.m[0xd],
        x.m[0xe],
        x.m[0xf]);
}

inline OVR::Quatf OvrFromXr(const XrQuaternionf& q) {
    return OVR::Quatf(q.x, q.y, q.z, q.w);
}

inline OVR::Vector3f OvrFromXr(const XrVector3f& v) {
    return OVR::Vector3f(v.x, v.y, v.z);
}

inline OVR::Posef OvrFromXr(const XrPosef& p) {
    return OVR::Posef(OvrFromXr(p.orientation), OvrFromXr(p.position));
}

/*
================================================================================

Egl

================================================================================
*/

struct Egl {
    void Clear();
    void CreateContext(const Egl* shareEgl);
    void DestroyContext();
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    EGLint MajorVersion;
    EGLint MinorVersion;
    EGLDisplay Display;
    EGLConfig Config;
    EGLSurface TinySurface;
    EGLSurface MainSurface;
    EGLContext Context;
#elif defined(XR_USE_GRAPHICS_API_OPENGL)
    HDC hDC;
    HGLRC hGLRC;
#endif
};

/*
================================================================================

App

================================================================================
*/

union CompositionLayerUnion {
    XrCompositionLayerProjection Projection;
    XrCompositionLayerQuad Quad;
    XrCompositionLayerCylinderKHR Cylinder;
    XrCompositionLayerCubeKHR Cube;
    XrCompositionLayerEquirectKHR Equirect;
    // FB_passthrough sample begin
    XrCompositionLayerPassthroughFB Passthrough;
    // FB_passthrough sample end
};

enum { MaxLayerCount = 16 };

struct App {
    void Clear();
    void HandleSessionStateChanges(XrSessionState state);
    void HandleXrEvents();

    Egl egl;

#if defined(XR_USE_PLATFORM_ANDROID)
    ANativeWindow* NativeWindow;
    bool Resumed;
#endif // defined(XR_USE_PLATFORM_ANDROID)
    bool ShouldExit;
    bool Focused;

    XrInstance Instance;
    XrSession Session;
    XrViewConfigurationProperties ViewportConfig;
    XrViewConfigurationView ViewConfigurationView[NUM_EYES];
    XrSystemId SystemId;
    XrSpace HeadSpace;
    XrSpace LocalSpace;
    XrSpace StageSpace;
    bool SessionActive;

    int SwapInterval;
    int CpuLevel;
    int GpuLevel;
    // These threads will be marked as performance threads.
    int MainThreadTid;
    int RenderThreadTid;
    CompositionLayerUnion Layers[MaxLayerCount];
    int LayerCount;

    bool TouchPadDownLastFrame;

    XrSwapchain ColorSwapChain;
    uint32_t SwapChainLength;
    OVR::Vector3f StageBounds;
    // Provided by XrPassthroughGl, which is not aware of VrApi or OpenXR
    AppRenderer appRenderer;
};
