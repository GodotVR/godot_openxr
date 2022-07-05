/************************************************************************************

Filename  : XrPassthrough.cpp
Content   : This sample uses the Android NativeActivity class.
Created   :
Authors   :

Copyright : Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> // for memset
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h> // for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/native_window_jni.h> // for native window JNI
#include <android_native_app_glue.h>
#include <assert.h>

#include "XrPassthrough.h"
#include "XrPassthroughInput.h"
#include "XrPassthroughGl.h"

using namespace OVR;

#if !defined(EGL_OPENGL_ES3_BIT_KHR)
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

#define OVR_LOG_TAG "XrPassthrough"

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, OVR_LOG_TAG, __VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, OVR_LOG_TAG, __VA_ARGS__)

static const int CPU_LEVEL = 2;
static const int GPU_LEVEL = 3;
static const int NUM_MULTI_SAMPLES = 4;

__attribute__((unused)) static void LOG_POSE(const char* msg, const XrPosef* p) {
    ALOGV(
        "%s: orientation = { %f %f %f %f }, position = { %f %f %f }",
        msg,
        p->orientation.x,
        p->orientation.y,
        p->orientation.z,
        p->orientation.w,
        p->position.x,
        p->position.y,
        p->position.z);
}

/*
================================================================================

OpenXR Utility Functions

================================================================================
*/

XrInstance instance;
void OXR_CheckErrors(XrResult result, const char* function, bool failOnError) {
    if (XR_FAILED(result)) {
        char errorBuffer[XR_MAX_RESULT_STRING_SIZE];
        xrResultToString(instance, result, errorBuffer);
        if (failOnError) {
            ALOGE("OpenXR error: %s: %s\n", function, errorBuffer);
        } else {
            ALOGV("OpenXR error: %s: %s\n", function, errorBuffer);
        }
    }
}

#define DECL_PFN(pfn) PFN_##pfn pfn = nullptr
#define INIT_PFN(pfn) OXR(xrGetInstanceProcAddr(instance, #pfn, (PFN_xrVoidFunction*)(&pfn)))

// FB_passthrough sample begin
DECL_PFN(xrCreatePassthroughFB);
DECL_PFN(xrDestroyPassthroughFB);
DECL_PFN(xrPassthroughStartFB);
DECL_PFN(xrPassthroughPauseFB);
DECL_PFN(xrCreatePassthroughLayerFB);
DECL_PFN(xrDestroyPassthroughLayerFB);
DECL_PFN(xrPassthroughLayerSetStyleFB);
DECL_PFN(xrPassthroughLayerPauseFB);
DECL_PFN(xrPassthroughLayerResumeFB);
DECL_PFN(xrCreateTriangleMeshFB);
DECL_PFN(xrDestroyTriangleMeshFB);
DECL_PFN(xrTriangleMeshGetVertexBufferFB);
DECL_PFN(xrTriangleMeshGetIndexBufferFB);
DECL_PFN(xrTriangleMeshBeginUpdateFB);
DECL_PFN(xrTriangleMeshEndUpdateFB);
DECL_PFN(xrCreateGeometryInstanceFB);
DECL_PFN(xrDestroyGeometryInstanceFB);
DECL_PFN(xrGeometryInstanceSetTransformFB);
// FB_passthrough sample end

/*
================================================================================

Egl Utility Functions

================================================================================
*/

static const char* EglErrorString(const EGLint error) {
    switch (error) {
        case EGL_SUCCESS:
            return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:
            return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:
            return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:
            return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:
            return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT:
            return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG:
            return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE:
            return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:
            return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE:
            return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH:
            return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER:
            return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP:
            return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:
            return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST:
            return "EGL_CONTEXT_LOST";
        default:
            return "unknown";
    }
}

void Egl::Clear() {
    MajorVersion = 0;
    MinorVersion = 0;
    Display = 0;
    Config = 0;
    TinySurface = EGL_NO_SURFACE;
    MainSurface = EGL_NO_SURFACE;
    Context = EGL_NO_CONTEXT;
}

