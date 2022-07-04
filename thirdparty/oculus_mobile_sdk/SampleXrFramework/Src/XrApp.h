/*******************************************************************************

Filename    :   XrApp.h
Content     :   OpenXR application base class.
Created     :   July 2020
Authors     :   Federico Schliemann
Language    :   c++
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*******************************************************************************/

#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "OVR_Math.h"

#include "System.h"
#include "FrameParams.h"
#include "OVR_FileSys.h"

#include <android/window.h>
#include <android/native_window_jni.h>
#include <android_native_app_glue.h>
#include <android/keycodes.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

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
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#elif defined(WIN32)
#include <unknwn.h>
#define XR_USE_GRAPHICS_API_OPENGL 1
#define XR_USE_PLATFORM_WINDOWS 1
#endif // defined(ANDROID)

#include <openxr/openxr.h>
#include <openxr/openxr_oculus.h>
#include <openxr/openxr_oculus_helpers.h>
#include <openxr/openxr_platform.h>

#include "Model/SceneView.h"
#include "Render/Egl.h"
#include "Render/Framebuffer.h"
#include "Render/SurfaceRender.h"

void OXR_CheckErrors(XrInstance instance, XrResult result, const char* function, bool failOnError);

#if defined(DEBUG)
#define OXR(func) OXR_CheckErrors(Instance, func, #func, true);
#else
#define OXR(func) OXR_CheckErrors(Instance, func, #func, false);
#endif

static inline XrVector2f ToXrVector2f(const OVR::Vector2f& s) {
    XrVector2f r;
    r.x = s.x;
    r.y = s.y;
    return r;
}

static inline OVR::Vector2f FromXrVector2f(const XrVector2f& s) {
    OVR::Vector2f r;
    r.x = s.x;
    r.y = s.y;
    return r;
}

static inline XrVector3f ToXrVector3f(const OVR::Vector3f& s) {
    XrVector3f r;
    r.x = s.x;
    r.y = s.y;
    r.z = s.z;
    return r;
}

static inline OVR::Vector3f FromXrVector3f(const XrVector3f& s) {
    OVR::Vector3f r;
    r.x = s.x;
    r.y = s.y;
    r.z = s.z;
    return r;
}

static inline OVR::Vector4f FromXrVector4f(const XrVector4f& s) {
    OVR::Vector4f r;
    r.x = s.x;
    r.y = s.y;
    r.z = s.z;
    r.w = s.w;
    return r;
}

static inline OVR::Vector4f FromXrColor4f(const XrColor4f& s) {
    OVR::Vector4f r;
    r.x = s.r;
    r.y = s.g;
    r.z = s.b;
    r.w = s.a;
    return r;
}

static inline XrQuaternionf ToXrQuaternionf(const OVR::Quatf& s) {
    XrQuaternionf r;
    r.x = s.x;
    r.y = s.y;
    r.z = s.z;
    r.w = s.w;
    return r;
}

static inline OVR::Quatf FromXrQuaternionf(const XrQuaternionf& s) {
    OVR::Quatf r;
    r.x = s.x;
    r.y = s.y;
    r.z = s.z;
    r.w = s.w;
    return r;
}

static inline XrPosef ToXrPosef(const OVR::Posef& s) {
    XrPosef r;
    r.orientation = ToXrQuaternionf(s.Rotation);
    r.position = ToXrVector3f(s.Translation);
    return r;
}

static inline OVR::Posef FromXrPosef(const XrPosef& s) {
    OVR::Posef r;
    r.Rotation = FromXrQuaternionf(s.orientation);
    r.Translation = FromXrVector3f(s.position);
    return r;
}

namespace OVRFW {

class XrApp {
   public:
    //============================
    // public interface
    enum ovrLifecycle { LIFECYCLE_UNKNOWN, LIFECYCLE_RESUMED, LIFECYCLE_PAUSED };

    enum ovrRenderState {
        RENDER_STATE_LOADING, // show the loading icon
        RENDER_STATE_RUNNING, // render frames
    };

    static const int CPU_LEVEL = 2;
    static const int GPU_LEVEL = 3;
    static const int NUM_MULTI_SAMPLES = 4;
    static const int MAX_NUM_EYES = 2;
    static const int MAX_NUM_LAYERS = 16;

    XrApp(
        const int32_t mainThreadTid,
        const int32_t renderThreadTid,
        const int cpuLevel,
        const int gpuLevel)
        : BackgroundColor(0.0f, 0.6f, 0.1f, 1.0f),
          CpuLevel(cpuLevel),
          GpuLevel(gpuLevel),
          MainThreadTid(mainThreadTid),
          RenderThreadTid(renderThreadTid),
          NumFramebuffers(MAX_NUM_EYES) {}
    XrApp() : XrApp(0, 0, CPU_LEVEL, GPU_LEVEL) {}
    virtual ~XrApp() = default;

    // App entry point
    void Run(struct android_app* app);

    //============================
    // public context interface

