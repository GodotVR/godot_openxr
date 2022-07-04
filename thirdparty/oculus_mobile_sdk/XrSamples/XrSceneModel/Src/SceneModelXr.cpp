/************************************************************************************

Filename  : SceneModelXr.cpp
Content   : This sample uses the Android NativeActivity class.
Created   :
Authors   :

Copyright : Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h> // for memset
#include <map>
#include <math.h>
#include <string>
#include <time.h>
#include <unistd.h>
#include <unordered_set>
#include <pthread.h>
#include <sys/prctl.h> // for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/native_window_jni.h> // for native window JNI
#include <android_native_app_glue.h>
#include <assert.h>

#include "SceneModelHelpers.h"
#include "SceneModelGl.h"
#include "SceneModelXr.h"
#include "SimpleXrInput.h"

#include <openxr/fb_spatial_entity.h>
#include <openxr/fb_spatial_entity_storage.h>
#include <openxr/fb_spatial_entity_query.h>
#include <openxr/fb_spatial_entity_container.h>
#include <openxr/fb_scene.h>
#include <openxr/fb_scene_capture.h>

using namespace OVR;

#if !defined(EGL_OPENGL_ES3_BIT_KHR)
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

#define DEBUG 1
#define OVR_LOG_TAG "SceneModelXr"

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, OVR_LOG_TAG, __VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, OVR_LOG_TAG, __VA_ARGS__)

static const int CPU_LEVEL = 2;
static const int GPU_LEVEL = 3;
static const int NUM_MULTI_SAMPLES = 4;

static const uint32_t MAX_PERSISTENT_SPACES = 100;

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

union ovrCompositorLayer_Union {
    XrCompositionLayerProjection Projection;
    XrCompositionLayerQuad Quad;
    XrCompositionLayerCylinderKHR Cylinder;
    XrCompositionLayerCubeKHR Cube;
    XrCompositionLayerEquirectKHR Equirect;
};

enum { ovrMaxLayerCount = 16 };

// Forward declarations
XrInstance instance;

/*
================================================================================

Hex and Binary Conversion Functions

================================================================================
*/

std::string bin2hex(const uint8_t* src, uint32_t size) {
    std::string res;
    res.reserve(size * 2);
    const char hex[] = "0123456789ABCDEF";
    for (uint32_t i = 0; i < size; ++i) {
        uint8_t c = src[i];
        res += hex[c >> 4];
        res += hex[c & 0xf];
    }
    return res;
}

void hex2bin(const std::string& hex, uint8_t* dst, uint32_t size) {
    assert(hex.size() == size * 2);
    for (uint32_t i = 0; i < size; ++i) {
        dst[i] = 0;
        for (uint32_t j = 0; j < 2; ++j) {
            uint8_t c = hex[2 * i + j];
            if ('0' <= c && c <= '9') {
                c -= '0';
            } else if ('A' <= c && c <= 'F') {
                c -= 'A';
                c += 10;
            } else {
                assert(false);
            }
            dst[i] += c << (j == 0 ? 4 : 0);
        }
    }
}

std::string uuidToHexString(const XrUuidEXT& uuid) {
    return bin2hex(uuid.data, XR_UUID_SIZE_EXT);
}

void hexStringToUuid(const std::string& hex, XrUuidEXT& uuid) {
    hex2bin(hex, uuid.data, XR_UUID_SIZE_EXT);
}

bool isValid(const XrUuidEXT& uuid) {
    for (int i = 0; i < XR_UUID_SIZE_EXT; ++i) {
        if (uuid.data[i] > 0) {
            return true;
        }
    }
    return false;
}

static const std::map<std::string, XrColor4f> SemanticLabelToColorMap = {
    {"DESK", {1.f, 0.f, 0.f, 0.2f}},
    {"COUCH", {0.f, 1.f, 0.f, 0.2f}},
    {"FLOOR", {0.2f, 0.2f, 0.8f, 1.0f}},
    {"CEILING", {0.3f, 0.3f, 0.3f, 1.0f}},
    {"WALL_FACE", {0.5f, 0.5f, 0.f, 1.0f}},
    {"WINDOW_FRAME", {0.f, 0.5f, 0.6f, 0.8f}},
    {"DOOR_FRAME", {0.f, 0.2f, 0.2f, 1.0f}},
    {"OTHER", {1.f, 0.f, 1.f, 0.2f}}};

XrColor4f GetColorForSemanticLabels(const std::string& labels) {
    const XrColor4f defaultColor = {0.2f, 0.2f, 0.f, 0.2f};
    if (labels.empty()) {
        return defaultColor;
    }

    // Find the color for the first semantic label
    const std::string delimiter = ","; // Delimiter to separate labels
    const std::string firstLabel = labels.substr(0, labels.find(delimiter));
    if (firstLabel.empty() || !SemanticLabelToColorMap.count(firstLabel)) {
        return defaultColor;
    }

    return SemanticLabelToColorMap.at(firstLabel);
}

/*
================================================================================

OpenXR Utility Functions

================================================================================
*/