void Egl::CreateContext(const Egl* shareEgl) {
    if (Display != 0) {
        return;
    }

    Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    ALOGV("        eglInitialize( Display, &MajorVersion, &MinorVersion )");
    eglInitialize(Display, &MajorVersion, &MinorVersion);
    // Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
    // flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
    // settings, and that is completely wasted for our warp target.
    const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;
    if (eglGetConfigs(Display, configs, MAX_CONFIGS, &numConfigs) == EGL_FALSE) {
        ALOGE("        eglGetConfigs() failed: %s", EglErrorString(eglGetError()));
        return;
    }
    const EGLint configAttribs[] = {
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_ALPHA_SIZE,
        8, // need alpha for the multi-pass timewarp compositor
        EGL_DEPTH_SIZE,
        0,
        EGL_STENCIL_SIZE,
        0,
        EGL_SAMPLES,
        0,
        EGL_NONE};
    Config = 0;
    for (int i = 0; i < numConfigs; i++) {
        EGLint value = 0;

        eglGetConfigAttrib(Display, configs[i], EGL_RENDERABLE_TYPE, &value);
        if ((value & EGL_OPENGL_ES3_BIT_KHR) != EGL_OPENGL_ES3_BIT_KHR) {
            continue;
        }

        // The pbuffer config also needs to be compatible with normal window rendering
        // so it can share textures with the window context.
        eglGetConfigAttrib(Display, configs[i], EGL_SURFACE_TYPE, &value);
        if ((value & (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) != (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) {
            continue;
        }

        int j = 0;
        for (; configAttribs[j] != EGL_NONE; j += 2) {
            eglGetConfigAttrib(Display, configs[i], configAttribs[j], &value);
            if (value != configAttribs[j + 1]) {
                break;
            }
        }
        if (configAttribs[j] == EGL_NONE) {
            Config = configs[i];
            break;
        }
    }
    if (Config == 0) {
        ALOGE("        eglChooseConfig() failed: %s", EglErrorString(eglGetError()));
        return;
    }
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    ALOGV("        Context = eglCreateContext( Display, Config, EGL_NO_CONTEXT, contextAttribs )");
    Context = eglCreateContext(
        Display,
        Config,
        (shareEgl != nullptr) ? shareEgl->Context : EGL_NO_CONTEXT,
        contextAttribs);
    if (Context == EGL_NO_CONTEXT) {
        ALOGE("        eglCreateContext() failed: %s", EglErrorString(eglGetError()));
        return;
    }
    const EGLint surfaceAttribs[] = {EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE};
    ALOGV("        TinySurface = eglCreatePbufferSurface( Display, Config, surfaceAttribs )");
    TinySurface = eglCreatePbufferSurface(Display, Config, surfaceAttribs);
    if (TinySurface == EGL_NO_SURFACE) {
        ALOGE("        eglCreatePbufferSurface() failed: %s", EglErrorString(eglGetError()));
        eglDestroyContext(Display, Context);
        Context = EGL_NO_CONTEXT;
        return;
    }
    ALOGV("        eglMakeCurrent( Display, TinySurface, TinySurface, Context )");
    if (eglMakeCurrent(Display, TinySurface, TinySurface, Context) == EGL_FALSE) {
        ALOGE("        eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
        eglDestroySurface(Display, TinySurface);
        eglDestroyContext(Display, Context);
        Context = EGL_NO_CONTEXT;
        return;
    }
}

void Egl::DestroyContext() {
    if (Display != 0) {
        ALOGE("        eglMakeCurrent( Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )");
        if (eglMakeCurrent(Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE) {
            ALOGE("        eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
        }
    }
    if (Context != EGL_NO_CONTEXT) {
        ALOGE("        eglDestroyContext( Display, Context )");
        if (eglDestroyContext(Display, Context) == EGL_FALSE) {
            ALOGE("        eglDestroyContext() failed: %s", EglErrorString(eglGetError()));
        }
        Context = EGL_NO_CONTEXT;
    }
    if (TinySurface != EGL_NO_SURFACE) {
        ALOGE("        eglDestroySurface( Display, TinySurface )");
        if (eglDestroySurface(Display, TinySurface) == EGL_FALSE) {
            ALOGE("        eglDestroySurface() failed: %s", EglErrorString(eglGetError()));
        }
        TinySurface = EGL_NO_SURFACE;
    }
    if (Display != 0) {
        ALOGE("        eglTerminate( Display )");
        if (eglTerminate(Display) == EGL_FALSE) {
            ALOGE("        eglTerminate() failed: %s", EglErrorString(eglGetError()));
        }
        Display = 0;
    }
}

void App::Clear() {
    NativeWindow = NULL;
    Resumed = false;
    Focused = false;
    Instance = XR_NULL_HANDLE;
    Session = XR_NULL_HANDLE;
    ViewportConfig = {};
    for (int i = 0; i < NUM_EYES; i++) {
        ViewConfigurationView[i] = {};
    }
    SystemId = XR_NULL_SYSTEM_ID;
    HeadSpace = XR_NULL_HANDLE;
    LocalSpace = XR_NULL_HANDLE;
    StageSpace = XR_NULL_HANDLE;
    SessionActive = false;
    SwapInterval = 1;
    for (int i = 0; i < MaxLayerCount; i++) {
        Layers[i] = {};
    }
    LayerCount = 0;
    CpuLevel = 2;
    GpuLevel = 2;
    MainThreadTid = 0;
    RenderThreadTid = 0;
    TouchPadDownLastFrame = false;

    egl.Clear();
    appRenderer.Clear();
}

void App::HandleSessionStateChanges(XrSessionState state) {
    if (state == XR_SESSION_STATE_READY) {
        assert(Resumed);
        assert(NativeWindow != NULL);
        assert(SessionActive == false);

        XrSessionBeginInfo sessionBeginInfo = {};
        sessionBeginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
        sessionBeginInfo.next = nullptr;
        sessionBeginInfo.primaryViewConfigurationType = ViewportConfig.viewConfigurationType;

        XrResult result;
        OXR(result = xrBeginSession(Session, &sessionBeginInfo));

        SessionActive = (result == XR_SUCCESS);

        // Set session state once we have entered VR mode and have a valid session object.
        if (SessionActive) {
            XrPerfSettingsLevelEXT cpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
            switch (CpuLevel) {
                case 0:
                    cpuPerfLevel = XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT;
                    break;
                case 1:
                    cpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT;
                    break;
                case 2:
                    cpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
                    break;
                case 3:
                    cpuPerfLevel = XR_PERF_SETTINGS_LEVEL_BOOST_EXT;
                    break;
                default:
                    ALOGE("Invalid CPU level %d", CpuLevel);
                    break;
            }

            XrPerfSettingsLevelEXT gpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
            switch (GpuLevel) {
                case 0:
                    gpuPerfLevel = XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT;
                    break;
                case 1:
                    gpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT;
                    break;
                case 2:
                    gpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
                    break;
                case 3:
                    gpuPerfLevel = XR_PERF_SETTINGS_LEVEL_BOOST_EXT;
                    break;
                default:
                    ALOGE("Invalid GPU level %d", GpuLevel);
                    break;
            }

            PFN_xrPerfSettingsSetPerformanceLevelEXT pfnPerfSettingsSetPerformanceLevelEXT = NULL;
            OXR(xrGetInstanceProcAddr(
                Instance,
                "xrPerfSettingsSetPerformanceLevelEXT",
                (PFN_xrVoidFunction*)(&pfnPerfSettingsSetPerformanceLevelEXT)));

            OXR(pfnPerfSettingsSetPerformanceLevelEXT(
                Session, XR_PERF_SETTINGS_DOMAIN_CPU_EXT, cpuPerfLevel));
            OXR(pfnPerfSettingsSetPerformanceLevelEXT(
                Session, XR_PERF_SETTINGS_DOMAIN_GPU_EXT, gpuPerfLevel));

            PFN_xrSetAndroidApplicationThreadKHR pfnSetAndroidApplicationThreadKHR = NULL;
            OXR(xrGetInstanceProcAddr(
                Instance,
                "xrSetAndroidApplicationThreadKHR",
                (PFN_xrVoidFunction*)(&pfnSetAndroidApplicationThreadKHR)));

            OXR(pfnSetAndroidApplicationThreadKHR(
                Session, XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR, MainThreadTid));
            OXR(pfnSetAndroidApplicationThreadKHR(
                Session, XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR, RenderThreadTid));
        }
    } else if (state == XR_SESSION_STATE_STOPPING) {
        assert(Resumed == false);
        assert(SessionActive);

        OXR(xrEndSession(Session));
        SessionActive = false;
    }
}

void App::HandleXrEvents() {
    XrEventDataBuffer eventDataBuffer = {};

    // Poll for events
    for (;;) {
        XrEventDataBaseHeader* baseEventHeader = (XrEventDataBaseHeader*)(&eventDataBuffer);
        baseEventHeader->type = XR_TYPE_EVENT_DATA_BUFFER;
        baseEventHeader->next = NULL;
        XrResult r;
        OXR(r = xrPollEvent(Instance, &eventDataBuffer));
        if (r != XR_SUCCESS) {
            break;
        }

        switch (baseEventHeader->type) {
            case XR_TYPE_EVENT_DATA_EVENTS_LOST:
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_EVENTS_LOST event");
                break;
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING event");
                break;
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED event");
                break;
            case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT: {
                const XrEventDataPerfSettingsEXT* perf_settings_event =
                    (XrEventDataPerfSettingsEXT*)(baseEventHeader);
                ALOGV(
                    "xrPollEvent: received XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT event: type %d subdomain %d : level %d -> level %d",
                    perf_settings_event->type,
                    perf_settings_event->subDomain,
                    perf_settings_event->fromLevel,
                    perf_settings_event->toLevel);
            } break;
            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                ALOGV(
                    "xrPollEvent: received XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING event");
                break;
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                const XrEventDataSessionStateChanged* session_state_changed_event =
                    (XrEventDataSessionStateChanged*)(baseEventHeader);
                ALOGV(
                    "xrPollEvent: received XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: %d for session %p at time %f",
                    session_state_changed_event->state,
                    (void*)session_state_changed_event->session,
                    FromXrTime(session_state_changed_event->time));

                switch (session_state_changed_event->state) {
                    case XR_SESSION_STATE_FOCUSED:
                        Focused = true;
                        break;
                    case XR_SESSION_STATE_VISIBLE:
                        Focused = false;
                        break;
                    case XR_SESSION_STATE_READY:
                    case XR_SESSION_STATE_STOPPING:
                        HandleSessionStateChanges(session_state_changed_event->state);
                        break;
                    default:
                        break;
                }
            } break;
            default:
                ALOGV("xrPollEvent: Unknown event");
                break;
        }
    }
}

/*
================================================================================

Native Activity

================================================================================
*/

/**
 * Process the next main command.
 */
static void app_handle_cmd(struct android_app* androidApp, int32_t cmd) {
    App& app = *(App*)androidApp->userData;

    switch (cmd) {
        // There is no APP_CMD_CREATE. The ANativeActivity creates the
        // application thread from onCreate(). The application thread
        // then calls android_main().
        case APP_CMD_START: {
            ALOGV("onStart()");
            ALOGV("    APP_CMD_START");
            break;
        }
        case APP_CMD_RESUME: {
            ALOGV("onResume()");
            ALOGV("    APP_CMD_RESUME");
            app.Resumed = true;
            break;
        }
        case APP_CMD_PAUSE: {
            ALOGV("onPause()");
            ALOGV("    APP_CMD_PAUSE");
            app.Resumed = false;
            break;
        }
        case APP_CMD_STOP: {
            ALOGV("onStop()");
            ALOGV("    APP_CMD_STOP");
            break;
        }
        case APP_CMD_DESTROY: {
            ALOGV("onDestroy()");
            ALOGV("    APP_CMD_DESTROY");
            app.NativeWindow = NULL;
            break;
        }
        case APP_CMD_INIT_WINDOW: {
            ALOGV("surfaceCreated()");
            ALOGV("    APP_CMD_INIT_WINDOW");
            app.NativeWindow = androidApp->window;
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            ALOGV("surfaceDestroyed()");
            ALOGV("    APP_CMD_TERM_WINDOW");
            app.NativeWindow = NULL;
            break;
        }
    }
}

void UpdateStageBounds(App& app) {
    XrExtent2Df stageBounds = {};

    XrResult result;
    OXR(result = xrGetReferenceSpaceBoundsRect(
            app.Session, XR_REFERENCE_SPACE_TYPE_STAGE, &stageBounds));
    if (result != XR_SUCCESS) {
        ALOGV("Stage bounds query failed: using small defaults");
        stageBounds.width = 1.0f;
        stageBounds.height = 1.0f;
    }

    app.StageBounds = Vector3f(stageBounds.width * 0.5f, 1.0f, stageBounds.height * 0.5f);
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* androidApp) {
    ALOGV("----------------------------------------------------------------");
    ALOGV("android_app_entry()");
    ALOGV("    android_main()");

    JNIEnv* Env;
    (*androidApp->activity->vm).AttachCurrentThread(&Env, nullptr);

    // Note that AttachCurrentThread will reset the thread name.
    prctl(PR_SET_NAME, (long)"OVR::Main", 0, 0, 0);

    App app;
    app.Clear();

    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
    xrGetInstanceProcAddr(
        XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)&xrInitializeLoaderKHR);
    if (xrInitializeLoaderKHR != NULL) {
        XrLoaderInitInfoAndroidKHR loaderInitializeInfoAndroid;
        memset(&loaderInitializeInfoAndroid, 0, sizeof(loaderInitializeInfoAndroid));
        loaderInitializeInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
        loaderInitializeInfoAndroid.next = NULL;
        loaderInitializeInfoAndroid.applicationVM = androidApp->activity->vm;
        loaderInitializeInfoAndroid.applicationContext = androidApp->activity->clazz;
        xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR*)&loaderInitializeInfoAndroid);
    }

    // Log available layers.
    {
        XrResult result;

        PFN_xrEnumerateApiLayerProperties xrEnumerateApiLayerProperties;
        OXR(result = xrGetInstanceProcAddr(
                XR_NULL_HANDLE,
                "xrEnumerateApiLayerProperties",
                (PFN_xrVoidFunction*)&xrEnumerateApiLayerProperties));
        if (result != XR_SUCCESS) {
            ALOGE("Failed to get xrEnumerateApiLayerProperties function pointer.");
            exit(1);
        }

        uint32_t numInputLayers = 0;
        uint32_t numOutputLayers = 0;
        OXR(xrEnumerateApiLayerProperties(numInputLayers, &numOutputLayers, NULL));

        numInputLayers = numOutputLayers;

        auto layerProperties = new XrApiLayerProperties[numOutputLayers];

        for (uint32_t i = 0; i < numOutputLayers; i++) {
            layerProperties[i].type = XR_TYPE_API_LAYER_PROPERTIES;
            layerProperties[i].next = NULL;
        }

        OXR(xrEnumerateApiLayerProperties(numInputLayers, &numOutputLayers, layerProperties));

        for (uint32_t i = 0; i < numOutputLayers; i++) {
            ALOGV("Found layer %s", layerProperties[i].layerName);
        }

        delete[] layerProperties;
    }

    // Check that the extensions required are present.
    const char* const requiredExtensionNames[] = {
        XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
        XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME,
        XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME,
        XR_FB_PASSTHROUGH_EXTENSION_NAME,
        XR_FB_TRIANGLE_MESH_EXTENSION_NAME};
    const uint32_t numRequiredExtensions =
        sizeof(requiredExtensionNames) / sizeof(requiredExtensionNames[0]);

    // Check the list of required extensions against what is supported by the runtime.
    {
        XrResult result;
        PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties;
        OXR(result = xrGetInstanceProcAddr(
                XR_NULL_HANDLE,
                "xrEnumerateInstanceExtensionProperties",
                (PFN_xrVoidFunction*)&xrEnumerateInstanceExtensionProperties));
        if (result != XR_SUCCESS) {
            ALOGE("Failed to get xrEnumerateInstanceExtensionProperties function pointer.");
            exit(1);
        }

        uint32_t numInputExtensions = 0;
        uint32_t numOutputExtensions = 0;
        OXR(xrEnumerateInstanceExtensionProperties(
            NULL, numInputExtensions, &numOutputExtensions, NULL));
        ALOGV("xrEnumerateInstanceExtensionProperties found %u extension(s).", numOutputExtensions);

        numInputExtensions = numOutputExtensions;

        auto extensionProperties = new XrExtensionProperties[numOutputExtensions];

        for (uint32_t i = 0; i < numOutputExtensions; i++) {
            extensionProperties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
            extensionProperties[i].next = NULL;
        }

        OXR(xrEnumerateInstanceExtensionProperties(
            NULL, numInputExtensions, &numOutputExtensions, extensionProperties));
        for (uint32_t i = 0; i < numOutputExtensions; i++) {
            ALOGV("Extension #%d = '%s'.", i, extensionProperties[i].extensionName);
        }

        for (uint32_t i = 0; i < numRequiredExtensions; i++) {
            bool found = false;
            for (uint32_t j = 0; j < numOutputExtensions; j++) {
                if (!strcmp(requiredExtensionNames[i], extensionProperties[j].extensionName)) {
                    ALOGV("Found required extension %s", requiredExtensionNames[i]);
                    found = true;
                    break;
                }
            }
            if (!found) {
                ALOGE("Failed to find required extension %s", requiredExtensionNames[i]);
                exit(1);
            }
        }

        delete[] extensionProperties;
    }

    // Create the OpenXR instance.
    XrApplicationInfo appInfo = {};
    strcpy(appInfo.applicationName, "XrPassthrough");
    appInfo.applicationVersion = 0;
    strcpy(appInfo.engineName, "Oculus Mobile Sample");
    appInfo.engineVersion = 0;
    appInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.next = nullptr;
    instanceCreateInfo.createFlags = 0;
    instanceCreateInfo.applicationInfo = appInfo;
    instanceCreateInfo.enabledApiLayerCount = 0;
    instanceCreateInfo.enabledApiLayerNames = NULL;
    instanceCreateInfo.enabledExtensionCount = numRequiredExtensions;
    instanceCreateInfo.enabledExtensionNames = requiredExtensionNames;

    XrResult initResult;
    OXR(initResult = xrCreateInstance(&instanceCreateInfo, &app.Instance));
    if (initResult != XR_SUCCESS) {
        ALOGE("Failed to create XR app.Instance: %d.", initResult);
        exit(1);
    }
    // Set the global used in macros
    instance = app.Instance;

    XrInstanceProperties instanceInfo;
    instanceInfo.type = XR_TYPE_INSTANCE_PROPERTIES;
    instanceInfo.next = NULL;
    OXR(xrGetInstanceProperties(app.Instance, &instanceInfo));
    ALOGV(
        "Runtime %s: Version : %u.%u.%u",
        instanceInfo.runtimeName,
        XR_VERSION_MAJOR(instanceInfo.runtimeVersion),
        XR_VERSION_MINOR(instanceInfo.runtimeVersion),
        XR_VERSION_PATCH(instanceInfo.runtimeVersion));

    XrSystemGetInfo systemGetInfo = {};
    systemGetInfo.type = XR_TYPE_SYSTEM_GET_INFO;
    systemGetInfo.next = NULL;
    systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId systemId;
    OXR(initResult = xrGetSystem(app.Instance, &systemGetInfo, &systemId));
    if (initResult != XR_SUCCESS) {
        ALOGE("Failed to get system.");
        exit(1);
    }

    XrSystemProperties systemProperties = {};
    systemProperties.type = XR_TYPE_SYSTEM_PROPERTIES;
    OXR(xrGetSystemProperties(app.Instance, systemId, &systemProperties));

    ALOGV(
        "System Properties: Name=%s VendorId=%x",
        systemProperties.systemName,
        systemProperties.vendorId);
    ALOGV(
        "System Graphics Properties: MaxWidth=%d MaxHeight=%d MaxLayers=%d",
        systemProperties.graphicsProperties.maxSwapchainImageWidth,
        systemProperties.graphicsProperties.maxSwapchainImageHeight,
        systemProperties.graphicsProperties.maxLayerCount);
    ALOGV(
        "System Tracking Properties: OrientationTracking=%s PositionTracking=%s",
        systemProperties.trackingProperties.orientationTracking ? "True" : "False",
        systemProperties.trackingProperties.positionTracking ? "True" : "False");

    assert(MaxLayerCount <= systemProperties.graphicsProperties.maxLayerCount);

    // Get the graphics requirements.
    PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = NULL;
    OXR(xrGetInstanceProcAddr(
        app.Instance,
        "xrGetOpenGLESGraphicsRequirementsKHR",
        (PFN_xrVoidFunction*)(&pfnGetOpenGLESGraphicsRequirementsKHR)));

    XrGraphicsRequirementsOpenGLESKHR graphicsRequirements = {};
    graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR;
    OXR(pfnGetOpenGLESGraphicsRequirementsKHR(app.Instance, systemId, &graphicsRequirements));

    // Create the EGL Context
    app.egl.CreateContext(nullptr);

    // Check the graphics requirements.
    int eglMajor = 0;
    int eglMinor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &eglMajor);
    glGetIntegerv(GL_MINOR_VERSION, &eglMinor);
    const XrVersion eglVersion = XR_MAKE_VERSION(eglMajor, eglMinor, 0);
    if (eglVersion < graphicsRequirements.minApiVersionSupported ||
        eglVersion > graphicsRequirements.maxApiVersionSupported) {
        ALOGE("GLES version %d.%d not supported", eglMajor, eglMinor);
        exit(0);
    }

    app.CpuLevel = CPU_LEVEL;
    app.GpuLevel = GPU_LEVEL;
    app.MainThreadTid = gettid();

    app.SystemId = systemId;

    // FB_passthrough sample begin
    INIT_PFN(xrCreatePassthroughFB);
    INIT_PFN(xrDestroyPassthroughFB);
    INIT_PFN(xrPassthroughStartFB);
    INIT_PFN(xrPassthroughPauseFB);
    INIT_PFN(xrCreatePassthroughLayerFB);
    INIT_PFN(xrDestroyPassthroughLayerFB);
    INIT_PFN(xrPassthroughLayerSetStyleFB);
    INIT_PFN(xrPassthroughLayerPauseFB);
    INIT_PFN(xrPassthroughLayerResumeFB);
    INIT_PFN(xrCreateTriangleMeshFB);
    INIT_PFN(xrDestroyTriangleMeshFB);
    INIT_PFN(xrTriangleMeshGetVertexBufferFB);
    INIT_PFN(xrTriangleMeshGetIndexBufferFB);
    INIT_PFN(xrTriangleMeshBeginUpdateFB);
    INIT_PFN(xrTriangleMeshEndUpdateFB);
    INIT_PFN(xrCreateGeometryInstanceFB);
    INIT_PFN(xrDestroyGeometryInstanceFB);
    INIT_PFN(xrGeometryInstanceSetTransformFB);
    // FB_passthrough sample end

    // Create the OpenXR Session.
    XrGraphicsBindingOpenGLESAndroidKHR graphicsBindingAndroidGLES = {};
    graphicsBindingAndroidGLES.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR;
    graphicsBindingAndroidGLES.next = NULL;
    graphicsBindingAndroidGLES.display = app.egl.Display;
    graphicsBindingAndroidGLES.config = app.egl.Config;
    graphicsBindingAndroidGLES.context = app.egl.Context;

    XrSessionCreateInfo sessionCreateInfo = {};
    sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
    sessionCreateInfo.next = &graphicsBindingAndroidGLES;
    sessionCreateInfo.createFlags = 0;
    sessionCreateInfo.systemId = app.SystemId;

    OXR(initResult = xrCreateSession(app.Instance, &sessionCreateInfo, &app.Session));
    if (initResult != XR_SUCCESS) {
        ALOGE("Failed to create XR session: %d.", initResult);
        exit(1);
    }

    // App only supports the primary stereo view config.
    const XrViewConfigurationType supportedViewConfigType =
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    // Enumerate the viewport configurations.
    uint32_t viewportConfigTypeCount = 0;
    OXR(xrEnumerateViewConfigurations(
        app.Instance, app.SystemId, 0, &viewportConfigTypeCount, NULL));

    auto viewportConfigurationTypes = new XrViewConfigurationType[viewportConfigTypeCount];

    OXR(xrEnumerateViewConfigurations(
        app.Instance,
        app.SystemId,
        viewportConfigTypeCount,
        &viewportConfigTypeCount,
        viewportConfigurationTypes));

    ALOGV("Available Viewport Configuration Types: %d", viewportConfigTypeCount);

    for (uint32_t i = 0; i < viewportConfigTypeCount; i++) {
        const XrViewConfigurationType viewportConfigType = viewportConfigurationTypes[i];

        ALOGV(
            "Viewport configuration type %d : %s",
            viewportConfigType,
            viewportConfigType == supportedViewConfigType ? "Selected" : "");

        XrViewConfigurationProperties viewportConfig;
        viewportConfig.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
        OXR(xrGetViewConfigurationProperties(
            app.Instance, app.SystemId, viewportConfigType, &viewportConfig));
        ALOGV(
            "FovMutable=%s ConfigurationType %d",
            viewportConfig.fovMutable ? "true" : "false",
            viewportConfig.viewConfigurationType);

        uint32_t viewCount;
        OXR(xrEnumerateViewConfigurationViews(
            app.Instance, app.SystemId, viewportConfigType, 0, &viewCount, NULL));

        if (viewCount > 0) {
            auto elements = new XrViewConfigurationView[viewCount];

            for (uint32_t e = 0; e < viewCount; e++) {
                elements[e].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
                elements[e].next = NULL;
            }

            OXR(xrEnumerateViewConfigurationViews(
                app.Instance, app.SystemId, viewportConfigType, viewCount, &viewCount, elements));

            // Log the view config info for each view type for debugging purposes.
            for (uint32_t e = 0; e < viewCount; e++) {
                const XrViewConfigurationView* element = &elements[e];

                ALOGV(
                    "Viewport [%d]: Recommended Width=%d Height=%d SampleCount=%d",
                    e,
                    element->recommendedImageRectWidth,
                    element->recommendedImageRectHeight,
                    element->recommendedSwapchainSampleCount);

                ALOGV(
                    "Viewport [%d]: Max Width=%d Height=%d SampleCount=%d",
                    e,
                    element->maxImageRectWidth,
                    element->maxImageRectHeight,
                    element->maxSwapchainSampleCount);
            }

            // Cache the view config properties for the selected config type.
            if (viewportConfigType == supportedViewConfigType) {
                assert(viewCount == NUM_EYES);
                for (uint32_t e = 0; e < viewCount; e++) {
                    app.ViewConfigurationView[e] = elements[e];
                }
            }

            delete[] elements;
        } else {
            ALOGE("Empty viewport configuration type: %d", viewCount);
        }
    }

    delete[] viewportConfigurationTypes;

    // Get the viewport configuration info for the chosen viewport configuration type.
    app.ViewportConfig.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;

    OXR(xrGetViewConfigurationProperties(
        app.Instance, app.SystemId, supportedViewConfigType, &app.ViewportConfig));

    bool stageSupported = false;

    uint32_t numOutputSpaces = 0;
    OXR(xrEnumerateReferenceSpaces(app.Session, 0, &numOutputSpaces, NULL));

    auto referenceSpaces = new XrReferenceSpaceType[numOutputSpaces];

    OXR(xrEnumerateReferenceSpaces(
        app.Session, numOutputSpaces, &numOutputSpaces, referenceSpaces));

    for (uint32_t i = 0; i < numOutputSpaces; i++) {
        if (referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE) {
            stageSupported = true;
            break;
        }
    }

    delete[] referenceSpaces;

    // Create a space to the first path
    XrReferenceSpaceCreateInfo spaceCreateInfo = {};
    spaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
    OXR(xrCreateReferenceSpace(app.Session, &spaceCreateInfo, &app.HeadSpace));

    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    OXR(xrCreateReferenceSpace(app.Session, &spaceCreateInfo, &app.LocalSpace));

    if (stageSupported) {
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        spaceCreateInfo.poseInReferenceSpace.position.y = 0.0f;
        OXR(xrCreateReferenceSpace(app.Session, &spaceCreateInfo, &app.StageSpace));
        ALOGV("Created stage space");
    }

    auto projections = new XrView[NUM_EYES];

    GLenum format = GL_SRGB8_ALPHA8;
    int width = app.ViewConfigurationView[0].recommendedImageRectWidth;
    int height = app.ViewConfigurationView[0].recommendedImageRectHeight;

    XrSwapchainCreateInfo swapChainCreateInfo = {};
    swapChainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
    swapChainCreateInfo.usageFlags =
        XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.format = format;
    swapChainCreateInfo.sampleCount = 1;
    swapChainCreateInfo.width = width;
    swapChainCreateInfo.height = height;
    swapChainCreateInfo.faceCount = 1;
    swapChainCreateInfo.arraySize = 2;
    swapChainCreateInfo.mipCount = 1;

    // Create the swapchain.
    OXR(xrCreateSwapchain(app.Session, &swapChainCreateInfo, &app.ColorSwapChain));
    OXR(xrEnumerateSwapchainImages(app.ColorSwapChain, 0, &app.SwapChainLength, nullptr));
    auto images = new XrSwapchainImageOpenGLESKHR[app.SwapChainLength];
    // Populate the swapchain image array.
    for (uint32_t i = 0; i < app.SwapChainLength; i++) {
        images[i] = {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR};
    }

    OXR(xrEnumerateSwapchainImages(
        app.ColorSwapChain,
        app.SwapChainLength,
        &app.SwapChainLength,
        (XrSwapchainImageBaseHeader*)images));

    auto colorTextures = new GLuint[app.SwapChainLength];
    for (uint32_t i = 0; i < app.SwapChainLength; i++) {
        colorTextures[i] = GLuint(images[i].image);
    }

    app.appRenderer.Create(
        format, width, height, NUM_MULTI_SAMPLES, app.SwapChainLength, colorTextures);

    delete[] images;
    delete[] colorTextures;

    AppInput_init(app);

    // FB_passthrough sample begin
    // Create passthrough objects
    XrPassthroughFB passthrough = XR_NULL_HANDLE;
    XrPassthroughLayerFB passthroughLayer = XR_NULL_HANDLE;
    XrPassthroughLayerFB reconPassthroughLayer = XR_NULL_HANDLE;
    XrPassthroughLayerFB geomPassthroughLayer = XR_NULL_HANDLE;
    XrGeometryInstanceFB geomInstance = XR_NULL_HANDLE;
    {
        XrPassthroughCreateInfoFB ptci = {XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
        XrResult result;
        OXR(result = xrCreatePassthroughFB(app.Session, &ptci, &passthrough));

        if (XR_SUCCEEDED(result)) {
            XrPassthroughLayerCreateInfoFB plci = {XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
            plci.passthrough = passthrough;
            plci.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
            OXR(xrCreatePassthroughLayerFB(app.Session, &plci, &reconPassthroughLayer));
        }

        if (XR_SUCCEEDED(result)) {
            XrPassthroughLayerCreateInfoFB plci = {XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
            plci.passthrough = passthrough;
            plci.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_PROJECTED_FB;
            OXR(xrCreatePassthroughLayerFB(app.Session, &plci, &geomPassthroughLayer));

            const XrVector3f verts[] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}};
            const uint32_t indexes[] = {0, 1, 2, 2, 1, 3};
            XrTriangleMeshCreateInfoFB tmci = {XR_TYPE_TRIANGLE_MESH_CREATE_INFO_FB};
            tmci.vertexCount = 4;
            tmci.vertexBuffer = &verts[0];
            tmci.triangleCount = 2;
            tmci.indexBuffer = &indexes[0];

            XrTriangleMeshFB mesh = XR_NULL_HANDLE;
            OXR(xrCreateTriangleMeshFB(app.Session, &tmci, &mesh));

            XrGeometryInstanceCreateInfoFB gici = {XR_TYPE_GEOMETRY_INSTANCE_CREATE_INFO_FB};
            gici.layer = geomPassthroughLayer;
            gici.mesh = mesh;
            gici.baseSpace = app.LocalSpace;
            gici.pose.orientation.w = 1.0f;
            gici.scale = {1.0f, 1.0f, 1.0f};
            OXR(xrCreateGeometryInstanceFB(app.Session, &gici, &geomInstance));
        }
    }
    // FB_passthrough sample end

    androidApp->userData = &app;
    androidApp->onAppCmd = app_handle_cmd;

    bool stageBoundsDirty = true;

    double startTimeInSeconds = -1.0;

    int frameCount = -1;
    int framesCyclePaused = 0;
    bool cyclePaused = false;

    constexpr int framesPerMode = 400;

    enum Mode {
        Mode_Passthrough_Basic = 0,
        Mode_Passthrough_DynamicRamp = 1,
        Mode_Passthrough_GreenRampYellowEdges = 2,
        Mode_Passthrough_Masked = 3,
        Mode_Passthrough_ProjQuad = 4,
        Mode_Passthrough_Stopped = 5,
        Mode_NumModes = 6
    };

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.2f};

    while (androidApp->destroyRequested == 0) {
        frameCount++;

        // Read all pending events.
        for (;;) {
            int events;
            struct android_poll_source* source;
            // If the timeout is zero, returns immediately without blocking.
            // If the timeout is negative, waits indefinitely until an event appears.
            const int timeoutMilliseconds = (app.Resumed == false && app.SessionActive == false &&
                                             androidApp->destroyRequested == 0)
                ? -1
                : 0;
            if (ALooper_pollAll(timeoutMilliseconds, NULL, &events, (void**)&source) < 0) {
                break;
            }

            // Process this event.
            if (source != NULL) {
                source->process(androidApp, source);
            }
        }

        app.HandleXrEvents();

        if (app.SessionActive == false) {
            frameCount = -1;
            framesCyclePaused = 0;
            continue;
        }

        AppInput_syncActions(app);
        if (boolState.type != 0 && boolState.changedSinceLastSync == XR_TRUE &&
            boolState.currentState != XR_FALSE) {
            cyclePaused = !cyclePaused;
        }

        if (cyclePaused) {
            clearColor[0] = 0.3f;
            framesCyclePaused++;
        } else {
            clearColor[0] = 0.0f;
        }
        app.appRenderer.scene.SetClearColor(clearColor);

        // Create the scene if not yet created.
        // The scene is created here to be able to show a loading icon.
        if (!app.appRenderer.scene.IsCreated()) {
            // Create the scene.
            app.appRenderer.scene.Create();
        }

        if (stageBoundsDirty) {
            UpdateStageBounds(app);
            stageBoundsDirty = false;
        }

        // NOTE: OpenXR does not use the concept of frame indices. Instead,
        // XrWaitFrame returns the predicted display time.
        XrFrameWaitInfo waitFrameInfo = {};
        waitFrameInfo.type = XR_TYPE_FRAME_WAIT_INFO;
        waitFrameInfo.next = NULL;

        XrFrameState frameState = {};
        frameState.type = XR_TYPE_FRAME_STATE;
        frameState.next = NULL;

        OXR(xrWaitFrame(app.Session, &waitFrameInfo, &frameState));

        // Get the HMD pose, predicted for the middle of the time period during which
        // the new eye images will be displayed. The number of frames predicted ahead
        // depends on the pipeline depth of the engine and the synthesis rate.
        // The better the prediction, the less black will be pulled in at the edges.
        XrFrameBeginInfo beginFrameDesc = {};
        beginFrameDesc.type = XR_TYPE_FRAME_BEGIN_INFO;
        beginFrameDesc.next = NULL;
        OXR(xrBeginFrame(app.Session, &beginFrameDesc));

        XrPosef xfLocalFromHead;
        {
            XrSpaceLocation loc = {XR_TYPE_SPACE_LOCATION};
            OXR(xrLocateSpace(
                app.HeadSpace, app.LocalSpace, frameState.predictedDisplayTime, &loc));
            xfLocalFromHead = loc.pose;
        }

        XrViewState viewState = {XR_TYPE_VIEW_STATE, NULL};

        XrViewLocateInfo projectionInfo = {};
        projectionInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
        projectionInfo.viewConfigurationType = app.ViewportConfig.viewConfigurationType;
        projectionInfo.displayTime = frameState.predictedDisplayTime;
        projectionInfo.space = app.HeadSpace;

        uint32_t projectionCapacityInput = NUM_EYES;
        uint32_t projectionCountOutput = projectionCapacityInput;

        OXR(xrLocateViews(
            app.Session,
            &projectionInfo,
            &viewState,
            projectionCapacityInput,
            &projectionCountOutput,
            projections));

        // update input information
        XrSpace controllerSpace[] = {
            leftControllerAimSpace,
            leftControllerGripSpace,
            rightControllerAimSpace,
            rightControllerGripSpace,
        };

        bool controllerActive[] = {leftControllerActive, rightControllerActive};
        for (int i = 0; i < 4; i++) {
            if (controllerActive[i >> 1]) {
                XrSpaceLocation loc = {XR_TYPE_SPACE_LOCATION};
                OXR(xrLocateSpace(
                    controllerSpace[i], app.LocalSpace, frameState.predictedDisplayTime, &loc));
                app.appRenderer.scene.trackedController[i].Active =
                    (loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;
                app.appRenderer.scene.trackedController[i].Pose = OvrFromXr(loc.pose);
            } else {
                app.appRenderer.scene.trackedController[i].Clear();
            }
        }

        // Simple animation
        double timeInSeconds = FromXrTime(frameState.predictedDisplayTime);
        if (startTimeInSeconds < 0.0) {
            startTimeInSeconds = timeInSeconds;
        }

        // FB_passthrough sample begin
        // Cycle through passthrough operation / display modes
        const Mode prevMode = Mode(
            ((frameCount - framesCyclePaused + (framesPerMode * Mode_NumModes) - 1) /
             framesPerMode) %
            Mode_NumModes);
        const Mode mode = Mode(((frameCount - framesCyclePaused) / framesPerMode) % Mode_NumModes);

        if (mode != prevMode) {
            // Unset any sticky state from the previous mode
            switch (prevMode) {
                case Mode_Passthrough_Basic:
                    OXR(xrPassthroughLayerPauseFB(passthroughLayer));
                    break;
                case Mode_Passthrough_DynamicRamp:
                    OXR(xrPassthroughLayerPauseFB(passthroughLayer));
                    break;
                case Mode_Passthrough_GreenRampYellowEdges:
                    OXR(xrPassthroughLayerPauseFB(passthroughLayer));
                    break;
                case Mode_Passthrough_Masked:
                    OXR(xrPassthroughLayerPauseFB(passthroughLayer));
                    clearColor[3] = 0.2f;
                    break;
                case Mode_Passthrough_ProjQuad:
                    OXR(xrPassthroughLayerPauseFB(passthroughLayer));
                    break;
                case Mode_Passthrough_Stopped:
                    OXR(xrPassthroughStartFB(passthrough));
                    break;
                default:
                    break;
            }
            XrPassthroughStyleFB style{XR_TYPE_PASSTHROUGH_STYLE_FB};
            switch (mode) {
                case Mode_Passthrough_Basic:
                    passthroughLayer = reconPassthroughLayer;
                    OXR(xrPassthroughLayerResumeFB(passthroughLayer));
                    style.textureOpacityFactor = 0.5f;
                    style.edgeColor = {0.0f, 0.0f, 0.0f, 0.0f};
                    OXR(xrPassthroughLayerSetStyleFB(passthroughLayer, &style));
                    break;
                case Mode_Passthrough_DynamicRamp:
                    passthroughLayer = reconPassthroughLayer;
                    OXR(xrPassthroughLayerResumeFB(passthroughLayer));
                    style.textureOpacityFactor = 0.5f;
                    style.edgeColor = {0.0f, 0.0f, 0.0f, 0.0f};
                    OXR(xrPassthroughLayerSetStyleFB(passthroughLayer, &style));
                    break;
                case Mode_Passthrough_GreenRampYellowEdges: {
                    passthroughLayer = reconPassthroughLayer;
                    OXR(xrPassthroughLayerResumeFB(passthroughLayer));
                    // Create a color map which maps each input value to a green ramp
                    XrPassthroughColorMapMonoToRgbaFB colorMap = {
                        XR_TYPE_PASSTHROUGH_COLOR_MAP_MONO_TO_RGBA_FB};
                    for (int i = 0; i < XR_PASSTHROUGH_COLOR_MAP_MONO_SIZE_FB; ++i) {
                        float colorValue = i / 255.0f;
                        colorMap.textureColorMap[i] = {0.0f, colorValue, 0.0f, 1.0f};
                    }
                    style.textureOpacityFactor = 0.5f;
                    style.edgeColor = {1.0f, 1.0f, 0.0f, 0.5f};
                    style.next = &colorMap;
                    OXR(xrPassthroughLayerSetStyleFB(passthroughLayer, &style));
                } break;
                case Mode_Passthrough_Masked:
                    passthroughLayer = reconPassthroughLayer;
                    OXR(xrPassthroughLayerResumeFB(passthroughLayer));
                    clearColor[3] = 1.0f;
                    style.textureOpacityFactor = 0.5f;
                    style.edgeColor = {0.0f, 0.0f, 0.0f, 0.0f};
                    OXR(xrPassthroughLayerSetStyleFB(passthroughLayer, &style));
                    break;
                case Mode_Passthrough_ProjQuad:
                    passthroughLayer = geomPassthroughLayer;
                    OXR(xrPassthroughLayerResumeFB(passthroughLayer));
                    break;
                case Mode_Passthrough_Stopped:
                    OXR(xrPassthroughPauseFB(passthrough));
                    break;
                default:
                    break;
            }
        }
        // per frame style adjustments for DynamicRamp
        if (mode == Mode_Passthrough_DynamicRamp) {
            const float frac =
                ((frameCount - framesCyclePaused) % framesPerMode) / (framesPerMode - 1.0f);
            // phase shifting color map
            XrPassthroughColorMapMonoToRgbaFB colorMap = {
                XR_TYPE_PASSTHROUGH_COLOR_MAP_MONO_TO_RGBA_FB};
            for (int i = 0; i < XR_PASSTHROUGH_COLOR_MAP_MONO_SIZE_FB; ++i) {
                float v = i / 64.0f;
                v -= floor(v) - 0.5f;
                v = fabsf(v);
                v = v * v;
                int idx = (i + int(4 * frac * XR_PASSTHROUGH_COLOR_MAP_MONO_SIZE_FB)) %
                    XR_PASSTHROUGH_COLOR_MAP_MONO_SIZE_FB;
                colorMap.textureColorMap[idx] = {v, v, v, 1.0f};
            }
            XrPassthroughStyleFB style{XR_TYPE_PASSTHROUGH_STYLE_FB};
            style.textureOpacityFactor = 0.5f;
            style.edgeColor = {0.0f, 0.0f, 0.0f, 0.0f};
            style.next = &colorMap;
            OXR(xrPassthroughLayerSetStyleFB(passthroughLayer, &style));
        } else if (mode == Mode_Passthrough_ProjQuad && leftControllerGripSpace != XR_NULL_HANDLE) {
            XrGeometryInstanceTransformFB git = {XR_TYPE_GEOMETRY_INSTANCE_TRANSFORM_FB};
            git.baseSpace = leftControllerGripSpace;
            git.time = frameState.predictedDisplayTime;
            // Approximate ring orientation relative to grip
            Quatf rx(Vector3f(1, 0, 0), DegreeToRad(-20.0f));
            Quatf ry(Vector3f(0, 1, 0), DegreeToRad(-11.0f));
            Quatf rz(Vector3f(0, 0, 1), DegreeToRad(0.0f));
            Quatf r = rz * ry * rx;
            git.pose.orientation = {r.x, r.y, r.z, r.w};
            git.pose.position = {0.05f, -0.15f, -0.05f};
            git.scale = {0.6f, 0.3f, 1.0f};
            OXR(xrGeometryInstanceSetTransformFB(geomInstance, &git));
        }
        // FB_passthrough sample end

        AppRenderer::FrameIn frameIn;
        uint32_t chainIndex = 0;
        XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, NULL};
        OXR(xrAcquireSwapchainImage(app.ColorSwapChain, &acquireInfo, &chainIndex));
        frameIn.SwapChainIndex = int(chainIndex);

        XrPosef xfLocalFromEye[NUM_EYES];

        for (int eye = 0; eye < NUM_EYES; eye++) {
            // LOG_POSE( "viewTransform", &projectionInfo.projections[eye].viewTransform );
            XrPosef xfHeadFromEye = projections[eye].pose;
            xfLocalFromEye[eye] = XrPosef_Multiply(xfLocalFromHead, xfHeadFromEye);

            XrPosef xfEyeFromLocal = XrPosef_Inverse(xfLocalFromEye[eye]);

            XrMatrix4x4f viewMat = XrMatrix4x4f_CreateFromRigidTransform(&xfEyeFromLocal);

            const XrFovf fov = projections[eye].fov;
            XrMatrix4x4f projMat;
            XrMatrix4x4f_CreateProjectionFov(&projMat, GRAPHICS_OPENGL_ES, fov, 0.1f, 0.0f);

            frameIn.View[eye] = OvrFromXr(viewMat);
            frameIn.Proj[eye] = OvrFromXr(projMat);
        }

        if (app.StageSpace != XR_NULL_HANDLE) {
            XrSpaceLocation loc = {XR_TYPE_SPACE_LOCATION};
            OXR(xrLocateSpace(
                app.StageSpace, app.LocalSpace, frameState.predictedDisplayTime, &loc));
            XrPosef xfLocalFromStage = loc.pose;

            frameIn.HasStage = true;
            frameIn.StagePose = OvrFromXr(xfLocalFromStage);
            frameIn.StageScale = app.StageBounds;
        } else {
            frameIn.HasStage = false;
        }

        XrSwapchainImageWaitInfo waitInfo;
        waitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
        waitInfo.next = NULL;
        waitInfo.timeout = 1000000000; /* timeout in nanoseconds */
        XrResult res = xrWaitSwapchainImage(app.ColorSwapChain, &waitInfo);
        int retry = 0;
        while (res == XR_TIMEOUT_EXPIRED) {
            res = xrWaitSwapchainImage(app.ColorSwapChain, &waitInfo);
            retry++;
            ALOGV(
                " Retry xrWaitSwapchainImage %d times due to XR_TIMEOUT_EXPIRED (duration %f seconds)",
                retry,
                waitInfo.timeout * (1E-9));
        }

        app.appRenderer.RenderFrame(frameIn);

        XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, NULL};
        OXR(xrReleaseSwapchainImage(app.ColorSwapChain, &releaseInfo));

        // Set-up the compositor layers for this frame.
        // NOTE: Multiple independent layers are allowed, but they need to be added
        // in a depth consistent order.

        XrCompositionLayerProjectionView proj_views[2] = {};

        app.LayerCount = 0;
        memset(app.Layers, 0, sizeof(CompositionLayerUnion) * MaxLayerCount);

        // FB_passthrough sample begin
        // passthrough layer is backmost layer (if available)
        if (passthroughLayer != XR_NULL_HANDLE) {
            XrCompositionLayerPassthroughFB passthrough_layer = {};
            passthrough_layer.type = XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB;
            passthrough_layer.layerHandle = passthroughLayer;
            passthrough_layer.flags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
            passthrough_layer.space = XR_NULL_HANDLE;
            app.Layers[app.LayerCount++].Passthrough = passthrough_layer;
        }
        // FB_passthrough sample end

        XrCompositionLayerProjection proj_layer = {};
        proj_layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
        proj_layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        proj_layer.layerFlags |= XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
        proj_layer.layerFlags |= XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
        proj_layer.space = app.LocalSpace;
        proj_layer.viewCount = NUM_EYES;
        proj_layer.views = proj_views;

        for (int eye = 0; eye < NUM_EYES; eye++) {
            XrCompositionLayerProjectionView& proj_view = proj_views[eye];
            proj_view = {};
            proj_view.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
            proj_view.pose = xfLocalFromEye[eye];
            proj_view.fov = projections[eye].fov;

            proj_view.subImage.swapchain = app.ColorSwapChain;
            proj_view.subImage.imageRect.offset.x = 0;
            proj_view.subImage.imageRect.offset.y = 0;
            proj_view.subImage.imageRect.extent.width = width;
            proj_view.subImage.imageRect.extent.height = height;
            proj_view.subImage.imageArrayIndex = eye;
        }

        app.Layers[app.LayerCount++].Projection = proj_layer;

        // Compose the layers for this frame.
        const XrCompositionLayerBaseHeader* layers[MaxLayerCount] = {};
        for (int i = 0; i < app.LayerCount; i++) {
            layers[i] = (const XrCompositionLayerBaseHeader*)&app.Layers[i];
        }

        XrFrameEndInfo endFrameInfo = {};
        endFrameInfo.type = XR_TYPE_FRAME_END_INFO;
        endFrameInfo.displayTime = frameState.predictedDisplayTime;
        endFrameInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endFrameInfo.layerCount = app.LayerCount;
        endFrameInfo.layers = layers;

        OXR(xrEndFrame(app.Session, &endFrameInfo));
    }

    app.appRenderer.Destroy();

    AppInput_shutdown();

    delete[] projections;

    app.egl.DestroyContext();

    OXR(xrDestroySpace(app.HeadSpace));
    OXR(xrDestroySpace(app.LocalSpace));
    // StageSpace is optional.
    if (app.StageSpace != XR_NULL_HANDLE) {
        OXR(xrDestroySpace(app.StageSpace));
    }
    OXR(xrDestroySession(app.Session));
    OXR(xrDestroyInstance(app.Instance));

    (*androidApp->activity->vm).DetachCurrentThread();
}
