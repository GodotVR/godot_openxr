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

#if defined(ANDROID)
#include <unistd.h>
#include <android/log.h>
#include <android/native_window_jni.h> // for native window JNI
#include <android_native_app_glue.h>
#endif // defined(ANDROID)

#include <assert.h>

#include <vector>

#include "XrPassthrough.h"
#include "XrPassthroughInput.h"

using namespace OVR;

#if !defined(EGL_OPENGL_ES3_BIT_KHR)
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

#define DEBUG 1
#define LOG_TAG "XrPassthrough"

#if defined(ANDROID)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#else
#include <cinttypes>
#define ALOGE(...)       \
    printf("ERROR: ");   \
    printf(__VA_ARGS__); \
    printf("\n")
#define ALOGV(...)       \
    printf("VERBOSE: "); \
    printf(__VA_ARGS__); \
    printf("\n")
#endif

namespace {

XrActionSet
CreateActionSet(XrInstance instance, int priority, const char* name, const char* localizedName) {
    XrActionSetCreateInfo asci = {};
    asci.type = XR_TYPE_ACTION_SET_CREATE_INFO;
    asci.next = nullptr;
    asci.priority = priority;
    strcpy(asci.actionSetName, name);
    strcpy(asci.localizedActionSetName, localizedName);
    XrActionSet actionSet = XR_NULL_HANDLE;
    OXR(xrCreateActionSet(instance, &asci, &actionSet));
    return actionSet;
}

XrAction CreateAction(
    XrActionSet actionSet,
    XrActionType type,
    const char* actionName,
    const char* localizedName,
    int countSubactionPaths = 0,
    XrPath* subactionPaths = nullptr) {
    ALOGV("CreateAction %s, %" PRIi32, actionName, countSubactionPaths);

    XrActionCreateInfo aci = {};
    aci.type = XR_TYPE_ACTION_CREATE_INFO;
    aci.next = nullptr;
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

XrActionSuggestedBinding
ActionSuggestedBinding(App& app, XrAction action, const char* bindingString) {
    XrActionSuggestedBinding asb;
    asb.action = action;
    XrPath bindingPath;
    OXR(xrStringToPath(app.Instance, bindingString, &bindingPath));
    asb.binding = bindingPath;
    return asb;
}

XrSpace CreateActionSpace(App& app, XrAction poseAction, XrPath subactionPath) {
    XrActionSpaceCreateInfo asci = {};
    asci.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
    asci.action = poseAction;
    asci.poseInActionSpace.orientation.w = 1.0f;
    asci.subactionPath = subactionPath;
    XrSpace actionSpace = XR_NULL_HANDLE;
    OXR(xrCreateActionSpace(app.Session, &asci, &actionSpace));
    return actionSpace;
}

XrActionStateBoolean GetActionStateBoolean(App& app, XrAction action) {
    XrActionStateGetInfo getInfo = {};
    getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    getInfo.action = action;

    XrActionStateBoolean state = {};
    state.type = XR_TYPE_ACTION_STATE_BOOLEAN;

    OXR(xrGetActionStateBoolean(app.Session, &getInfo, &state));
    return state;
}

bool ActionPoseIsActive(App& app, XrAction action, XrPath subactionPath) {
    XrActionStateGetInfo getInfo = {};
    getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    getInfo.action = action;
    getInfo.subactionPath = subactionPath;

    XrActionStatePose state = {};
    state.type = XR_TYPE_ACTION_STATE_POSE;
    OXR(xrGetActionStatePose(app.Session, &getInfo, &state));
    return state.isActive != XR_FALSE;
}

XrActionSet runningActionSet = XR_NULL_HANDLE;

XrAction aimPoseAction = XR_NULL_HANDLE;
XrAction gripPoseAction = XR_NULL_HANDLE;

XrAction boolAction = XR_NULL_HANDLE;
XrPath leftHandPath = XR_NULL_PATH;
XrPath rightHandPath = XR_NULL_PATH;
} // namespace

XrActionStateBoolean boolState;

bool leftControllerActive = false;
bool rightControllerActive = false;

XrSpace leftControllerAimSpace = XR_NULL_HANDLE;
XrSpace rightControllerAimSpace = XR_NULL_HANDLE;
XrSpace leftControllerGripSpace = XR_NULL_HANDLE;
XrSpace rightControllerGripSpace = XR_NULL_HANDLE;

void AppInput_init(App& app) {
    // Actions
    runningActionSet =
        CreateActionSet(app.Instance, 1, "running_action_set", "Action Set used on main loop");
    boolAction = CreateAction(runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "toggle", "Toggle");

    OXR(xrStringToPath(app.Instance, "/user/hand/left", &leftHandPath));
    OXR(xrStringToPath(app.Instance, "/user/hand/right", &rightHandPath));
    XrPath handSubactionPaths[2] = {leftHandPath, rightHandPath};

    aimPoseAction = CreateAction(
        runningActionSet, XR_ACTION_TYPE_POSE_INPUT, "aim_pose", nullptr, 2, handSubactionPaths);

    gripPoseAction = CreateAction(
        runningActionSet, XR_ACTION_TYPE_POSE_INPUT, "grip_pose", nullptr, 2, handSubactionPaths);

    XrPath interactionProfilePath = XR_NULL_PATH;

    OXR(xrStringToPath(
        app.Instance, "/interaction_profiles/oculus/touch_controller", &interactionProfilePath));

    // Action creation
    {
        // Map bindings

        std::vector<XrActionSuggestedBinding> bindings;
        bindings.push_back(
            ActionSuggestedBinding(app, boolAction, "/user/hand/left/input/trigger"));
        bindings.push_back(
            ActionSuggestedBinding(app, boolAction, "/user/hand/right/input/trigger"));
        bindings.push_back(
            ActionSuggestedBinding(app, aimPoseAction, "/user/hand/left/input/aim/pose"));
        bindings.push_back(
            ActionSuggestedBinding(app, aimPoseAction, "/user/hand/right/input/aim/pose"));
        bindings.push_back(
            ActionSuggestedBinding(app, gripPoseAction, "/user/hand/left/input/grip/pose"));
        bindings.push_back(
            ActionSuggestedBinding(app, gripPoseAction, "/user/hand/right/input/grip/pose"));

        XrInteractionProfileSuggestedBinding suggestedBindings = {};
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.interactionProfile = interactionProfilePath;
        suggestedBindings.suggestedBindings = &bindings[0];
        suggestedBindings.countSuggestedBindings = bindings.size();
        OXR(xrSuggestInteractionProfileBindings(app.Instance, &suggestedBindings));

        // Attach to session
        XrSessionActionSetsAttachInfo attachInfo = {};
        attachInfo.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
        attachInfo.countActionSets = 1;
        attachInfo.actionSets = &runningActionSet;
        OXR(xrAttachSessionActionSets(app.Session, &attachInfo));
    }
}

void AppInput_shutdown() {
    if (leftControllerAimSpace != XR_NULL_HANDLE) {
        OXR(xrDestroySpace(leftControllerAimSpace));
        OXR(xrDestroySpace(rightControllerAimSpace));
        OXR(xrDestroySpace(leftControllerGripSpace));
        OXR(xrDestroySpace(rightControllerGripSpace));
        leftControllerAimSpace = XR_NULL_HANDLE;
        rightControllerAimSpace = XR_NULL_HANDLE;
        leftControllerGripSpace = XR_NULL_HANDLE;
        rightControllerGripSpace = XR_NULL_HANDLE;
    }
}

void AppInput_syncActions(App& app) {
    // sync action data
    XrActiveActionSet activeActionSet = {};
    activeActionSet.actionSet = runningActionSet;
    activeActionSet.subactionPath = XR_NULL_PATH;

    XrActionsSyncInfo syncInfo = {};
    syncInfo.type = XR_TYPE_ACTIONS_SYNC_INFO;
    syncInfo.next = nullptr;
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &activeActionSet;
    OXR(xrSyncActions(app.Session, &syncInfo));

    // query input action states
    XrActionStateGetInfo getInfo = {};
    getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    getInfo.next = nullptr;
    getInfo.subactionPath = XR_NULL_PATH;

    boolState = GetActionStateBoolean(app, boolAction);

    if (leftControllerAimSpace == XR_NULL_HANDLE && app.SessionActive == true) {
        leftControllerAimSpace = CreateActionSpace(app, aimPoseAction, leftHandPath);
        rightControllerAimSpace = CreateActionSpace(app, aimPoseAction, rightHandPath);
        leftControllerGripSpace = CreateActionSpace(app, gripPoseAction, leftHandPath);
        rightControllerGripSpace = CreateActionSpace(app, gripPoseAction, rightHandPath);
    }

    leftControllerActive = ActionPoseIsActive(app, aimPoseAction, leftHandPath);
    rightControllerActive = ActionPoseIsActive(app, aimPoseAction, rightHandPath);
}