    // Returns the application's context
    const xrJava* GetContext() const {
        return &Context;
    }

    // App state share
    OVRFW::ovrFileSys* GetFileSys() {
        return FileSys.get();
    }
    OVRFW::ovrSurfaceRender& GetSurfaceRender() {
        return SurfaceRender;
    }
    OVRFW::OvrSceneView& GetScene() {
        return Scene;
    }

    void SetRunWhilePaused(bool b) {
        RunWhilePaused = b;
    }

    void HandleAndroidCmd(struct android_app* app, int32_t cmd);

   protected:
    uint64_t GetFrameIndex() const {
        return FrameIndex;
    }
    double GetDisplayTime() const {
        return DisplayTime;
    }
    int GetNumFramebuffers() const {
        return NumFramebuffers;
    }
    ovrFramebuffer* GetFrameBuffer(int eye) {
        return &FrameBuffer[eye];
    }

    //============================
    // App functions
    // All App* function can be overridden by the derived application class to
    // implement application-specific behaviors

    // Returns a list of OpenXr extensions needed for this app
    virtual std::vector<const char*> GetExtensions();

    // Called when the application initializes.
    // Must return true if the application initializes successfully.
    virtual bool AppInit(const xrJava* context);
    // Called when the application shuts down
    virtual void AppShutdown(const xrJava* context);
    // Called when the application is resumed by the system.
    virtual void AppResumed(const xrJava* contet);
    // Called when the application is paused by the system.
    virtual void AppPaused(const xrJava* context);
    // Called when app loses focus
    virtual void AppLostFocus();
    // Called when app re-gains focus
    virtual void AppGainedFocus();
    // Called once per frame to allow the application to render eye buffers.
    virtual void AppRenderFrame(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out);
    // Called once per eye each frame for default renderer
    virtual void
    AppRenderEye(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out, int eye);
    // Called once per eye each frame for default renderer
    virtual void AppEyeGLStateSetup(const ovrApplFrameIn& in, const ovrFramebuffer* fb, int eye);

    virtual bool SessionInit();
    virtual void SessionEnd();
    virtual void Update(const ovrApplFrameIn& in);
    virtual void Render(const ovrApplFrameIn& in, ovrRendererOutput& out);

    /// composition override
    union xrCompositorLayerUnion {
        XrCompositionLayerProjection Projection;
        XrCompositionLayerQuad Quad;
        XrCompositionLayerCylinderKHR Cylinder;
        XrCompositionLayerCubeKHR Cube;
        XrCompositionLayerEquirectKHR Equirect;
        XrCompositionLayerPassthroughFB Passthrough;
    };
    virtual void PreProjectionAddLayer(xrCompositorLayerUnion* layers, int& layerCount) {
        /// do nothing
    }
    virtual void PostProjectionAddLayer(xrCompositorLayerUnion* layers, int& layerCount) {
        /// do nothing
    }

    // Returns a map from interaction profile paths to vectors of suggested bindings.
    // xrSuggestInteractionProfileBindings() is called once for each interaction profile path in the
    // returned map.
    // Apps are encouraged to suggest bindings for every device/interaction profile they support.
    // Override this for custom action bindings, or modify the default bindings.
    virtual std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> GetSuggestedBindings(
        XrInstance instance);

    /// Xr Helpers
    XrInstance& GetInstance() {
        return Instance;
    };
    XrSession& GetSession() {
        return Session;
    }
    XrSystemId& GetSystemId() {
        return SystemId;
    }

    XrSpace& GetHeadSpace() {
        return HeadSpace;
    }
    XrSpace& GetLocalSpace() {
        return LocalSpace;
    }
    XrSpace& GetStageSpace() {
        return StageSpace;
    }
    XrSpace& GetCurrentSpace() {
        return CurrentSpace;
    }

    XrActionSet CreateActionSet(int priority, const char* name, const char* localizedName);
    XrAction CreateAction(
        XrActionSet actionSet,
        XrActionType type,
        const char* actionName,
        const char* localizedName,
        int countSubactionPaths = 0,
        XrPath* subactionPaths = nullptr);
    XrActionSuggestedBinding ActionSuggestedBinding(XrAction action, const char* bindingString);
    XrSpace CreateActionSpace(XrAction poseAction, XrPath subactionPath);
    XrActionStateBoolean GetActionStateBoolean(
        XrAction action,
        XrPath subactionPath = XR_NULL_PATH);
    XrActionStateFloat GetActionStateFloat(XrAction action, XrPath subactionPath = XR_NULL_PATH);
    XrActionStateVector2f GetActionStateVector2(
        XrAction action,
        XrPath subactionPath = XR_NULL_PATH);
    bool ActionPoseIsActive(XrAction action, XrPath subactionPath);
    struct LocVel {
        XrSpaceLocation loc;
        XrSpaceVelocity vel;
    };
    XrApp::LocVel GetSpaceLocVel(XrSpace space, XrTime time);

