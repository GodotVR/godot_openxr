// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/*******************************************************************************

Filename    :   XrApp.h
Content     :   OpenXR application base class.
Created     :   July 2020
Authors     :   Federico Schliemann
Language    :   c++

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

#if defined(ANDROID)
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
#elif defined(WIN32)
#include "windows.h"
#endif // defined(ANDROID)

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
#define XR_USE_PLATFORM_WIN32 1
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

#if defined(DEBUG) || defined(_DEBUG)
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
#if defined(ANDROID)
    void Run(struct android_app* app);
#else
    void Run();
#endif // defined(ANDROID)

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

#if defined(ANDROID)
    void HandleAndroidCmd(struct android_app* app, int32_t cmd);
#endif // defined(ANDROID)

   protected:
    int GetNumFramebuffers() const {
        return NumFramebuffers;
    }
    ovrFramebuffer* GetFrameBuffer(int eye) {
        return &FrameBuffer[eye];
    }

    std::vector<XrExtensionProperties> GetXrExtensionProperties() const;

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
    virtual void SyncActionSets(ovrApplFrameIn& in);

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
    void HandleInput(ovrApplFrameIn& in);

    // Internal Render
    void RenderFrame(const ovrApplFrameIn& in, ovrRendererOutput& out);

   public:
    OVR::Vector4f BackgroundColor;
    bool FreeMove;

   protected:
    xrJava Context;
    ovrLifecycle Lifecycle = LIFECYCLE_UNKNOWN;
    ovrEgl Egl = {
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
        0,
        0,
        EGL_NO_DISPLAY,
        EGL_CAST(EGLConfig, 0),
        EGL_NO_SURFACE,
        EGL_NO_SURFACE,
        EGL_NO_CONTEXT
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    };

#if defined(ANDROID)
    ANativeWindow* NativeWindow;
    bool Resumed = false;
#endif // defined(ANDROID)
    bool ShouldExit = false;
    bool Focused = false;

    // When set the framework will skip calling
    // SyncActionSets(), this is useful if an app
    // wants control over xrSyncAction
    // Note: This means input in ovrApplFrameIn won't be set
    bool SkipSyncActions = false;

    XrInstance Instance = XR_NULL_HANDLE;
    XrSession Session = XR_NULL_HANDLE;
    XrViewConfigurationProperties ViewportConfig;
    XrViewConfigurationView ViewConfigurationView[MAX_NUM_EYES];
    XrView Projections[MAX_NUM_EYES];
    XrSystemId SystemId = XR_NULL_SYSTEM_ID;
    XrSpace HeadSpace = XR_NULL_HANDLE;
    XrSpace LocalSpace = XR_NULL_HANDLE;
    XrSpace StageSpace = XR_NULL_HANDLE;
    XrSpace CurrentSpace = XR_NULL_HANDLE;
    bool SessionActive = false;

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
    XrTime PrevDisplayTime = 0.0;
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

#elif defined(WIN32)

#define ENTRY_POINT(appClass)                               \
    __pragma(comment(linker, "/SUBSYSTEM:WINDOWS"));        \
    int APIENTRY WinMain(HINSTANCE, HINSTANCE, PSTR, int) { \
        auto appl = std::make_unique<appClass>();           \
        appl->Run();                                        \
        return 0;                                           \
    }

#else

#define ENTRY_POINT(appClass)                     \
    int main(int, const char**) {                 \
        auto appl = std::make_unique<appClass>(); \
        appl->Run();                              \
        return 0;                                 \
    }

#endif // defined(ANDROID)
