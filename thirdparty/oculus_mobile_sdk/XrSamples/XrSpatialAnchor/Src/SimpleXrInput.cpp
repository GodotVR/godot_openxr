// Simple Xr Input

#if defined(ANDROID)
#include <android/log.h>
#endif

#include <inttypes.h>
#include <vector>

#include "SimpleXrInput.h"

#if defined(ANDROID)
#define DEBUG 1
#define OVR_LOG_TAG "SimpleXrInput"

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, OVR_LOG_TAG, __VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, OVR_LOG_TAG, __VA_ARGS__)
#else
#define DEBUG 1
#define ALOGE(...)       \
    printf("ERROR: ");   \
    printf(__VA_ARGS__); \
    printf("\n")
#define ALOGV(...)       \
    printf("VERBOSE: "); \
    printf(__VA_ARGS__); \
    printf("\n")
#endif

/*
================================================================================

OpenXR Utility Functions

================================================================================
*/

#if defined(DEBUG)
static void
OXR_CheckErrors(XrInstance instance, XrResult result, const char* function, bool failOnError) {
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
#define OXR(func) OXR_CheckErrors(instance, func, #func, true);
#else
#define OXR(func) OXR_CheckErrors(instance, func, #func, false);
#endif

struct SimpleXrInputImpl : public SimpleXrInput {
    XrInstance instance = XR_NULL_HANDLE;

    XrPath leftHandPath;
    XrPath rightHandPath;

    XrActionSet actionSet = XR_NULL_HANDLE;
    XrAction a = XR_NULL_HANDLE;
    XrAction b = XR_NULL_HANDLE;
    XrAction x = XR_NULL_HANDLE;
    XrAction y = XR_NULL_HANDLE;
    XrAction aim = XR_NULL_HANDLE;
    XrAction grip = XR_NULL_HANDLE;

    XrSession session = XR_NULL_HANDLE;

    struct AimGrip {
        XrSpace aim;
        XrSpace grip;
    };

    struct LeftRight {
        AimGrip left;
        AimGrip right;
    };

    LeftRight spaces;
    uint32_t syncCount;

    SimpleXrInputImpl(XrInstance instance_) : instance(instance_) {
        InitializeInput();
    }

    ~SimpleXrInputImpl() override {
        if (actionSet != XR_NULL_HANDLE) {
            OXR(xrDestroyActionSet(actionSet));
        }
    }

    void InitializeInput() {
        XrPath interactionProfile;
        OXR(xrStringToPath(
            instance, "/interaction_profiles/oculus/touch_controller", &interactionProfile));

        OXR(xrStringToPath(instance, "/user/hand/left", &leftHandPath));
        OXR(xrStringToPath(instance, "/user/hand/right", &rightHandPath));
        XrPath handSubactionPaths[2] = {leftHandPath, rightHandPath};

        actionSet = CreateActionSet(1, "main_action_set", "main ActionSet");
        a = CreateAction(actionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "a", "A button");
        b = CreateAction(actionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "b", "B button");
        x = CreateAction(actionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "x", "X button");
        y = CreateAction(actionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "y", "Y button");

        aim = CreateAction(
            actionSet, XR_ACTION_TYPE_POSE_INPUT, "aim_pose", nullptr, 2, handSubactionPaths);
        grip = CreateAction(
            actionSet, XR_ACTION_TYPE_POSE_INPUT, "grip_pose", nullptr, 2, handSubactionPaths);

        std::vector<XrActionSuggestedBinding> bindings;
        bindings.push_back(Suggest(a, "/user/hand/right/input/a/click"));
        bindings.push_back(Suggest(b, "/user/hand/right/input/b/click"));
        bindings.push_back(Suggest(x, "/user/hand/left/input/x/click"));
        bindings.push_back(Suggest(y, "/user/hand/left/input/y/click"));
        bindings.push_back(Suggest(aim, "/user/hand/left/input/aim/pose"));
        bindings.push_back(Suggest(aim, "/user/hand/right/input/aim/pose"));
        bindings.push_back(Suggest(grip, "/user/hand/left/input/grip/pose"));
        bindings.push_back(Suggest(grip, "/user/hand/right/input/grip/pose"));

        XrInteractionProfileSuggestedBinding suggestions = {
            XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestions.interactionProfile = interactionProfile;
        suggestions.suggestedBindings = &bindings[0];
        suggestions.countSuggestedBindings = bindings.size();
        OXR(xrSuggestInteractionProfileBindings(instance, &suggestions));

        syncCount = 0;
    }

    void BeginSession(XrSession session_) override {
        if (syncCount != 0) {
            ALOGV("SimpleXrInput::BeginSession call order invalid");
            return;
        }
        syncCount++;
        session = session_;
        // Attach actionSet to session
        XrSessionActionSetsAttachInfo attachInfo = {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
        attachInfo.countActionSets = 1;
        attachInfo.actionSets = &actionSet;
        OXR(xrAttachSessionActionSets(session, &attachInfo));
    }

    void EndSession() override {
        ALOGV("SimpleXrInput::EndSession");
        syncCount = 0;
    }

    void SyncActions() override {
        if (syncCount == 1) {
            spaces.left.aim = CreateActionSpace(aim, leftHandPath);
            spaces.right.aim = CreateActionSpace(aim, rightHandPath);
            spaces.left.grip = CreateActionSpace(grip, leftHandPath);
            spaces.right.grip = CreateActionSpace(grip, rightHandPath);
        }
        // sync action data
        XrActiveActionSet activeActionSet = {};
        activeActionSet.actionSet = actionSet;
        activeActionSet.subactionPath = XR_NULL_PATH;

        XrActionsSyncInfo syncInfo = {XR_TYPE_ACTIONS_SYNC_INFO};
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeActionSet;
        OXR(xrSyncActions(session, &syncInfo));
        syncCount++;
    }

    OVR::Posef FromControllerSpace(
        Side side,
        ControllerSpace controllerSpace,
        XrSpace baseSpace,
        XrTime atTime) override {
        AimGrip& ag = side == Side_Left ? spaces.left : spaces.right;
        XrSpace& space = controllerSpace == Controller_Aim ? ag.aim : ag.grip;
        XrSpaceLocation loc = {XR_TYPE_SPACE_LOCATION};
        OXR(xrLocateSpace(space, baseSpace, atTime, &loc));
        return *reinterpret_cast<OVR::Posef*>(&loc.pose);
    }

    bool A() override {
        return GetActionStateBoolean(a).currentState == XR_TRUE;
    }

    bool B() override {
        return GetActionStateBoolean(b).currentState == XR_TRUE;
    }

    bool X() override {
        return GetActionStateBoolean(x).currentState == XR_TRUE;
    }

    bool Y() override {
        return GetActionStateBoolean(y).currentState == XR_TRUE;
    }

    XrActionSet CreateActionSet(int priority, const char* name, const char* localizedName) {
        XrActionSetCreateInfo asci = {XR_TYPE_ACTION_SET_CREATE_INFO};
        asci.priority = priority;
        strcpy(asci.actionSetName, name);
        strcpy(asci.localizedActionSetName, localizedName);
        XrActionSet as = XR_NULL_HANDLE;
        OXR(xrCreateActionSet(instance, &asci, &as));
        return as;
    }

    XrAction CreateAction(
        XrActionSet as,
        XrActionType type,
        const char* actionName,
        const char* localizedName,
        int countSubactionPaths = 0,
        XrPath* subactionPaths = nullptr) {
        ALOGV("CreateAction %s, %" PRIi32, actionName, countSubactionPaths);

        XrActionCreateInfo aci = {XR_TYPE_ACTION_CREATE_INFO};
        aci.actionType = type;
        if (countSubactionPaths > 0) {
            aci.countSubactionPaths = countSubactionPaths;
            aci.subactionPaths = subactionPaths;
        }
        strcpy(aci.actionName, actionName);
        strcpy(aci.localizedActionName, localizedName ? localizedName : actionName);
        XrAction action = XR_NULL_HANDLE;
        OXR(xrCreateAction(as, &aci, &action));
        return action;
    }

    XrActionSuggestedBinding Suggest(XrAction action, const char* bindingString) {
        XrActionSuggestedBinding asb;
        asb.action = action;
        XrPath bindingPath;
        OXR(xrStringToPath(instance, bindingString, &bindingPath));
        asb.binding = bindingPath;
        return asb;
    }

    XrSpace CreateActionSpace(XrAction poseAction, XrPath subactionPath) {
        XrActionSpaceCreateInfo asci = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
        asci.action = poseAction;
        asci.poseInActionSpace.orientation.w = 1.0f;
        asci.subactionPath = subactionPath;
        XrSpace actionSpace = XR_NULL_HANDLE;
        OXR(xrCreateActionSpace(session, &asci, &actionSpace));
        return actionSpace;
    }

    XrActionStateBoolean GetActionStateBoolean(XrAction action) {
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = action;

        XrActionStateBoolean state = {XR_TYPE_ACTION_STATE_BOOLEAN};

        OXR(xrGetActionStateBoolean(session, &getInfo, &state));
        return state;
    }

    XrActionStateFloat GetActionStateFloat(XrAction action) {
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = action;

        XrActionStateFloat state = {XR_TYPE_ACTION_STATE_FLOAT};

        OXR(xrGetActionStateFloat(session, &getInfo, &state));
        return state;
    }

    XrActionStateVector2f GetActionStateVector2(XrAction action) {
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = action;

        XrActionStateVector2f state = {XR_TYPE_ACTION_STATE_VECTOR2F};

        OXR(xrGetActionStateVector2f(session, &getInfo, &state));
        return state;
    }

    bool ActionPoseIsActive(XrAction action, XrPath subactionPath) {
        XrActionStateGetInfo getInfo = {XR_TYPE_ACTION_STATE_GET_INFO};
        getInfo.action = action;
        getInfo.subactionPath = subactionPath;

        XrActionStatePose state = {XR_TYPE_ACTION_STATE_POSE};
        OXR(xrGetActionStatePose(session, &getInfo, &state));
        return state.isActive != XR_FALSE;
    }
};

SimpleXrInput* CreateSimpleXrInput(XrInstance instance_) {
    return new SimpleXrInputImpl(instance_);
}
