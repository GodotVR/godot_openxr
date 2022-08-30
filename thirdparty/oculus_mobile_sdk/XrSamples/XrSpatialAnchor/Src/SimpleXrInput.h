// Simple Xr Input

#if defined(ANDROID)
#include <jni.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

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

class SimpleXrInput {
   public:
    enum Side {
        Side_Left = 0,
        Side_Right = 1,
    };

    enum ControllerSpace {
        Controller_Aim = 0,
        Controller_Grip = 1,
    };

    virtual ~SimpleXrInput() {}
    virtual void BeginSession(XrSession session_) = 0;
    virtual void EndSession() = 0;
    virtual void SyncActions() = 0;

    // Returns the pose that transforms the controller space into baseSpace
    virtual OVR::Posef FromControllerSpace(
        Side side,
        ControllerSpace controllerSpace,
        XrSpace baseSpace,
        XrTime atTime) = 0;

    virtual bool A() = 0;
    virtual bool B() = 0;
    virtual bool X() = 0;
    virtual bool Y() = 0;
};

SimpleXrInput* CreateSimpleXrInput(XrInstance instance_);