    /// XR Input state overrides
    virtual void AttachActionSets();
    virtual void SyncActionSets(ovrApplFrameIn& in, XrFrameState& frameState);

    // Called to deal with lifetime
    void HandleSessionStateChanges(XrSessionState state);

   private:
    // Called one time when the application process starts.
    // Returns true if the application initialized successfully.
    bool Init(const xrJava* context);

    // Called on each session creation
    bool InitSession();

    // Called on each session end
    void EndSession();

    // Called one time when the applicatoin process exits
    void Shutdown(const xrJava* context);

    // Called to handle any lifecycle state changes. This will call
    // AppPaused() and AppResumed()
    void HandleLifecycle(const xrJava* context);

    // Events
    virtual void HandleXrEvents();

    // Internal Input
    void HandleInput(ovrApplFrameIn& in, XrFrameState& frameState);

    // Internal Render
    void RenderFrame(const ovrApplFrameIn& in, ovrRendererOutput& out);

   public:
    OVR::Vector4f BackgroundColor;
    bool FreeMove;

   protected:
    xrJava Context;
    ovrLifecycle Lifecycle = LIFECYCLE_UNKNOWN;
    ovrEgl Egl = {
        0,
        0,
        EGL_NO_DISPLAY,
        EGL_CAST(EGLConfig, 0),
        EGL_NO_SURFACE,
        EGL_NO_SURFACE,
        EGL_NO_CONTEXT};

    ANativeWindow* NativeWindow;
    bool Resumed;
    bool Focused;

    XrInstance Instance;
    XrSession Session;
    XrViewConfigurationProperties ViewportConfig;
    XrViewConfigurationView ViewConfigurationView[MAX_NUM_EYES];
    XrView Projections[MAX_NUM_EYES];
    XrSystemId SystemId;
    XrSpace HeadSpace;
    XrSpace LocalSpace;
    XrSpace StageSpace;
    XrSpace CurrentSpace;
    bool SessionActive;

    XrActionSet BaseActionSet = XR_NULL_HANDLE;
    XrPath LeftHandPath = XR_NULL_PATH;
    XrPath RightHandPath = XR_NULL_PATH;
    XrAction AimPoseAction = XR_NULL_HANDLE;
    XrAction GripPoseAction = XR_NULL_HANDLE;
    XrAction JoystickAction = XR_NULL_HANDLE;
    XrAction IndexTriggerAction = XR_NULL_HANDLE;
    XrAction IndexTriggerClickAction = XR_NULL_HANDLE;
    XrAction GripTriggerAction = XR_NULL_HANDLE;
    XrAction ButtonAAction = XR_NULL_HANDLE;
    XrAction ButtonBAction = XR_NULL_HANDLE;
    XrAction ButtonXAction = XR_NULL_HANDLE;
    XrAction ButtonYAction = XR_NULL_HANDLE;
    XrAction ButtonMenuAction = XR_NULL_HANDLE;
    /// common touch actions
    XrAction ThumbStickTouchAction = XR_NULL_HANDLE;
    XrAction ThumbRestTouchAction = XR_NULL_HANDLE;
    XrAction TriggerTouchAction = XR_NULL_HANDLE;

    XrSpace LeftControllerAimSpace = XR_NULL_HANDLE;
    XrSpace RightControllerAimSpace = XR_NULL_HANDLE;
    XrSpace LeftControllerGripSpace = XR_NULL_HANDLE;
    XrSpace RightControllerGripSpace = XR_NULL_HANDLE;
    uint32_t LastFrameAllButtons = 0u;
    uint32_t LastFrameAllTouches = 0u;

    OVRFW::ovrSurfaceRender SurfaceRender;
    OVRFW::OvrSceneView Scene;
    std::unique_ptr<OVRFW::ovrFileSys> FileSys;
    std::unique_ptr<OVRFW::ModelFile> SceneModel;

   private:
    uint64_t FrameIndex = 0;
    double DisplayTime = 0.0;
    int SwapInterval;
    int CpuLevel = CPU_LEVEL;
    int GpuLevel = GPU_LEVEL;
    int MainThreadTid;
    int RenderThreadTid;

    xrCompositorLayerUnion Layers[MAX_NUM_LAYERS];
    int LayerCount;

    ovrFramebuffer FrameBuffer[MAX_NUM_EYES];
    int NumFramebuffers = MAX_NUM_EYES;

    bool IsAppFocused = false;
    bool RunWhilePaused = false;
};

} // namespace OVRFW

#if defined(ANDROID)

#define ENTRY_POINT(appClass)                     \
    void android_main(struct android_app* app) {  \
        auto appl = std::make_unique<appClass>(); \
        appl->Run(app);                           \
    }

#else

#define ENTRY_POINT(appClass)                     \
    int main(int argv, const char** argc) {       \
        auto appl = std::make_unique<appClass>(); \
        appl->Run(argv, argc);                    \
    }

#endif // defined(ANDROID)
