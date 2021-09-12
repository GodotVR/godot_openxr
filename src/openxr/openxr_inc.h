#define XR_MND_BALL_ON_STICK_EXTENSION_NAME "XR_MNDX_ball_on_a_stick_controller"

#ifdef WIN32
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#elif ANDROID
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#else
#define XR_USE_PLATFORM_XLIB
#define XR_USE_GRAPHICS_API_OPENGL
#endif

#include <openxr/openxr.h>
