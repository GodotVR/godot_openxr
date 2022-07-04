/*******************************************************************************

Filename    :   XrApp.cpp
Content     :   OpenXR application base class.
Created     :   July 2020
Authors     :   Federico Schliemann
Language    :   c++
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*******************************************************************************/

#include "XrApp.h"

#include <android/window.h>
#include <android/native_window_jni.h>
#include <openxr/openxr.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h> // for prctl( PR_SET_NAME )

using OVR::Bounds3f;
using OVR::Matrix4f;
using OVR::Posef;
using OVR::Quatf;
using OVR::Vector2f;
using OVR::Vector3f;
using OVR::Vector4f;

void OXR_CheckErrors(XrInstance instance, XrResult result, const char* function, bool failOnError) {
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

inline OVR::Matrix4f XrMatrix4x4f_To_OVRMatrix4f(const XrMatrix4x4f& src) {
    // col major to row major ==> transpose
    return OVR::Matrix4f(
        src.m[0],
        src.m[4],
        src.m[8],
        src.m[12],
        src.m[1],
        src.m[5],
        src.m[9],
        src.m[13],
        src.m[2],
        src.m[6],
        src.m[10],
        src.m[14],
        src.m[3],
        src.m[7],
        src.m[11],
        src.m[15]);
}

inline OVR::Posef XrPosef_To_OVRPosef(const XrPosef& src) {
    return OVR::Posef{
        {src.orientation.x, src.orientation.y, src.orientation.z, src.orientation.w},
        {src.position.x, src.position.y, src.position.z}};
}

inline OVR::Vector2f XrVector2f_To_OVRVector2f(const XrVector2f& src) {
    return OVR::Vector2f{src.x, src.y};
}

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

namespace OVRFW {

/**
 * Process the next main command.
 */
static void app_handle_cmd(struct android_app* app, int32_t cmd) {
    XrApp* appState = (XrApp*)app->userData;
    appState->HandleAndroidCmd(app, cmd);
}

void XrApp::HandleAndroidCmd(struct android_app* app, int32_t cmd) {
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
            Resumed = true;
            break;
        }
        case APP_CMD_PAUSE: {
            ALOGV("onPause()");
            ALOGV("    APP_CMD_PAUSE");
            Resumed = false;
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
            NativeWindow = NULL;
            break;
        }
        case APP_CMD_INIT_WINDOW: {
            ALOGV("surfaceCreated()");
            ALOGV("    APP_CMD_INIT_WINDOW");
            NativeWindow = app->window;
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            ALOGV("surfaceDestroyed()");
            ALOGV("    APP_CMD_TERM_WINDOW");
            NativeWindow = NULL;
            break;
        }
    }
}

