// Simple Xr Input

#include <jni.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <OVR_Math.h>

#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include <openxr/openxr.h>
#include <openxr/openxr_oculus.h>
#include <openxr/openxr_oculus_helpers.h>
#include <openxr/openxr_platform.h>

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

    // Whether the A button is pressed
    virtual bool IsButtonAPressed() const = 0;

    // Whether the B button is pressed
    virtual bool IsButtonBPressed() const = 0;

    // Whether the X button is pressed
    virtual bool IsButtonXPressed() const = 0;

    // Whether the Y button is pressed
    virtual bool IsButtonYPressed() const = 0;

    // Whether the left/right trigger is pressed
    virtual bool IsTriggerPressed(Side side) const = 0;

    // Whether the left/right thumb click is pressed
    virtual bool IsThumbClickPressed(Side side) const = 0;

    // Whether the left/right thumb stick is moved up
    virtual bool IsThumbStickUp() const = 0;

    // Whether the left/right thumb stick is moved down
    virtual bool IsThumbStickDown() const = 0;

    // Whether the left/right thumb stick is moved left
    virtual bool IsThumbStickLeft() const = 0;

    // Whether the left/right thumb stick is moved right
    virtual bool IsThumbStickRight() const = 0;
};

SimpleXrInput* CreateSimpleXrInput(XrInstance instance_);