#if defined(DEBUG)
static void OXR_CheckErrors(XrResult result, const char* function, bool failOnError) {
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
#endif

#if defined(DEBUG)
#define OXR(func) OXR_CheckErrors(func, #func, true);
#else
#define OXR(func) OXR_CheckErrors(func, #func, false);
#endif

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

/*
================================================================================

ovrEgl

================================================================================
*/

struct ovrEgl {
    void Clear();
    void CreateContext(const ovrEgl* shareEgl);
    void DestroyContext();
    EGLint MajorVersion;
    EGLint MinorVersion;
    EGLDisplay Display;
    EGLConfig Config;
    EGLSurface TinySurface;
    EGLSurface MainSurface;
    EGLContext Context;
};

void ovrEgl::Clear() {
    MajorVersion = 0;
    MinorVersion = 0;
    Display = 0;
    Config = 0;
    TinySurface = EGL_NO_SURFACE;
    MainSurface = EGL_NO_SURFACE;
    Context = EGL_NO_CONTEXT;
}

void ovrEgl::CreateContext(const ovrEgl* shareEgl) {
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

void ovrEgl::DestroyContext() {
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

/*
================================================================================

ovrApp

================================================================================
*/

struct ovrExtensionFunctionPointers {
    PFN_xrEnumerateSpaceSupportedComponentsFB xrEnumerateSpaceSupportedComponentsFB = nullptr;
    PFN_xrGetSpaceComponentStatusFB xrGetSpaceComponentStatusFB = nullptr;
    PFN_xrSetSpaceComponentStatusFB xrSetSpaceComponentStatusFB = nullptr;
    PFN_xrQuerySpacesFB xrQuerySpacesFB = nullptr;
    PFN_xrRetrieveSpaceQueryResultsFB xrRetrieveSpaceQueryResultsFB = nullptr;
    PFN_xrGetSpaceBoundingBox2DFB xrGetSpaceBoundingBox2DFB = nullptr;
    PFN_xrGetSpaceBoundingBox3DFB xrGetSpaceBoundingBox3DFB = nullptr;
    PFN_xrGetSpaceSemanticLabelsFB xrGetSpaceSemanticLabelsFB = nullptr;
    PFN_xrGetSpaceBoundary2DFB xrGetSpaceBoundary2DFB = nullptr;
    PFN_xrGetSpaceRoomLayoutFB xrGetSpaceRoomLayoutFB = nullptr;
    PFN_xrGetSpaceContainerFB xrGetSpaceContainerFB = nullptr;
    PFN_xrRequestSceneCaptureFB xrRequestSceneCaptureFB = nullptr;
};

struct ovrApp {
    void Clear();
    void HandleSessionStateChanges(XrSessionState state);
    void HandleXrEvents();
    bool IsComponentSupported(XrSpace space, XrSpaceComponentTypeFB type);
    bool IsComponentEnabled(XrSpace space, XrSpaceComponentTypeFB type);
    void CollectRoomLayoutUuids(XrSpace space, std::unordered_set<std::string>& UuidSet);
    void CollectSpaceContainerUuids(XrSpace space, std::unordered_set<std::string>& UuidSet);

    ovrEgl Egl;
    ANativeWindow* NativeWindow;
    bool Resumed;
    bool Focused;

    XrSession Session;
    XrViewConfigurationProperties ViewportConfig;
    XrViewConfigurationView ViewConfigurationView[NUM_EYES];
    XrSystemId SystemId;
    XrSpace HeadSpace;
    XrSpace LocalSpace;
    XrSpace StageSpace;
    bool SessionActive;

    ovrExtensionFunctionPointers FunPtrs;

    int SwapInterval;
    int CpuLevel;
    int GpuLevel;
    // These threads will be marked as performance threads.
    int MainThreadTid;
    int RenderThreadTid;
    ovrCompositorLayer_Union Layers[ovrMaxLayerCount];
    int LayerCount;
    bool TouchPadDownLastFrame;

    enum class QueryType {
        None,
        QueryAll,
        QueryAllBounded2DEnabled,
        QueryAllRoomLayoutEnabled,
        QueryByUuids,
    };
    QueryType NextQueryType;
    bool QueryAllAnchorsInRoom = true;
    bool IsQueryComplete = true;

    enum class PlaneVisualizationMode {
        BoundingBox = 0,
        Boundary,
        Count, // Not a valid enum
    };
    PlaneVisualizationMode CurrentPlaneVisualizationMode = PlaneVisualizationMode::Boundary;

    bool ClearScene = false;

    XrSwapchain ColorSwapChain;
    uint32_t SwapChainLength;
    OVR::Vector3f StageBounds;
    // Provided by SceneModelGl, which is not aware of VrApi or OpenXR
    ovrAppRenderer AppRenderer;
    // 3D point input in base space for user surface creation.
    std::vector<OVR::Vector3f> ManualPointInput;

    std::map<XrAsyncRequestIdFB, XrSpace> DestroySpaceEventMap;

    std::unordered_set<std::string> UuidSet;
};

void ovrApp::Clear() {
    NativeWindow = NULL;
    Resumed = false;
    Focused = false;
    instance = XR_NULL_HANDLE;
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
    for (int i = 0; i < ovrMaxLayerCount; i++) {
        Layers[i] = {};
    }
    LayerCount = 0;
    CpuLevel = 2;
    GpuLevel = 2;
    MainThreadTid = 0;
    RenderThreadTid = 0;
    TouchPadDownLastFrame = false;
    NextQueryType = QueryType::None;
    IsQueryComplete = true;
    ClearScene = false;

    Egl.Clear();
    AppRenderer.Clear();
}

void ovrApp::HandleSessionStateChanges(XrSessionState state) {
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
                instance,
                "xrPerfSettingsSetPerformanceLevelEXT",
                (PFN_xrVoidFunction*)(&pfnPerfSettingsSetPerformanceLevelEXT)));

            OXR(pfnPerfSettingsSetPerformanceLevelEXT(
                Session, XR_PERF_SETTINGS_DOMAIN_CPU_EXT, cpuPerfLevel));
            OXR(pfnPerfSettingsSetPerformanceLevelEXT(
                Session, XR_PERF_SETTINGS_DOMAIN_GPU_EXT, gpuPerfLevel));

            PFN_xrSetAndroidApplicationThreadKHR pfnSetAndroidApplicationThreadKHR = NULL;
            OXR(xrGetInstanceProcAddr(
                instance,
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

bool ovrApp::IsComponentSupported(XrSpace space, XrSpaceComponentTypeFB type) {
    uint32_t numComponents = 0;
    OXR(FunPtrs.xrEnumerateSpaceSupportedComponentsFB(space, 0, &numComponents, nullptr));
    std::vector<XrSpaceComponentTypeFB> components(numComponents);
    OXR(FunPtrs.xrEnumerateSpaceSupportedComponentsFB(
        space, numComponents, &numComponents, components.data()));

    bool supported = false;
    for (uint32_t c = 0; c < numComponents; ++c) {
        if (components[c] == type) {
            supported = true;
            break;
        }
    }
    return supported;
}

bool ovrApp::IsComponentEnabled(XrSpace space, XrSpaceComponentTypeFB type) {
    XrSpaceComponentStatusFB status = {XR_TYPE_SPACE_COMPONENT_STATUS_FB, nullptr};
    OXR(FunPtrs.xrGetSpaceComponentStatusFB(space, type, &status));
    return (status.enabled && !status.changePending);
}

void ovrApp::CollectRoomLayoutUuids(XrSpace space, std::unordered_set<std::string>& uuidSet) {
    assert(FunPtrs.xrGetSpaceRoomLayoutFB != nullptr);

    XrRoomLayoutFB roomLayout = {};
    std::vector<XrUuidEXT> wallUuids;

    // First call
    OXR(FunPtrs.xrGetSpaceRoomLayoutFB(Session, space, &roomLayout));
    // If wallUuidCountOutput == 0, no walls in the component. The UUIDs of the ceiling and floor
    // has been returned if available
    if (roomLayout.wallUuidCountOutput != 0) {
        // Second call
        wallUuids.resize(roomLayout.wallUuidCountOutput);
        roomLayout.wallUuidCapacityInput = wallUuids.size();
        roomLayout.wallUuids = wallUuids.data();
        OXR(FunPtrs.xrGetSpaceRoomLayoutFB(Session, space, &roomLayout));
    }
    if (isValid(roomLayout.floorUuid)) {
        uuidSet.insert(uuidToHexString(roomLayout.floorUuid));
    }
    if (isValid(roomLayout.ceilingUuid)) {
        uuidSet.insert(uuidToHexString(roomLayout.ceilingUuid));
    }
    for (uint32_t i = 0; i < roomLayout.wallUuidCountOutput; i++) {
        uuidSet.insert(uuidToHexString(roomLayout.wallUuids[i]));
    }
}

void ovrApp::CollectSpaceContainerUuids(XrSpace space, std::unordered_set<std::string>& uuidSet) {
    assert(FunPtrs.xrGetSpaceContainerFB != nullptr);

    XrSpaceContainerFB spaceContainer = {};
    // First call
    OXR(FunPtrs.xrGetSpaceContainerFB(Session, space, &spaceContainer));
    if (spaceContainer.uuidCountOutput == 0) {
        // No UUIDs
        return;
    }
    // Second call
    std::vector<XrUuidEXT> uuids(spaceContainer.uuidCountOutput);
    spaceContainer.uuidCapacityInput = uuids.size();
    spaceContainer.uuids = uuids.data();
    OXR(FunPtrs.xrGetSpaceContainerFB(Session, space, &spaceContainer));

    for (uint32_t i = 0; i < spaceContainer.uuidCountOutput; i++) {
        uuidSet.insert(uuidToHexString(spaceContainer.uuids[i]));
    }
}

std::string GetSemanticLabels(ovrApp& app, const XrSpace space) {
    XrSemanticLabelsFB labels = {XR_TYPE_SEMANTIC_LABELS_FB, nullptr, 0};

    if (!app.IsComponentEnabled(space, XR_SPACE_COMPONENT_TYPE_SEMANTIC_LABELS_FB)) {
        return std::string();
    }

    // First call.
    OXR(app.FunPtrs.xrGetSpaceSemanticLabelsFB(app.Session, space, &labels));
    // Second call
    std::vector<char> labelData(labels.byteCountOutput);
    labels.byteCapacityInput = labelData.size();
    labels.labels = labelData.data();
    OXR(app.FunPtrs.xrGetSpaceSemanticLabelsFB(app.Session, space, &labels));

    return std::string(labels.labels, labels.byteCountOutput);
}

bool UpdateOvrPlane(ovrApp& app, ovrPlane& plane) {
    const auto labels = GetSemanticLabels(app, plane.Space);
    const auto color = GetColorForSemanticLabels(labels);
    float zOffset = 0.0f;
    // Move windows and doors so they appear in front of the walls
    if (labels.find("WINDOW_FRAME") != std::string::npos ||
        labels.find("DOOR_FRAME") != std::string::npos) {
        zOffset = 0.01f; // move 1cm on Z+
    }

    if (app.CurrentPlaneVisualizationMode == ovrApp::PlaneVisualizationMode::BoundingBox) {
        XrResult res;
        XrRect2Df boundingBox2D;

        assert(app.FunPtrs.xrGetSpaceBoundingBox2DFB != nullptr);
        OXR(res = app.FunPtrs.xrGetSpaceBoundingBox2DFB(app.Session, plane.Space, &boundingBox2D));
        if (XR_FAILED(res)) {
            ALOGE("Failed getting bounding box 2D!");
            return false;
        }
        plane.Update(boundingBox2D, color, zOffset);
        return true;
    } else if (app.CurrentPlaneVisualizationMode == ovrApp::PlaneVisualizationMode::Boundary) {
        XrResult res;
        XrBoundary2DFB boundary2D = {XR_TYPE_BOUNDARY_2D_FB, nullptr, 0};

        assert(app.FunPtrs.xrGetSpaceBoundary2DFB != nullptr);
        // First call
        OXR(res = app.FunPtrs.xrGetSpaceBoundary2DFB(app.Session, plane.Space, &boundary2D));
        if (XR_FAILED(res)) {
            ALOGE("Failed getting boundary 2D!");
            return false;
        }
        // Second call
        std::vector<XrVector2f> vertices(boundary2D.vertexCountOutput);
        boundary2D.vertexCapacityInput = vertices.size();
        boundary2D.vertices = vertices.data();
        OXR(res = app.FunPtrs.xrGetSpaceBoundary2DFB(app.Session, plane.Space, &boundary2D));
        if (XR_FAILED(res)) {
            ALOGE("Failed getting boundary 2D!");
            return false;
        }
        plane.Update(boundary2D, color, zOffset);
        return true;
    }
    return false;
}

bool UpdateOvrVolume(ovrApp& app, ovrVolume& volume) {
    XrResult res;
    XrRect3DfFB boundingBox3D;
    assert(app.FunPtrs.xrGetSpaceBoundingBox3DFB != nullptr);
    OXR(res = app.FunPtrs.xrGetSpaceBoundingBox3DFB(app.Session, volume.Space, &boundingBox3D));
    if (XR_FAILED(res)) {
        ALOGE("Failed getting bounding box 3D!");
        return false;
    }
    const auto labels = GetSemanticLabels(app, volume.Space);

    volume.Update(boundingBox3D, GetColorForSemanticLabels(labels));
    return true;
}

void ovrApp::HandleXrEvents() {
    XrEventDataBuffer eventDataBuffer = {};

    // Poll for events
    for (;;) {
        XrEventDataBaseHeader* baseEventHeader = (XrEventDataBaseHeader*)(&eventDataBuffer);
        baseEventHeader->type = XR_TYPE_EVENT_DATA_BUFFER;
        baseEventHeader->next = NULL;
        XrResult r;
        OXR(r = xrPollEvent(instance, &eventDataBuffer));
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
            case XR_TYPE_EVENT_DATA_SPACE_SET_STATUS_COMPLETE_FB: {
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_SPACE_SET_STATUS_COMPLETE_FB");
                const XrEventDataSpaceSetStatusCompleteFB* setStatusComplete =
                    (XrEventDataSpaceSetStatusCompleteFB*)(baseEventHeader);
                if (setStatusComplete->result == XR_SUCCESS) {
                    if (setStatusComplete->componentType == XR_SPACE_COMPONENT_TYPE_LOCATABLE_FB) {
                        if (IsComponentEnabled(
                                setStatusComplete->space, XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB)) {
                            ovrPlane plane(setStatusComplete->space);
                            if (UpdateOvrPlane(*this, plane)) {
                                AppRenderer.Scene.Planes.emplace_back(plane);
                            }
                        }
                        if (IsComponentEnabled(
                                setStatusComplete->space, XR_SPACE_COMPONENT_TYPE_BOUNDED_3D_FB)) {
                            ovrVolume volume(setStatusComplete->space);
                            if (UpdateOvrVolume(*this, volume)) {
                                AppRenderer.Scene.Volumes.emplace_back(volume);
                            }
                        }
                    }
                }
            } break;
            case XR_TYPE_EVENT_DATA_SPACE_QUERY_RESULTS_AVAILABLE_FB: {
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_SPACE_QUERY_RESULTS_AVAILABLE_FB");
                const XrEventDataSpaceQueryResultsAvailableFB* resultsAvailable =
                    (XrEventDataSpaceQueryResultsAvailableFB*)(baseEventHeader);

                XrResult res = XR_SUCCESS;

                XrSpaceQueryResultsFB queryResults{XR_TYPE_SPACE_QUERY_RESULTS_FB};
                queryResults.resultCapacityInput = 0;
                queryResults.resultCountOutput = 0;
                queryResults.results = nullptr;

                res = FunPtrs.xrRetrieveSpaceQueryResultsFB(
                    Session, resultsAvailable->requestId, &queryResults);
                if (res != XR_SUCCESS) {
                    ALOGV("xrRetrieveSpaceQueryResultsFB: error %u", res);
                    break;
                }

                std::vector<XrSpaceQueryResultFB> results(queryResults.resultCountOutput);
                queryResults.resultCapacityInput = results.size();
                queryResults.resultCountOutput = 0;
                queryResults.results = results.data();

                res = FunPtrs.xrRetrieveSpaceQueryResultsFB(
                    Session, resultsAvailable->requestId, &queryResults);
                if (res != XR_SUCCESS) {
                    ALOGV("xrRetrieveSpaceQueryResultsFB: error %u", res);
                    break;
                }

                for (uint32_t i = 0; i < queryResults.resultCountOutput; ++i) {
                    auto& result = results[i];

                    if (IsComponentSupported(result.space, XR_SPACE_COMPONENT_TYPE_LOCATABLE_FB)) {
                        XrSpaceComponentStatusSetInfoFB request = {
                            XR_TYPE_SPACE_COMPONENT_STATUS_SET_INFO_FB,
                            nullptr,
                            XR_SPACE_COMPONENT_TYPE_LOCATABLE_FB,
                            true,
                            0};
                        XrAsyncRequestIdFB requestId;
                        res =
                            FunPtrs.xrSetSpaceComponentStatusFB(result.space, &request, &requestId);
                        if (res == XR_ERROR_SPACE_COMPONENT_STATUS_ALREADY_SET_FB) {
                            if (IsComponentEnabled(
                                    result.space, XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB)) {
                                ovrPlane plane(result.space);
                                if (UpdateOvrPlane(*this, plane)) {
                                    AppRenderer.Scene.Planes.emplace_back(plane);
                                }
                            }
                            if (IsComponentEnabled(
                                    result.space, XR_SPACE_COMPONENT_TYPE_BOUNDED_3D_FB)) {
                                ovrVolume volume(result.space);
                                if (UpdateOvrVolume(*this, volume)) {
                                    AppRenderer.Scene.Volumes.emplace_back(volume);
                                }
                            }
                        }
                    }

                    if (QueryAllAnchorsInRoom) {
                        if (IsComponentSupported(
                                result.space, XR_SPACE_COMPONENT_TYPE_SPACE_CONTAINER_FB) &&
                            IsComponentEnabled(
                                result.space, XR_SPACE_COMPONENT_TYPE_SPACE_CONTAINER_FB)) {
                            CollectSpaceContainerUuids(result.space, UuidSet);
                        }
                    } else {
                        if (IsComponentSupported(
                                result.space, XR_SPACE_COMPONENT_TYPE_ROOM_LAYOUT_FB) &&
                            IsComponentEnabled(
                                result.space, XR_SPACE_COMPONENT_TYPE_ROOM_LAYOUT_FB)) {
                            CollectRoomLayoutUuids(result.space, UuidSet);
                        }
                    }
                }
            } break;
            case XR_TYPE_EVENT_DATA_SPACE_QUERY_COMPLETE_FB: {
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_SPACE_QUERY_COMPLETE_FB");
                IsQueryComplete = true;
            } break;
            case XR_TYPE_EVENT_DATA_SCENE_CAPTURE_COMPLETE_FB: {
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_SCENE_CAPTURE_COMPLETE_FB");

                const XrEventDataSceneCaptureCompleteFB* captureResult =
                    (XrEventDataSceneCaptureCompleteFB*)(baseEventHeader);
                if (captureResult->result == XR_SUCCESS) {
                    ALOGV(
                        "xrPollEvent: Scene capture (ID = %" PRIu64 ") succeeded",
                        captureResult->requestId);
                } else {
                    ALOGE(
                        "xrPollEvent: Scene capture (ID = %" PRIu64 ") failed with an error %d",
                        captureResult->requestId,
                        captureResult->result);
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
    ovrApp& app = *(ovrApp*)androidApp->userData;

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

static bool QueryAllAnchors(ovrApp& app) {
    XrSpaceQueryInfoFB queryInfo = {
        XR_TYPE_SPACE_QUERY_INFO_FB,
        nullptr,
        XR_SPACE_QUERY_ACTION_LOAD_FB,
        MAX_PERSISTENT_SPACES,
        0,
        nullptr,
        nullptr};

    XrAsyncRequestIdFB requestId;
    OXR(app.FunPtrs.xrQuerySpacesFB(
        app.Session, (XrSpaceQueryInfoBaseHeaderFB*)&queryInfo, &requestId));
    return true;
}

static bool QueryAllAnchorsWithSpecificComponentEnabled(
    ovrApp& app,
    const XrSpaceComponentTypeFB componentType) {
    XrSpaceStorageLocationFilterInfoFB storageLocationFilterInfo = {
        XR_TYPE_SPACE_STORAGE_LOCATION_FILTER_INFO_FB, nullptr, XR_SPACE_STORAGE_LOCATION_LOCAL_FB};

    XrSpaceComponentFilterInfoFB componentFilterInfo = {
        XR_TYPE_SPACE_COMPONENT_FILTER_INFO_FB, &storageLocationFilterInfo, componentType};

    XrSpaceQueryInfoFB queryInfo = {
        XR_TYPE_SPACE_QUERY_INFO_FB,
        nullptr,
        XR_SPACE_QUERY_ACTION_LOAD_FB,
        MAX_PERSISTENT_SPACES,
        0,
        (XrSpaceFilterInfoBaseHeaderFB*)&componentFilterInfo,
        nullptr};

    XrAsyncRequestIdFB requestId;
    OXR(app.FunPtrs.xrQuerySpacesFB(
        app.Session, (XrSpaceQueryInfoBaseHeaderFB*)&queryInfo, &requestId));
    return true;
}

static bool QueryAnchorsByUuids(ovrApp& app) {
    if (app.UuidSet.empty()) {
        ALOGV("No UUID to query");
        return false;
    }

    std::vector<XrUuidEXT> uuidsToQuery(app.UuidSet.size());
    int i = 0;
    for (const std::string& uuid : app.UuidSet) {
        hexStringToUuid(uuid, uuidsToQuery[i++]);
        ALOGV("Added UUID %s to the query", uuid.c_str());
    }

    XrSpaceStorageLocationFilterInfoFB storageLocationFilterInfo = {
        XR_TYPE_SPACE_STORAGE_LOCATION_FILTER_INFO_FB, nullptr, XR_SPACE_STORAGE_LOCATION_LOCAL_FB};

    XrSpaceUuidFilterInfoFB uuidFilterInfo = {
        XR_TYPE_SPACE_UUID_FILTER_INFO_FB,
        &storageLocationFilterInfo,
        (uint32_t)uuidsToQuery.size(),
        uuidsToQuery.data()};

    XrSpaceQueryInfoFB queryInfo = {
        XR_TYPE_SPACE_QUERY_INFO_FB,
        nullptr,
        XR_SPACE_QUERY_ACTION_LOAD_FB,
        MAX_PERSISTENT_SPACES,
        0,
        (XrSpaceFilterInfoBaseHeaderFB*)&uuidFilterInfo,
        nullptr};

    XrAsyncRequestIdFB requestId;
    OXR(app.FunPtrs.xrQuerySpacesFB(
        app.Session, (XrSpaceQueryInfoBaseHeaderFB*)&queryInfo, &requestId));
    return true;
}

void UpdateStageBounds(ovrApp& app) {
    XrExtent2Df stageBounds = {};

    XrResult result;
    OXR(result = xrGetReferenceSpaceBoundsRect(
            app.Session, XR_REFERENCE_SPACE_TYPE_STAGE, &stageBounds));
    if (result != XR_SUCCESS) {
        ALOGV("Stage bounds query failed: using small defaults");
        stageBounds.width = 1.0f;
        stageBounds.height = 1.0f;
    }

    app.StageBounds = OVR::Vector3f(stageBounds.width * 0.5f, 1.0f, stageBounds.height * 0.5f);
}

void UpdateScenePlanes(ovrApp& app, const XrFrameState& frameState) {
    auto& scene = app.AppRenderer.Scene;
    for (auto& plane : scene.Planes) {
        XrSpaceLocation spaceLocation = {};
        spaceLocation.type = XR_TYPE_SPACE_LOCATION;
        XrResult res = XR_SUCCESS;
        OXR(res = xrLocateSpace(
                plane.Space, app.LocalSpace, frameState.predictedDisplayTime, &spaceLocation));
        if (XR_FAILED(res)) {
            ALOGE("Failed getting anchor pose!");
            continue;
        }
        plane.SetPose(spaceLocation.pose);
    }
}

void CycleScenePlaneRenderingMode(ovrApp& app) {
    app.CurrentPlaneVisualizationMode = static_cast<ovrApp::PlaneVisualizationMode>(
        (static_cast<int>(app.CurrentPlaneVisualizationMode) + 1) %
        (static_cast<int>(ovrApp::PlaneVisualizationMode::Count)));
    auto& scene = app.AppRenderer.Scene;
    for (auto& plane : scene.Planes) {
        UpdateOvrPlane(app, plane);
    }
}

void UpdateSceneVolumes(ovrApp& app, const XrFrameState& frameState) {
    auto& scene = app.AppRenderer.Scene;

    for (auto& volume : scene.Volumes) {
        XrSpaceLocation spaceLocation = {};
        spaceLocation.type = XR_TYPE_SPACE_LOCATION;
        XrResult res = XR_SUCCESS;
        OXR(res = xrLocateSpace(
                volume.Space, app.LocalSpace, frameState.predictedDisplayTime, &spaceLocation));
        if (XR_FAILED(res)) {
            ALOGE("Failed getting anchor pose!");
            continue;
        }
        volume.SetPose(spaceLocation.pose);
    }
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

    ovrApp app;
    app.Clear();

    // Start the app with a query.
    app.NextQueryType = ovrApp::QueryType::QueryAll;

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
        XR_FB_SPATIAL_ENTITY_EXTENSION_NAME,
        XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME,
        XR_FB_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME,
        XR_FB_SPATIAL_ENTITY_CONTAINER_EXTENSION_NAME,
        XR_FB_SCENE_EXTENSION_NAME,
        XR_FB_SCENE_CAPTURE_EXTENSION_NAME};
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
    strcpy(appInfo.applicationName, "SceneModelXr");
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
    OXR(initResult = xrCreateInstance(&instanceCreateInfo, &instance));
    if (initResult != XR_SUCCESS) {
        ALOGE("Failed to create XR instance: %d.", initResult);
        exit(1);
    }

    XrInstanceProperties instanceInfo;
    instanceInfo.type = XR_TYPE_INSTANCE_PROPERTIES;
    instanceInfo.next = NULL;
    OXR(xrGetInstanceProperties(instance, &instanceInfo));
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
    OXR(initResult = xrGetSystem(instance, &systemGetInfo, &systemId));
    if (initResult != XR_SUCCESS) {
        ALOGE("Failed to get system.");
        exit(1);
    }

    XrSystemProperties systemProperties = {};
    systemProperties.type = XR_TYPE_SYSTEM_PROPERTIES;
    OXR(xrGetSystemProperties(instance, systemId, &systemProperties));

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

    assert(ovrMaxLayerCount <= systemProperties.graphicsProperties.maxLayerCount);

    // Get the graphics requirements.
    PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = NULL;
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrGetOpenGLESGraphicsRequirementsKHR",
        (PFN_xrVoidFunction*)(&pfnGetOpenGLESGraphicsRequirementsKHR)));

    XrGraphicsRequirementsOpenGLESKHR graphicsRequirements = {};
    graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR;
    OXR(pfnGetOpenGLESGraphicsRequirementsKHR(instance, systemId, &graphicsRequirements));

    // Create the EGL Context
    app.Egl.CreateContext(nullptr);

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

    // Create the OpenXR Session.
    XrGraphicsBindingOpenGLESAndroidKHR graphicsBindingAndroidGLES = {};
    graphicsBindingAndroidGLES.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR;
    graphicsBindingAndroidGLES.next = NULL;
    graphicsBindingAndroidGLES.display = app.Egl.Display;
    graphicsBindingAndroidGLES.config = app.Egl.Config;
    graphicsBindingAndroidGLES.context = app.Egl.Context;

    XrSessionCreateInfo sessionCreateInfo = {};
    sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
    sessionCreateInfo.next = &graphicsBindingAndroidGLES;
    sessionCreateInfo.createFlags = 0;
    sessionCreateInfo.systemId = app.SystemId;

    OXR(initResult = xrCreateSession(instance, &sessionCreateInfo, &app.Session));
    if (initResult != XR_SUCCESS) {
        ALOGE("Failed to create XR session: %d.", initResult);
        exit(1);
    }

    // App only supports the primary stereo view config.
    const XrViewConfigurationType supportedViewConfigType =
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    // Enumerate the viewport configurations.
    uint32_t viewportConfigTypeCount = 0;
    OXR(xrEnumerateViewConfigurations(instance, app.SystemId, 0, &viewportConfigTypeCount, NULL));

    auto viewportConfigurationTypes = new XrViewConfigurationType[viewportConfigTypeCount];

    OXR(xrEnumerateViewConfigurations(
        instance,
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
            instance, app.SystemId, viewportConfigType, &viewportConfig));
        ALOGV(
            "FovMutable=%s ConfigurationType %d",
            viewportConfig.fovMutable ? "true" : "false",
            viewportConfig.viewConfigurationType);

        uint32_t viewCount;
        OXR(xrEnumerateViewConfigurationViews(
            instance, app.SystemId, viewportConfigType, 0, &viewCount, NULL));

        if (viewCount > 0) {
            auto elements = new XrViewConfigurationView[viewCount];

            for (uint32_t e = 0; e < viewCount; e++) {
                elements[e].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
                elements[e].next = NULL;
            }

            OXR(xrEnumerateViewConfigurationViews(
                instance, app.SystemId, viewportConfigType, viewCount, &viewCount, elements));

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
        instance, app.SystemId, supportedViewConfigType, &app.ViewportConfig));

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

    app.AppRenderer.Create(
        format, width, height, NUM_MULTI_SAMPLES, app.SwapChainLength, colorTextures);

    delete[] images;
    delete[] colorTextures;

    androidApp->userData = &app;
    androidApp->onAppCmd = app_handle_cmd;

    bool stageBoundsDirty = true;

    double startTimeInSeconds = -1.0;

    int frameCount = -1;

    SimpleXrInput* input = CreateSimpleXrInput(instance);
    input->BeginSession(app.Session);

    /// Hook up extensions for spatial entity
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrEnumerateSpaceSupportedComponentsFB",
        (PFN_xrVoidFunction*)(&app.FunPtrs.xrEnumerateSpaceSupportedComponentsFB)));
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrGetSpaceComponentStatusFB",
        (PFN_xrVoidFunction*)(&app.FunPtrs.xrGetSpaceComponentStatusFB)));
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrSetSpaceComponentStatusFB",
        (PFN_xrVoidFunction*)(&app.FunPtrs.xrSetSpaceComponentStatusFB)));
    OXR(xrGetInstanceProcAddr(
        instance, "xrQuerySpacesFB", (PFN_xrVoidFunction*)(&app.FunPtrs.xrQuerySpacesFB)));
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrRetrieveSpaceQueryResultsFB",
        (PFN_xrVoidFunction*)(&app.FunPtrs.xrRetrieveSpaceQueryResultsFB)));
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrGetSpaceBoundingBox2DFB",
        (PFN_xrVoidFunction*)(&app.FunPtrs.xrGetSpaceBoundingBox2DFB)));
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrGetSpaceBoundingBox3DFB",
        (PFN_xrVoidFunction*)(&app.FunPtrs.xrGetSpaceBoundingBox3DFB)));
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrGetSpaceSemanticLabelsFB",
        (PFN_xrVoidFunction*)(&app.FunPtrs.xrGetSpaceSemanticLabelsFB)));
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrGetSpaceBoundary2DFB",
        (PFN_xrVoidFunction*)(&app.FunPtrs.xrGetSpaceBoundary2DFB)));
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrGetSpaceRoomLayoutFB",
        (PFN_xrVoidFunction*)(&app.FunPtrs.xrGetSpaceRoomLayoutFB)));
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrGetSpaceContainerFB",
        (PFN_xrVoidFunction*)(&app.FunPtrs.xrGetSpaceContainerFB)));
    OXR(xrGetInstanceProcAddr(
        instance,
        "xrRequestSceneCaptureFB",
        (PFN_xrVoidFunction*)(&app.FunPtrs.xrRequestSceneCaptureFB)));

    // Two values for left and right controllers.
    std::array<XrTime, 2> lastInputTimes = {0, 0};
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
            continue;
        }

        input->SyncActions();

        // Create the scene if not yet created.
        // The scene is created here to be able to show a loading icon.
        if (!app.AppRenderer.Scene.IsCreated()) {
            // Create the scene.
            app.AppRenderer.Scene.Create();
        }

        if (stageBoundsDirty) {
            UpdateStageBounds(app);
            stageBoundsDirty = false;
        }

        if (app.ClearScene) {
            // This is called after the app starts, or after Button X is pressed.
            for (auto& plane : app.AppRenderer.Scene.Planes) {
                plane.Geometry.DestroyVAO();
                plane.Geometry.Destroy();
            }
            app.AppRenderer.Scene.Planes.clear();

            for (auto& volume : app.AppRenderer.Scene.Volumes) {
                volume.Geometry.DestroyVAO();
                volume.Geometry.Destroy();
            }
            app.AppRenderer.Scene.Volumes.clear();

            app.ClearScene = false;

            app.UuidSet.clear();

            app.IsQueryComplete = true;
        }

        if (app.NextQueryType != ovrApp::QueryType::None && app.IsQueryComplete) {
            // Start the next query if there is a new query and the current query has completed
            bool result = false;
            if (app.NextQueryType == ovrApp::QueryType::QueryAll) {
                result = QueryAllAnchors(app);
                app.NextQueryType = ovrApp::QueryType::None;
            } else if (app.NextQueryType == ovrApp::QueryType::QueryAllBounded2DEnabled) {
                result = QueryAllAnchorsWithSpecificComponentEnabled(
                    app, XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB);
                app.NextQueryType = ovrApp::QueryType::None;
            } else if (app.NextQueryType == ovrApp::QueryType::QueryAllRoomLayoutEnabled) {
                result = QueryAllAnchorsWithSpecificComponentEnabled(
                    app, XR_SPACE_COMPONENT_TYPE_ROOM_LAYOUT_FB);
                app.NextQueryType = ovrApp::QueryType::QueryByUuids;
            } else if (app.NextQueryType == ovrApp::QueryType::QueryByUuids) {
                result = QueryAnchorsByUuids(app);
                app.NextQueryType = ovrApp::QueryType::None;
            }
            app.IsQueryComplete = result ? false : true;
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

        XrSpaceLocation loc = {};
        loc.type = XR_TYPE_SPACE_LOCATION;
        OXR(xrLocateSpace(app.HeadSpace, app.LocalSpace, frameState.predictedDisplayTime, &loc));
        XrPosef xfLocalFromHead = loc.pose;

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

        UpdateScenePlanes(app, frameState);

        UpdateSceneVolumes(app, frameState);

        assert(input != nullptr);
        // A Button: Refresh all by querying room entity that has room layout component enabled.
        if (input->IsButtonAPressed()) {
            app.ClearScene = true;
            app.NextQueryType = ovrApp::QueryType::QueryAllRoomLayoutEnabled;
            app.QueryAllAnchorsInRoom = true;
            lastInputTimes[1] = frameState.predictedDisplayTime;
        }

        // B Button: Refresh anchors by querying all that have Bounded 2D component enabled.
        if (input->IsButtonBPressed()) {
            app.ClearScene = true;
            app.NextQueryType = ovrApp::QueryType::QueryAllBounded2DEnabled;
            lastInputTimes[1] = frameState.predictedDisplayTime;
        }

        // X Button: Refresh anchors by querying all.
        if (input->IsButtonXPressed()) {
            app.ClearScene = true;
            app.NextQueryType = ovrApp::QueryType::QueryAll;
            lastInputTimes[0] = frameState.predictedDisplayTime;
        }

        // Y Button: Refresh all by querying room entity for room layout anchors
        // (ceiling/floor/walls) only.
        if (input->IsButtonYPressed()) {
            app.ClearScene = true;
            app.NextQueryType = ovrApp::QueryType::QueryAllRoomLayoutEnabled;
            app.QueryAllAnchorsInRoom = false;
            lastInputTimes[0] = frameState.predictedDisplayTime;
        }

        // Left Index Trigger: Toggle rendering mode
        if (input->IsTriggerPressed(SimpleXrInput::Side_Left)) {
            CycleScenePlaneRenderingMode(app);
        }

        // Right Index Trigger: Request scene capture.
        if (input->IsTriggerPressed(SimpleXrInput::Side_Right)) {
            XrAsyncRequestIdFB requestId;
            XrSceneCaptureRequestInfoFB request;
            request.type = XR_TYPE_SCENE_CAPTURE_REQUEST_INFO_FB;
            request.next = nullptr;
            request.requestByteCount = 0;
            request.request = nullptr;
            OXR(app.FunPtrs.xrRequestSceneCaptureFB(app.Session, &request, &requestId));
            lastInputTimes[1] = frameState.predictedDisplayTime;
        }

        // Simple animation
        double timeInSeconds = FromXrTime(frameState.predictedDisplayTime);
        if (startTimeInSeconds < 0.0) {
            startTimeInSeconds = timeInSeconds;
        }

        ovrAppRenderer::FrameIn frameIn;
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
            loc = {};
            loc.type = XR_TYPE_SPACE_LOCATION;
            OXR(xrLocateSpace(
                app.StageSpace, app.LocalSpace, frameState.predictedDisplayTime, &loc));
            XrPosef xfLocalFromStage = loc.pose;

            frameIn.HasStage = true;
            frameIn.StagePose = OvrFromXr(xfLocalFromStage);
            frameIn.StageScale = app.StageBounds;
        } else {
            frameIn.HasStage = false;
        }

        for (int controllerIndex = 0; controllerIndex < 2; ++controllerIndex) {
            // If there is an input signal from this controller, pause rendring this controller
            // for 0.1 second to give the user a sense of feedback.
            frameIn.RenderController[controllerIndex] =
                frameState.predictedDisplayTime > lastInputTimes[controllerIndex] + 100000000;
            if (frameIn.RenderController[controllerIndex]) {
                frameIn.ControllerPoses[controllerIndex] = input->FromControllerSpace(
                    controllerIndex == 0 ? SimpleXrInput::Side_Left : SimpleXrInput::Side_Right,
                    SimpleXrInput::Controller_Aim,
                    app.LocalSpace,
                    frameState.predictedDisplayTime);
            }
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

        app.AppRenderer.RenderFrame(frameIn);

        XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, NULL};
        OXR(xrReleaseSwapchainImage(app.ColorSwapChain, &releaseInfo));

        // Set-up the compositor layers for this frame.
        // NOTE: Multiple independent layers are allowed, but they need to be added
        // in a depth consistent order.

        XrCompositionLayerProjectionView proj_views[2] = {};

        app.LayerCount = 0;
        memset(app.Layers, 0, sizeof(ovrCompositorLayer_Union) * ovrMaxLayerCount);

        XrCompositionLayerProjection proj_layer = {};
        proj_layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
        proj_layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        proj_layer.layerFlags |= XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
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
        const XrCompositionLayerBaseHeader* layers[ovrMaxLayerCount] = {};
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

    input->EndSession();

    delete input;

    app.AppRenderer.Destroy();

    delete[] projections;

    app.Egl.DestroyContext();

    OXR(xrDestroySpace(app.HeadSpace));
    OXR(xrDestroySpace(app.LocalSpace));
    // StageSpace is optional.
    if (app.StageSpace != XR_NULL_HANDLE) {
        OXR(xrDestroySpace(app.StageSpace));
    }
    OXR(xrDestroySession(app.Session));
    OXR(xrDestroyInstance(instance));

    (*androidApp->activity->vm).DetachCurrentThread();
}