void XrApp::HandleSessionStateChanges(XrSessionState state) {
    if (state == XR_SESSION_STATE_READY) {
        assert(Resumed);
        assert(NativeWindow != NULL);
        assert(SessionActive == false);

        XrSessionBeginInfo sessionBeginInfo;
        memset(&sessionBeginInfo, 0, sizeof(sessionBeginInfo));
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

void XrApp::HandleXrEvents() {
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

XrActionSet XrApp::CreateActionSet(int priority, const char* name, const char* localizedName) {
    XrActionSetCreateInfo asci = {};
    asci.type = XR_TYPE_ACTION_SET_CREATE_INFO;
    asci.next = NULL;
    asci.priority = priority;
    strcpy(asci.actionSetName, name);
    strcpy(asci.localizedActionSetName, localizedName);
    XrActionSet actionSet = XR_NULL_HANDLE;
    OXR(xrCreateActionSet(Instance, &asci, &actionSet));
    return actionSet;
}

XrAction XrApp::CreateAction(
    XrActionSet actionSet,
    XrActionType type,
    const char* actionName,
    const char* localizedName,
    int countSubactionPaths,
    XrPath* subactionPaths) {
    ALOGV("CreateAction %s, %" PRIi32, actionName, countSubactionPaths);

    XrActionCreateInfo aci = {};
    aci.type = XR_TYPE_ACTION_CREATE_INFO;
    aci.next = NULL;
    aci.actionType = type;
    if (countSubactionPaths > 0) {
        aci.countSubactionPaths = countSubactionPaths;
        aci.subactionPaths = subactionPaths;
    }
    strcpy(aci.actionName, actionName);
    strcpy(aci.localizedActionName, localizedName ? localizedName : actionName);
    XrAction action = XR_NULL_HANDLE;
    OXR(xrCreateAction(actionSet, &aci, &action));
    return action;
}

XrActionSuggestedBinding XrApp::ActionSuggestedBinding(XrAction action, const char* bindingString) {
    XrActionSuggestedBinding asb;
    asb.action = action;
    XrPath bindingPath;
    OXR(xrStringToPath(Instance, bindingString, &bindingPath));
    asb.binding = bindingPath;
    return asb;
}

XrSpace XrApp::CreateActionSpace(XrAction poseAction, XrPath subactionPath) {
    XrActionSpaceCreateInfo asci = {};
    asci.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
    asci.action = poseAction;
    asci.poseInActionSpace.orientation.w = 1.0f;
    asci.subactionPath = subactionPath;
    XrSpace actionSpace = XR_NULL_HANDLE;
    OXR(xrCreateActionSpace(Session, &asci, &actionSpace));
    return actionSpace;
}

XrActionStateBoolean XrApp::GetActionStateBoolean(XrAction action, XrPath subactionPath) {
    XrActionStateGetInfo getInfo = {};
    getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    getInfo.action = action;
    getInfo.subactionPath = subactionPath;
    XrActionStateBoolean state = {};
    state.type = XR_TYPE_ACTION_STATE_BOOLEAN;
    OXR(xrGetActionStateBoolean(Session, &getInfo, &state));
    return state;
}

XrActionStateFloat XrApp::GetActionStateFloat(XrAction action, XrPath subactionPath) {
    XrActionStateGetInfo getInfo = {};
    getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    getInfo.action = action;
    getInfo.subactionPath = subactionPath;
    XrActionStateFloat state = {};
    state.type = XR_TYPE_ACTION_STATE_FLOAT;
    OXR(xrGetActionStateFloat(Session, &getInfo, &state));
    return state;
}

XrActionStateVector2f XrApp::GetActionStateVector2(XrAction action, XrPath subactionPath) {
    XrActionStateGetInfo getInfo = {};
    getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    getInfo.action = action;
    getInfo.subactionPath = subactionPath;
    XrActionStateVector2f state = {};
    state.type = XR_TYPE_ACTION_STATE_VECTOR2F;
    OXR(xrGetActionStateVector2f(Session, &getInfo, &state));
    return state;
}

bool XrApp::ActionPoseIsActive(XrAction action, XrPath subactionPath) {
    XrActionStateGetInfo getInfo = {};
    getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    getInfo.action = action;
    getInfo.subactionPath = subactionPath;
    XrActionStatePose state = {};
    state.type = XR_TYPE_ACTION_STATE_POSE;
    OXR(xrGetActionStatePose(Session, &getInfo, &state));
    return state.isActive != XR_FALSE;
}

XrApp::LocVel XrApp::GetSpaceLocVel(XrSpace space, XrTime time) {
    XrApp::LocVel lv = {{}};
    lv.loc.type = XR_TYPE_SPACE_LOCATION;
    lv.loc.next = &lv.vel;
    lv.vel.type = XR_TYPE_SPACE_VELOCITY;
    OXR(xrLocateSpace(space, CurrentSpace, time, &lv.loc));
    lv.loc.next = NULL; // pointer no longer valid or necessary
    return lv;
}

// Returns a list of OpenXr extensions needed for this app
std::vector<const char*> XrApp::GetExtensions() {
    std::vector<const char*> extensions = {
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
        XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
#elif defined(XR_USE_GRAPHICS_API_OPENGL)
        XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
        XR_KHR_COMPOSITION_LAYER_COLOR_SCALE_BIAS_EXTENSION_NAME,
#if defined(XR_USE_PLATFORM_ANDROID)
        XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME,
        XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME,
#endif // defined(XR_USE_PLATFORM_ANDROID)
        XR_KHR_COMPOSITION_LAYER_CUBE_EXTENSION_NAME,
        XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME
    };
    return extensions;
}

// Returns a map from interaction profile paths to vectors of suggested bindings.
// xrSuggestInteractionProfileBindings() is called once for each interaction profile path in the
// returned map.
// Apps are encouraged to suggest bindings for every device/interaction profile they support.
// Override this for custom action bindings, or modify the default bindings.
std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> XrApp::GetSuggestedBindings(
    XrInstance instance) {
    std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> suggestedBindings{};

    // By default we support "oculus/touch_controller" and "khr/simple_controller" as a fallback
    // All supported controllers should be explicitly listed here
    XrPath simpleInteractionProfile = XR_NULL_PATH;
    XrPath touchInteractionProfile = XR_NULL_PATH;

    OXR(xrStringToPath(
        instance, "/interaction_profiles/khr/simple_controller", &simpleInteractionProfile));
    OXR(xrStringToPath(
        instance, "/interaction_profiles/oculus/touch_controller", &touchInteractionProfile));

    // -----------------------------------------
    // Bindings for oculus/touch_controller
    // -----------------------------------------
    // Note: using the fact that operator[] creates an object if it doesn't exist in the map
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(AimPoseAction, "/user/hand/left/input/aim/pose"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(AimPoseAction, "/user/hand/right/input/aim/pose"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(GripPoseAction, "/user/hand/left/input/grip/pose"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(GripPoseAction, "/user/hand/right/input/grip/pose"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(JoystickAction, "/user/hand/left/input/thumbstick"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(JoystickAction, "/user/hand/right/input/thumbstick"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(IndexTriggerAction, "/user/hand/left/input/trigger/value"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(IndexTriggerAction, "/user/hand/right/input/trigger/value"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(GripTriggerAction, "/user/hand/left/input/squeeze/value"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(GripTriggerAction, "/user/hand/right/input/squeeze/value"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(ButtonAAction, "/user/hand/right/input/a/click"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(ButtonBAction, "/user/hand/right/input/b/click"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(ButtonXAction, "/user/hand/left/input/x/click"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(ButtonYAction, "/user/hand/left/input/y/click"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(ButtonMenuAction, "/user/hand/left/input/menu/click"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(ThumbStickTouchAction, "/user/hand/left/input/thumbstick/touch"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(ThumbStickTouchAction, "/user/hand/right/input/thumbstick/touch"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(ThumbRestTouchAction, "/user/hand/left/input/thumbrest/touch"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(ThumbRestTouchAction, "/user/hand/right/input/thumbrest/touch"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(TriggerTouchAction, "/user/hand/left/input/trigger/touch"));
    suggestedBindings[touchInteractionProfile].emplace_back(
        ActionSuggestedBinding(TriggerTouchAction, "/user/hand/right/input/trigger/touch"));

    // -----------------------------------------
    // Default bindings for khr/simple_controller
    // -----------------------------------------
    suggestedBindings[simpleInteractionProfile].emplace_back(
        ActionSuggestedBinding(AimPoseAction, "/user/hand/left/input/aim/pose"));
    suggestedBindings[simpleInteractionProfile].emplace_back(
        ActionSuggestedBinding(AimPoseAction, "/user/hand/right/input/aim/pose"));
    suggestedBindings[simpleInteractionProfile].emplace_back(
        ActionSuggestedBinding(GripPoseAction, "/user/hand/right/input/grip/pose"));
    suggestedBindings[simpleInteractionProfile].emplace_back(
        ActionSuggestedBinding(GripPoseAction, "/user/hand/left/input/grip/pose"));

    suggestedBindings[simpleInteractionProfile].emplace_back(
        ActionSuggestedBinding(IndexTriggerAction, "/user/hand/right/input/select/click"));
    suggestedBindings[simpleInteractionProfile].emplace_back(
        ActionSuggestedBinding(IndexTriggerAction, "/user/hand/left/input/select/click"));

    suggestedBindings[simpleInteractionProfile].emplace_back(
        ActionSuggestedBinding(ButtonBAction, "/user/hand/right/input/menu/click"));
    suggestedBindings[simpleInteractionProfile].emplace_back(
        ActionSuggestedBinding(ButtonMenuAction, "/user/hand/left/input/menu/click"));

    return suggestedBindings;
}

// Called one time when the application process starts.
// Returns true if the application initialized successfully.
bool XrApp::Init(const xrJava* context) {
    // Loader
    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
    xrGetInstanceProcAddr(
        XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)&xrInitializeLoaderKHR);
    if (xrInitializeLoaderKHR != NULL) {
        XrLoaderInitInfoAndroidKHR loaderInitializeInfoAndroid;
        memset(&loaderInitializeInfoAndroid, 0, sizeof(loaderInitializeInfoAndroid));
        loaderInitializeInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
        loaderInitializeInfoAndroid.next = NULL;
        loaderInitializeInfoAndroid.applicationVM = context->Vm;
        loaderInitializeInfoAndroid.applicationContext = context->ActivityObject;
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

        std::vector<XrApiLayerProperties> layerProperties(numOutputLayers);

        for (uint32_t i = 0; i < numOutputLayers; i++) {
            layerProperties[i].type = XR_TYPE_API_LAYER_PROPERTIES;
            layerProperties[i].next = NULL;
        }

        OXR(xrEnumerateApiLayerProperties(
            numInputLayers, &numOutputLayers, layerProperties.data()));

        for (uint32_t i = 0; i < numOutputLayers; i++) {
            ALOGV("Found layer %s", layerProperties[i].layerName);
        }
    }

    // Check that the extensions required are present.
    std::vector<const char*> extensions = GetExtensions();

    const char* const* requiredExtensionNames = extensions.data();
    const uint32_t numRequiredExtensions = extensions.size();

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

        std::vector<XrExtensionProperties> extensionProperties(
            numOutputExtensions, {XR_TYPE_EXTENSION_PROPERTIES});

        OXR(xrEnumerateInstanceExtensionProperties(
            NULL, numInputExtensions, &numOutputExtensions, extensionProperties.data()));
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
                ALOGW("WARNING - Failed to find required extension %s", requiredExtensionNames[i]);
            }
        }
    }

    // Create the OpenXR instance.
    XrApplicationInfo appInfo;
    memset(&appInfo, 0, sizeof(appInfo));
    strcpy(appInfo.applicationName, "OpenXR_NativeActivity");
    appInfo.applicationVersion = 0;
    strcpy(appInfo.engineName, "Oculus Mobile Sample");
    appInfo.engineVersion = 0;
    appInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrInstanceCreateInfo instanceCreateInfo;
    memset(&instanceCreateInfo, 0, sizeof(instanceCreateInfo));
    instanceCreateInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.next = nullptr;
    instanceCreateInfo.createFlags = 0;
    instanceCreateInfo.applicationInfo = appInfo;
    instanceCreateInfo.enabledApiLayerCount = 0;
    instanceCreateInfo.enabledApiLayerNames = NULL;
    instanceCreateInfo.enabledExtensionCount = numRequiredExtensions;
    instanceCreateInfo.enabledExtensionNames = requiredExtensionNames;

    XrResult initResult;
    OXR(initResult = xrCreateInstance(&instanceCreateInfo, &Instance));
    if (initResult != XR_SUCCESS) {
        ALOGE("Failed to create XR instance: %d.", initResult);
        exit(1);
    }

    ///

    XrInstanceProperties instanceInfo;
    instanceInfo.type = XR_TYPE_INSTANCE_PROPERTIES;
    instanceInfo.next = NULL;
    OXR(xrGetInstanceProperties(Instance, &instanceInfo));
    ALOGV(
        "Runtime %s: Version : %u.%u.%u",
        instanceInfo.runtimeName,
        XR_VERSION_MAJOR(instanceInfo.runtimeVersion),
        XR_VERSION_MINOR(instanceInfo.runtimeVersion),
        XR_VERSION_PATCH(instanceInfo.runtimeVersion));

    XrSystemGetInfo systemGetInfo;
    memset(&systemGetInfo, 0, sizeof(systemGetInfo));
    systemGetInfo.type = XR_TYPE_SYSTEM_GET_INFO;
    systemGetInfo.next = NULL;
    systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId systemId;
    OXR(initResult = xrGetSystem(Instance, &systemGetInfo, &systemId));
    if (initResult != XR_SUCCESS) {
        ALOGE("Failed to get system.");
        exit(1);
    }

    XrSystemProperties systemProperties = {};
    systemProperties.type = XR_TYPE_SYSTEM_PROPERTIES;
    OXR(xrGetSystemProperties(Instance, systemId, &systemProperties));
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
    assert(MAX_NUM_LAYERS <= systemProperties.graphicsProperties.maxLayerCount);

    // Get the graphics requirements.
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = NULL;
    OXR(xrGetInstanceProcAddr(
        Instance,
        "xrGetOpenGLESGraphicsRequirementsKHR",
        (PFN_xrVoidFunction*)(&pfnGetOpenGLESGraphicsRequirementsKHR)));

    XrGraphicsRequirementsOpenGLESKHR graphicsRequirements = {};
    graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR;
    OXR(pfnGetOpenGLESGraphicsRequirementsKHR(Instance, systemId, &graphicsRequirements));
#elif defined(XR_USE_GRAPHICS_API_OPENGL)
    // Get the graphics requirements.
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = NULL;
    OXR(xrGetInstanceProcAddr(
        Instance,
        "xrGetOpenGLGraphicsRequirementsKHR",
        (PFN_xrVoidFunction*)(&pfnGetOpenGLGraphicsRequirementsKHR)));

    XrGraphicsRequirementsOpenGLKHR graphicsRequirements = {};
    graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;
    OXR(pfnGetOpenGLGraphicsRequirementsKHR(Instance, systemId, &graphicsRequirements));
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)

    // Create the EGL Context
    ovrEgl_CreateContext(&Egl, NULL);

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
    EglInitExtensions();

    CpuLevel = CPU_LEVEL;
    GpuLevel = GPU_LEVEL;
    MainThreadTid = gettid();
    SystemId = systemId;

    // Actions
    BaseActionSet = CreateActionSet(1, "base_action_set", "Action Set used on main loop");

    OXR(xrStringToPath(Instance, "/user/hand/left", &LeftHandPath));
    OXR(xrStringToPath(Instance, "/user/hand/right", &RightHandPath));
    XrPath handSubactionPaths[2] = {LeftHandPath, RightHandPath};

    AimPoseAction = CreateAction(
        BaseActionSet, XR_ACTION_TYPE_POSE_INPUT, "aim_pose", NULL, 2, &handSubactionPaths[0]);
    GripPoseAction = CreateAction(
        BaseActionSet, XR_ACTION_TYPE_POSE_INPUT, "grip_pose", NULL, 2, &handSubactionPaths[0]);
    JoystickAction = CreateAction(
        BaseActionSet,
        XR_ACTION_TYPE_VECTOR2F_INPUT,
        "move_on_joy",
        NULL,
        2,
        &handSubactionPaths[0]);
    IndexTriggerAction = CreateAction(
        BaseActionSet,
        XR_ACTION_TYPE_FLOAT_INPUT,
        "index_trigger",
        NULL,
        2,
        &handSubactionPaths[0]);
    GripTriggerAction = CreateAction(
        BaseActionSet, XR_ACTION_TYPE_FLOAT_INPUT, "grip_trigger", NULL, 2, &handSubactionPaths[0]);
    ButtonAAction = CreateAction(BaseActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "button_a", NULL);
    ButtonBAction = CreateAction(BaseActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "button_b", NULL);
    ButtonXAction = CreateAction(BaseActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "button_x", NULL);
    ButtonYAction = CreateAction(BaseActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "button_y", NULL);
    ButtonMenuAction =
        CreateAction(BaseActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "button_menu", NULL);
    ThumbStickTouchAction =
        CreateAction(BaseActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "thumb_stick_touch", NULL);
    ThumbRestTouchAction =
        CreateAction(BaseActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "thumb_rest_touch", NULL);
    TriggerTouchAction =
        CreateAction(BaseActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "index_trigger_touch", NULL);

    /// Interaction profile can be overridden
    std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> allSuggestedBindings =
        GetSuggestedBindings(GetInstance());

    // Best practice is for apps to suggest bindings for *ALL* interaction profiles
    // that the app supports. Loop over all interaction profiles we support and suggest bindings:
    for (auto& [interactionProfilePath, bindings] : allSuggestedBindings) {
        XrInteractionProfileSuggestedBinding suggestedBindings = {};
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.next = NULL;
        suggestedBindings.interactionProfile = interactionProfilePath;
        suggestedBindings.suggestedBindings = (const XrActionSuggestedBinding*)bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();

        OXR(xrSuggestInteractionProfileBindings(Instance, &suggestedBindings));
    }

    FileSys = std::unique_ptr<OVRFW::ovrFileSys>(ovrFileSys::Create(*context));
    if (FileSys) {
        OVRFW::ovrFileSys& fs = *FileSys;
        MaterialParms materialParms;
        materialParms.UseSrgbTextureFormats = false;
        SceneModel = std::unique_ptr<OVRFW::ModelFile>(LoadModelFile(
            fs, "apk:///assets/box.ovrscene", Scene.GetDefaultGLPrograms(), materialParms));
        if (SceneModel != nullptr) {
            Scene.SetWorldModel(*SceneModel);
            Vector3f modelOffset;
            modelOffset.x = 0.5f;
            modelOffset.y = 0.0f;
            modelOffset.z = -2.25f;
            Scene.GetWorldModel()->State.SetMatrix(
                Matrix4f::Scaling(2.5f, 2.5f, 2.5f) * Matrix4f::Translation(modelOffset));
        }
    }
    SurfaceRender.Init();

    return AppInit(context);
}

bool XrApp::InitSession() {
    // Create the OpenXR Session.
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    XrGraphicsBindingOpenGLESAndroidKHR graphicsBindingAndroidGLES = {};
    graphicsBindingAndroidGLES.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR;
    graphicsBindingAndroidGLES.next = NULL;
    graphicsBindingAndroidGLES.display = Egl.Display;
    graphicsBindingAndroidGLES.config = Egl.Config;
    graphicsBindingAndroidGLES.context = Egl.Context;
#elif defined(XR_USE_GRAPHICS_API_OPENGL)
    XrGraphicsBindingOpenGLWin32KHR graphicsBindingGL = {};
    graphicsBindingGL.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
    graphicsBindingGL.next = NULL;
    graphicsBindingGL.hDC = Egl.hDC;
    graphicsBindingGL.hGLRC = Egl.hGLRC;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)

    XrSessionCreateInfo sessionCreateInfo = {};
    memset(&sessionCreateInfo, 0, sizeof(sessionCreateInfo));
    sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    sessionCreateInfo.next = &graphicsBindingAndroidGLES;
#elif defined(XR_USE_GRAPHICS_API_OPENGL)
    sessionCreateInfo.next = &graphicsBindingGL;
#endif
    sessionCreateInfo.createFlags = 0;
    sessionCreateInfo.systemId = SystemId;

    XrResult initResult;
    OXR(initResult = xrCreateSession(Instance, &sessionCreateInfo, &Session));
    if (initResult != XR_SUCCESS) {
        ALOGE("Failed to create XR session: %d.", initResult);
        exit(1);
    }

    // App only supports the primary stereo view config.
    const XrViewConfigurationType supportedViewConfigType =
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    // Enumerate the viewport configurations.
    {
        uint32_t viewportConfigTypeCount = 0;
        OXR(xrEnumerateViewConfigurations(Instance, SystemId, 0, &viewportConfigTypeCount, NULL));

        std::vector<XrViewConfigurationType> viewportConfigurationTypes(viewportConfigTypeCount);

        OXR(xrEnumerateViewConfigurations(
            Instance,
            SystemId,
            viewportConfigTypeCount,
            &viewportConfigTypeCount,
            viewportConfigurationTypes.data()));

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
                Instance, SystemId, viewportConfigType, &viewportConfig));
            ALOGV(
                "FovMutable=%s ConfigurationType %d",
                viewportConfig.fovMutable ? "true" : "false",
                viewportConfig.viewConfigurationType);

            uint32_t viewCount;
            OXR(xrEnumerateViewConfigurationViews(
                Instance, SystemId, viewportConfigType, 0, &viewCount, NULL));

            if (viewCount > 0) {
                std::vector<XrViewConfigurationView> elements(
                    viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});

                OXR(xrEnumerateViewConfigurationViews(
                    Instance,
                    SystemId,
                    viewportConfigType,
                    viewCount,
                    &viewCount,
                    elements.data()));

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
                    assert(viewCount == MAX_NUM_EYES);
                    for (uint32_t e = 0; e < viewCount; e++) {
                        ViewConfigurationView[e] = elements[e];
                    }
                }
            } else {
                ALOGE("Empty viewport configuration type: %d", viewCount);
            }
        }
    }

    // Get the viewport configuration info for the chosen viewport configuration type.
    ViewportConfig.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
    OXR(xrGetViewConfigurationProperties(
        Instance, SystemId, supportedViewConfigType, &ViewportConfig));

    bool stageSupported = false;
    uint32_t numOutputSpaces = 0;
    OXR(xrEnumerateReferenceSpaces(Session, 0, &numOutputSpaces, NULL));

    std::vector<XrReferenceSpaceType> referenceSpaces(numOutputSpaces);
    OXR(xrEnumerateReferenceSpaces(
        Session, numOutputSpaces, &numOutputSpaces, referenceSpaces.data()));
    for (uint32_t i = 0; i < numOutputSpaces; i++) {
        if (referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE) {
            stageSupported = true;
            break;
        }
    }

    // Create a space to the first path
    XrReferenceSpaceCreateInfo spaceCreateInfo = {};
    spaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
    OXR(xrCreateReferenceSpace(Session, &spaceCreateInfo, &HeadSpace));

    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    OXR(xrCreateReferenceSpace(Session, &spaceCreateInfo, &LocalSpace));

    // to use as fake stage
    if (stageSupported) {
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        spaceCreateInfo.poseInReferenceSpace.position.y = 0.0f;
        OXR(xrCreateReferenceSpace(Session, &spaceCreateInfo, &StageSpace));
        ALOGV("Created stage space");
        CurrentSpace = StageSpace;
    }

    EglInitExtensions();

    // Create the frame buffers.
    for (int eye = 0; eye < MAX_NUM_EYES; eye++) {
        ovrFramebuffer_Create(
            Session,
            &FrameBuffer[eye],
            GL_SRGB8_ALPHA8,
            ViewConfigurationView[0].recommendedImageRectWidth,
            ViewConfigurationView[0].recommendedImageRectHeight,
            NUM_MULTI_SAMPLES);
    }

    // Attach to session
    AttachActionSets();

    LeftControllerAimSpace = XR_NULL_HANDLE;
    LeftControllerGripSpace = XR_NULL_HANDLE;
    RightControllerAimSpace = XR_NULL_HANDLE;
    RightControllerGripSpace = XR_NULL_HANDLE;

    return SessionInit();
}

void XrApp::EndSession() {
    for (int eye = 0; eye < MAX_NUM_EYES; eye++) {
        ovrFramebuffer_Destroy(&FrameBuffer[eye]);
    }
    ovrEgl_DestroyContext(&Egl);

    OXR(xrDestroySpace(HeadSpace));
    OXR(xrDestroySpace(LocalSpace));
    // StageSpace is optional.
    if (StageSpace != XR_NULL_HANDLE) {
        OXR(xrDestroySpace(StageSpace));
    }
    CurrentSpace = XR_NULL_HANDLE;
    OXR(xrDestroySession(Session));
    SessionEnd();
}

// Called one time when the applicatoin process exits
void XrApp::Shutdown(const xrJava* context) {
    OXR(xrDestroyInstance(Instance));
}

// Internal Input
void XrApp::AttachActionSets() {
    XrSessionActionSetsAttachInfo attachInfo = {};
    attachInfo.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
    attachInfo.next = NULL;
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &BaseActionSet;
    OXR(xrAttachSessionActionSets(Session, &attachInfo));
}

void XrApp::SyncActionSets(ovrApplFrameIn& in, XrFrameState& frameState) {
    // sync action data
    XrActiveActionSet activeActionSet = {};
    activeActionSet.actionSet = BaseActionSet;
    activeActionSet.subactionPath = XR_NULL_PATH;

    XrActionsSyncInfo syncInfo = {};
    syncInfo.type = XR_TYPE_ACTIONS_SYNC_INFO;
    syncInfo.next = NULL;
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &activeActionSet;
    OXR(xrSyncActions(Session, &syncInfo));

    // query input action states
    XrActionStateGetInfo getInfo = {};
    getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    getInfo.next = NULL;
    getInfo.subactionPath = XR_NULL_PATH;

    XrAction controller[] = {AimPoseAction, GripPoseAction, AimPoseAction, GripPoseAction};
    XrPath subactionPath[] = {LeftHandPath, LeftHandPath, RightHandPath, RightHandPath};
    XrSpace controllerSpace[] = {
        LeftControllerAimSpace,
        LeftControllerGripSpace,
        RightControllerAimSpace,
        RightControllerGripSpace,
    };
    bool ControllerPoseActive[] = {false, false, false, false};
    XrPosef ControllerPose[] = {
        XrPosef_Identity(), XrPosef_Identity(), XrPosef_Identity(), XrPosef_Identity()};
    for (int i = 0; i < 4; i++) {
        if (ActionPoseIsActive(controller[i], subactionPath[i])) {
            LocVel lv = GetSpaceLocVel(controllerSpace[i], frameState.predictedDisplayTime);
            ControllerPoseActive[i] =
                (lv.loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;
            ControllerPose[i] = lv.loc.pose;
        } else {
            ControllerPoseActive[i] = false;
            ControllerPose[i] = XrPosef_Identity();
        }
    }

#if 0 // Enable for debugging current interaction profile issues
    XrPath topLevelUserPath = XR_NULL_PATH;
    OXR(xrStringToPath(Instance, "/user/hand/left", &topLevelUserPath));
    XrInteractionProfileState ipState{XR_TYPE_INTERACTION_PROFILE_STATE};
    xrGetCurrentInteractionProfile(Session, topLevelUserPath, &ipState);
    uint32_t ipPathOutSize = 0;
    char ipPath[256];
    xrPathToString(Instance, ipState.interactionProfile, sizeof(ipPath), &ipPathOutSize, ipPath);
    ALOG( "Current interaction profile is: '%s'", ipPath);
#endif

    /// accounting
    in.PredictedDisplayTime = FromXrTime(frameState.predictedDisplayTime);
    /// Update pose
    XrSpaceLocation loc = {};
    loc.type = XR_TYPE_SPACE_LOCATION;
    OXR(xrLocateSpace(HeadSpace, CurrentSpace, frameState.predictedDisplayTime, &loc));
    in.HeadPose = XrPosef_To_OVRPosef(loc.pose);
    /// grip & point space
    in.LeftRemotePointPose = XrPosef_To_OVRPosef(ControllerPose[0]);
    in.LeftRemotePose = XrPosef_To_OVRPosef(ControllerPose[1]);
    in.RightRemotePointPose = XrPosef_To_OVRPosef(ControllerPose[2]);
    in.RightRemotePose = XrPosef_To_OVRPosef(ControllerPose[3]);
    in.LeftRemoteTracked = ControllerPoseActive[1];
    in.RightRemoteTracked = ControllerPoseActive[3];

    in.LeftRemoteIndexTrigger = GetActionStateFloat(IndexTriggerAction, LeftHandPath).currentState;
    in.RightRemoteIndexTrigger =
        GetActionStateFloat(IndexTriggerAction, RightHandPath).currentState;
    in.LeftRemoteGripTrigger = GetActionStateFloat(GripTriggerAction, LeftHandPath).currentState;
    in.RightRemoteGripTrigger = GetActionStateFloat(GripTriggerAction, RightHandPath).currentState;
    in.LeftRemoteJoystick =
        XrVector2f_To_OVRVector2f(GetActionStateVector2(JoystickAction, LeftHandPath).currentState);
    in.RightRemoteJoystick = XrVector2f_To_OVRVector2f(
        GetActionStateVector2(JoystickAction, RightHandPath).currentState);

    bool aPressed = GetActionStateBoolean(ButtonAAction).currentState;
    bool bPressed = GetActionStateBoolean(ButtonBAction).currentState;
    bool xPressed = GetActionStateBoolean(ButtonXAction).currentState;
    bool yPressed = GetActionStateBoolean(ButtonYAction).currentState;
    bool menuPressed = GetActionStateBoolean(ButtonMenuAction).currentState;

    in.LastFrameAllButtons = LastFrameAllButtons;
    in.AllButtons = 0u;

    if (aPressed) {
        in.AllButtons |= ovrApplFrameIn::kButtonA;
    }
    if (bPressed) {
        in.AllButtons |= ovrApplFrameIn::kButtonB;
    }
    if (xPressed) {
        in.AllButtons |= ovrApplFrameIn::kButtonX;
    }
    if (yPressed) {
        in.AllButtons |= ovrApplFrameIn::kButtonY;
    }
    if (menuPressed) {
        in.AllButtons |= ovrApplFrameIn::kButtonMenu;
    }
    if (in.LeftRemoteIndexTrigger > 0.1f) {
        in.AllButtons |= ovrApplFrameIn::kTrigger;
    }
    if (in.RightRemoteIndexTrigger > 0.1f) {
        in.AllButtons |= ovrApplFrameIn::kTrigger;
    }
    if (in.LeftRemoteGripTrigger > 0.1f) {
        in.AllButtons |= ovrApplFrameIn::kGripTrigger;
    }
    if (in.RightRemoteGripTrigger > 0.1f) {
        in.AllButtons |= ovrApplFrameIn::kGripTrigger;
    }
    LastFrameAllButtons = in.AllButtons;

    /// touch
    in.LastFrameAllTouches = LastFrameAllTouches;
    in.AllTouches = 0u;

    const bool thumbstickTouched = GetActionStateBoolean(ThumbStickTouchAction).currentState;
    const bool thumbrestTouched = GetActionStateBoolean(ThumbRestTouchAction).currentState;
    const bool triggerTouched = GetActionStateBoolean(TriggerTouchAction).currentState;

    if (thumbstickTouched) {
        in.AllTouches |= ovrApplFrameIn::kTouchJoystick;
    }
    if (thumbrestTouched) {
        in.AllTouches |= ovrApplFrameIn::kTouchThumbrest;
    }
    if (triggerTouched) {
        in.AllTouches |= ovrApplFrameIn::kTouchTrigger;
    }

    LastFrameAllTouches = in.AllTouches;

    /*
        /// timing
        double RealTimeInSeconds = 0.0;
        float DeltaSeconds = 0.0f;
        /// device config
        float IPD = 0.065f;
        float EyeHeight = 1.6750f;
        int32_t RecenterCount = 0;
    */
}

void XrApp::HandleInput(ovrApplFrameIn& in, XrFrameState& frameState) {
    // Sync default actions
    SyncActionSets(in, frameState);
    // Call extension
    Update(in);
}

// Called once per frame to allow the application to render eye buffers.
void XrApp::AppRenderFrame(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) {
    Scene.SetFreeMove(FreeMove);
    /// create a local copy
    OVRFW::ovrApplFrameIn localIn = in;
    if (false == FreeMove) {
        localIn.LeftRemoteJoystick.x = 0.0f;
        localIn.LeftRemoteJoystick.y = 0.0f;
        localIn.RightRemoteJoystick.x = 0.0f;
        localIn.RightRemoteJoystick.y = 0.0f;
    }
    Scene.Frame(localIn);
    Scene.GenerateFrameSurfaceList(out.FrameMatrices, out.Surfaces);
    Render(in, out);

    for (int eye = 0; eye < MAX_NUM_EYES; eye++) {
        ovrFramebuffer* frameBuffer = &FrameBuffer[eye];
        ovrFramebuffer_Acquire(frameBuffer);
        ovrFramebuffer_SetCurrent(frameBuffer);

        AppEyeGLStateSetup(in, frameBuffer, eye);
        AppRenderEye(in, out, eye);

        ovrFramebuffer_Resolve(frameBuffer);
        ovrFramebuffer_Release(frameBuffer);
    }
    ovrFramebuffer_SetNone();
}

void XrApp::AppRenderEye(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out, int eye) {
    // Render the surfaces returned by Frame.
    SurfaceRender.RenderSurfaceList(
        out.Surfaces,
        out.FrameMatrices.EyeView[0], // always use 0 as it assumes an array
        out.FrameMatrices.EyeProjection[0], // always use 0 as it assumes an array
        eye);
}

// Called once per eye each frame for default renderer
void XrApp::AppEyeGLStateSetup(const ovrApplFrameIn& in, const ovrFramebuffer* fb, int eye) {
    GL(glEnable(GL_SCISSOR_TEST));
    GL(glDepthMask(GL_TRUE));
    GL(glEnable(GL_DEPTH_TEST));
    GL(glDepthFunc(GL_LEQUAL));
    GL(glEnable(GL_CULL_FACE));
    GL(glViewport(0, 0, fb->Width, fb->Height));
    GL(glScissor(0, 0, fb->Width, fb->Height));
    GL(glClearColor(BackgroundColor.x, BackgroundColor.y, BackgroundColor.z, BackgroundColor.w));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

// Called when the application initializes.
// Overridden by the actual app
// Must return true if the application initializes successfully.
bool XrApp::AppInit(const xrJava* context) {
    return true;
}

// Called when the application shuts down
void XrApp::AppShutdown(const xrJava* context) {
    SurfaceRender.Shutdown();
}

// Called when the application is resumed by the system.
void XrApp::AppResumed(const xrJava* contet) {}

// Called when the application is paused by the system.
void XrApp::AppPaused(const xrJava* context) {}

// Called when app loses focus
void XrApp::AppLostFocus() {}

// Called when app re-gains focus
void XrApp::AppGainedFocus() {}

bool XrApp::SessionInit() {
    return true;
}

void XrApp::SessionEnd() {}

void XrApp::Update(const ovrApplFrameIn& in) {}

void XrApp::Render(const ovrApplFrameIn& in, ovrRendererOutput& out) {}

// App entry point
void XrApp::Run(struct android_app* app) {
    ALOGV("----------------------------------------------------------------");
    ALOGV("android_app_entry()");
    ALOGV("    android_main()");

    // TODO: We should make this not required for OOPC apps.
    ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

    JNIEnv* Env;
    (*app->activity->vm).AttachCurrentThread(&Env, nullptr);

    // Note that AttachCurrentThread will reset the thread name.
    prctl(PR_SET_NAME, (long)"XrApp::Main", 0, 0, 0);

    Context.Vm = app->activity->vm;
    Context.Env = Env;
    Context.ActivityObject = app->activity->clazz;
    Init(&Context);

    InitSession();

    app->userData = this;
    app->onAppCmd = app_handle_cmd;

    bool stageBoundsDirty = true;
    int frameCount = -1;

    while (app->destroyRequested == 0) {
        frameCount++;

        // Read all pending events.
        for (;;) {
            int events;
            struct android_poll_source* source;
            // If the timeout is zero, returns immediately without blocking.
            // If the timeout is negative, waits indefinitely until an event appears.
            const int timeoutMilliseconds =
                (Resumed == false && SessionActive == false && app->destroyRequested == 0) ? -1 : 0;
            if (ALooper_pollAll(timeoutMilliseconds, NULL, &events, (void**)&source) < 0) {
                break;
            }

            // Process this event.
            if (source != NULL) {
                source->process(app, source);
            }
        }

        HandleXrEvents();

        if (SessionActive == false) {
            continue;
        }

        if (LeftControllerAimSpace == XR_NULL_HANDLE) {
            LeftControllerAimSpace = CreateActionSpace(AimPoseAction, LeftHandPath);
        }
        if (RightControllerAimSpace == XR_NULL_HANDLE) {
            RightControllerAimSpace = CreateActionSpace(AimPoseAction, RightHandPath);
        }
        if (LeftControllerGripSpace == XR_NULL_HANDLE) {
            LeftControllerGripSpace = CreateActionSpace(GripPoseAction, LeftHandPath);
        }
        if (RightControllerGripSpace == XR_NULL_HANDLE) {
            RightControllerGripSpace = CreateActionSpace(GripPoseAction, RightHandPath);
        }

        if (stageBoundsDirty) {
            XrExtent2Df stageBounds = {};
            XrResult result;
            OXR(result = xrGetReferenceSpaceBoundsRect(
                    Session, XR_REFERENCE_SPACE_TYPE_STAGE, &stageBounds));
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

        OXR(xrWaitFrame(Session, &waitFrameInfo, &frameState));

        // Get the HMD pose, predicted for the middle of the time period during which
        // the new eye images will be displayed. The number of frames predicted ahead
        // depends on the pipeline depth of the engine and the synthesis rate.
        // The better the prediction, the less black will be pulled in at the edges.
        XrFrameBeginInfo beginFrameDesc = {};
        beginFrameDesc.type = XR_TYPE_FRAME_BEGIN_INFO;
        beginFrameDesc.next = NULL;
        OXR(xrBeginFrame(Session, &beginFrameDesc));

        XrSpaceLocation loc = {};
        loc.type = XR_TYPE_SPACE_LOCATION;
        OXR(xrLocateSpace(HeadSpace, CurrentSpace, frameState.predictedDisplayTime, &loc));
        XrPosef xfStageFromHead = loc.pose;
        OXR(xrLocateSpace(HeadSpace, LocalSpace, frameState.predictedDisplayTime, &loc));

        XrViewState viewState = {XR_TYPE_VIEW_STATE, NULL};

        XrViewLocateInfo projectionInfo = {};
        projectionInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
        projectionInfo.viewConfigurationType = ViewportConfig.viewConfigurationType;
        projectionInfo.displayTime = frameState.predictedDisplayTime;
        projectionInfo.space = HeadSpace;

        uint32_t projectionCapacityInput = MAX_NUM_EYES;
        uint32_t projectionCountOutput = projectionCapacityInput;

        OXR(xrLocateViews(
            Session,
            &projectionInfo,
            &viewState,
            projectionCapacityInput,
            &projectionCountOutput,
            Projections));

        OVRFW::ovrApplFrameIn in = {};
        OVRFW::ovrRendererOutput out = {};
        in.FrameIndex = frameCount;

        XrPosef viewTransform[2];
        for (int eye = 0; eye < MAX_NUM_EYES; eye++) {
            XrPosef xfHeadFromEye = Projections[eye].pose;
            XrPosef xfStageFromEye = XrPosef_Multiply(xfStageFromHead, xfHeadFromEye);
            viewTransform[eye] = XrPosef_Inverse(xfStageFromEye);
            XrMatrix4x4f viewMat = XrMatrix4x4f_CreateFromRigidTransform(&viewTransform[eye]);
            const XrFovf fov = Projections[eye].fov;
            XrMatrix4x4f projMat;
            XrMatrix4x4f_CreateProjectionFov(&projMat, GRAPHICS_OPENGL_ES, fov, 0.1f, 0.0f);
            out.FrameMatrices.EyeView[eye] = XrMatrix4x4f_To_OVRMatrix4f(viewMat);
            out.FrameMatrices.EyeProjection[eye] = XrMatrix4x4f_To_OVRMatrix4f(projMat);
            in.Eye[eye].ViewMatrix = out.FrameMatrices.EyeView[eye];
            in.Eye[eye].ProjectionMatrix = out.FrameMatrices.EyeProjection[eye];
        }

        XrPosef centerView = XrPosef_Inverse(xfStageFromHead);
        XrMatrix4x4f viewMat = XrMatrix4x4f_CreateFromRigidTransform(&centerView);
        out.FrameMatrices.CenterView = XrMatrix4x4f_To_OVRMatrix4f(viewMat);

        // Input
        HandleInput(in, frameState);

        // Set-up the compositor layers for this frame.
        // NOTE: Multiple independent layers are allowed, but they need to be added
        // in a depth consistent order.
        XrCompositionLayerProjectionView projection_layer_elements[2] = {};
        LayerCount = 0;
        memset(Layers, 0, sizeof(xrCompositorLayerUnion) * MAX_NUM_LAYERS);

        // allow apps to submit a layer before the world view projection layer (uncommon)
        PreProjectionAddLayer(Layers, LayerCount);

        // Render the world-view layer (projection)
        {
            AppRenderFrame(in, out);

            XrCompositionLayerProjection projection_layer = {};
            projection_layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
            projection_layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
            projection_layer.layerFlags |= XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
            projection_layer.space = CurrentSpace;
            projection_layer.viewCount = MAX_NUM_EYES;
            projection_layer.views = projection_layer_elements;

            for (int eye = 0; eye < MAX_NUM_EYES; eye++) {
                ovrFramebuffer* frameBuffer = &FrameBuffer[eye];
                memset(
                    &projection_layer_elements[eye], 0, sizeof(XrCompositionLayerProjectionView));
                projection_layer_elements[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
                projection_layer_elements[eye].pose = XrPosef_Inverse(viewTransform[eye]);
                projection_layer_elements[eye].fov = Projections[eye].fov;
                memset(&projection_layer_elements[eye].subImage, 0, sizeof(XrSwapchainSubImage));
                projection_layer_elements[eye].subImage.swapchain =
                    frameBuffer->ColorSwapChain.Handle;
                projection_layer_elements[eye].subImage.imageRect.offset.x = 0;
                projection_layer_elements[eye].subImage.imageRect.offset.y = 0;
                projection_layer_elements[eye].subImage.imageRect.extent.width =
                    frameBuffer->ColorSwapChain.Width;
                projection_layer_elements[eye].subImage.imageRect.extent.height =
                    frameBuffer->ColorSwapChain.Height;
                projection_layer_elements[eye].subImage.imageArrayIndex = 0;
            }

            Layers[LayerCount++].Projection = projection_layer;
        }

        // allow apps to submit a layer after the world view projection layer (uncommon)
        PostProjectionAddLayer(Layers, LayerCount);

        // Compose the layers for this frame.
        const XrCompositionLayerBaseHeader* layers[MAX_NUM_LAYERS] = {};
        for (int i = 0; i < LayerCount; i++) {
            layers[i] = (const XrCompositionLayerBaseHeader*)&Layers[i];
        }

        XrFrameEndInfo endFrameInfo = {};
        endFrameInfo.type = XR_TYPE_FRAME_END_INFO;
        endFrameInfo.displayTime = frameState.predictedDisplayTime;
        endFrameInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endFrameInfo.layerCount = LayerCount;
        endFrameInfo.layers = layers;

        OXR(xrEndFrame(Session, &endFrameInfo));
    }

    EndSession();
    Shutdown(&Context);
    (*app->activity->vm).DetachCurrentThread();
}

} // namespace OVRFW
