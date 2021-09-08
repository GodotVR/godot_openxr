////////////////////////////////////////////////////////////////////////////////////////////////
// Helper calls and singleton container for accessing openxr

#include <ARVRServer.hpp>
#include <CameraMatrix.hpp>
#include <JSON.hpp>
#include <JSONParseResult.hpp>
#include <ProjectSettings.hpp>

#include "openxr/OpenXRApi.h"

#include <cmath>
#include <map>

using namespace godot;

////////////////////////////////////////////////////////////////////////////////
// Extension functions

PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL xrCreateHandTrackerEXT(
		XrSession session,
		const XrHandTrackerCreateInfoEXT *createInfo,
		XrHandTrackerEXT *handTracker) {
	if (xrCreateHandTrackerEXT_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrCreateHandTrackerEXT_ptr)(session, createInfo, handTracker);
}

PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyHandTrackerEXT(
		XrHandTrackerEXT handTracker) {
	if (xrDestroyHandTrackerEXT_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrDestroyHandTrackerEXT_ptr)(handTracker);
}

PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL xrLocateHandJointsEXT(
		XrHandTrackerEXT handTracker,
		const XrHandJointsLocateInfoEXT *locateInfo,
		XrHandJointLocationsEXT *locations) {
	if (xrLocateHandJointsEXT_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrLocateHandJointsEXT_ptr)(handTracker, locateInfo, locations);
}

////////////////////////////////////////////////////////////////////////////////
// Default action set configuration

// TODO: it makes sense to include this in source because we'll store any user defined version in Godot scenes
// but there has to be a nicer way to embed it :)

const char *OpenXRApi::default_action_sets_json = "[\n"
												  "	{\n"
												  "		\"name\": \"godot\",\n"
												  "		\"localised_name\": \"Action Set Used by Godot\",\n"
												  "		\"priority\": 0,\n"
												  "		\"actions\": [\n"
												  "			{\n"
												  "				\"type\": \"pose\",\n"
												  "				\"name\": \"aim_pose\",\n"
												  "				\"localised_name\": \"Aim Pose\",\n"
												  "				\"paths\": [\n"
												  "					\"/user/hand/left\",\n"
												  "					\"/user/hand/right\",\n"
												  "				],\n"
												  "			},\n"
												  "			{\n"
												  "				\"type\": \"pose\",\n"
												  "				\"name\": \"grip_pose\",\n"
												  "				\"localised_name\": \"Grip Pose\",\n"
												  "				\"paths\": [\n"
												  "					\"/user/hand/left\",\n"
												  "					\"/user/hand/right\",\n"
												  "				],\n"
												  "			},\n"
												  "			{\n"
												  "				\"type\": \"float\",\n"
												  "				\"name\": \"front_trigger\",\n"
												  "				\"localised_name\": \"Front trigger\",\n"
												  "				\"paths\": [\n"
												  "					\"/user/hand/left\",\n"
												  "					\"/user/hand/right\",\n"
												  "				],\n"
												  "			},\n"
												  "			{\n"
												  "				\"type\": \"float\",\n"
												  "				\"name\": \"side_trigger\",\n"
												  "				\"localised_name\": \"Side trigger\",\n"
												  "				\"paths\": [\n"
												  "					\"/user/hand/left\",\n"
												  "					\"/user/hand/right\",\n"
												  "				],\n"
												  "			},\n"
												  "			{\n"
												  "				\"type\": \"vector2\",\n"
												  "				\"name\": \"joystick\",\n"
												  "				\"localised_name\": \"Joystick\",\n"
												  "				\"paths\": [\n"
												  "					\"/user/hand/left\",\n"
												  "					\"/user/hand/right\",\n"
												  "				],\n"
												  "			},\n"
												  "			{\n"
												  "				\"type\": \"bool\",\n"
												  "				\"name\": \"ax_buttons\",\n"
												  "				\"localised_name\": \"A and X buttons\",\n"
												  "				\"paths\": [\n"
												  "					\"/user/hand/left\",\n"
												  "					\"/user/hand/right\",\n"
												  "				],\n"
												  "			},\n"
												  "			{\n"
												  "				\"type\": \"bool\",\n"
												  "				\"name\": \"bym_button\",\n"
												  "				\"localised_name\": \"B, Y and menu buttons\",\n"
												  "				\"paths\": [\n"
												  "					\"/user/hand/left\",\n"
												  "					\"/user/hand/right\",\n"
												  "				],\n"
												  "			},\n"
												  "			{\n"
												  "				\"type\": \"bool\",\n"
												  "				\"name\": \"front_button\",\n"
												  "				\"localised_name\": \"Front trigger as a button\",\n"
												  "				\"paths\": [\n"
												  "					\"/user/hand/left\",\n"
												  "					\"/user/hand/right\",\n"
												  "				],\n"
												  "			},\n"
												  "			{\n"
												  "				\"type\": \"bool\",\n"
												  "				\"name\": \"side_button\",\n"
												  "				\"localised_name\": \"Side trigger as a button\",\n"
												  "				\"paths\": [\n"
												  "					\"/user/hand/left\",\n"
												  "					\"/user/hand/right\",\n"
												  "				],\n"
												  "			},\n"
												  "			{\n"
												  "				\"type\": \"bool\",\n"
												  "				\"name\": \"joystick_button\",\n"
												  "				\"localised_name\": \"Thumbstick click\",\n"
												  "				\"paths\": [\n"
												  "					\"/user/hand/left\",\n"
												  "					\"/user/hand/right\",\n"
												  "				],\n"
												  "			},\n"
												  "			{\n"
												  "				\"type\": \"vibration\",\n"
												  "				\"name\": \"haptic\",\n"
												  "				\"localised_name\": \"Controller haptic vibration\",\n"
												  "				\"paths\": [\n"
												  "					\"/user/hand/left\",\n"
												  "					\"/user/hand/right\",\n"
												  "				],\n"
												  "			},\n"
												  "		],\n"
												  "	}\n"
												  "]\n";

// documentated interaction profiles can be found here: https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#semantic-path-interaction-profiles
const char *OpenXRApi::default_interaction_profiles_json = "[\n"
														   "	{\n"
														   "		\"path\": \"/interaction_profiles/khr/simple_controller\",\n"
														   "		\"bindings\": [\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"aim_pose\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/aim/pose\",\n"
														   "					\"/user/hand/right/input/aim/pose\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"grip_pose\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/grip/pose\",\n"
														   "					\"/user/hand/right/input/grip/pose\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"haptic\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/output/haptic\",\n"
														   "					\"/user/hand/right/output/haptic\"\n"
														   "				]\n"
														   "			},\n"
														   "		],\n"
														   "	},\n"
														   "	{\n"
														   "		\"path\": \"/interaction_profiles/microsoft/motion_controller\",\n"
														   "		\"bindings\": [\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"aim_pose\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/aim/pose\",\n"
														   "					\"/user/hand/right/input/aim/pose\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"grip_pose\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/grip/pose\",\n"
														   "					\"/user/hand/right/input/grip/pose\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"front_trigger\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/trigger/value\",\n"
														   "					\"/user/hand/right/input/trigger/value\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"side_trigger\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/squeeze/click\",\n"
														   "					\"/user/hand/right/input/squeeze/click\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"joystick\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/thumbstick\",\n"
														   "					\"/user/hand/right/input/thumbstick\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"front_button\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/trigger/value\",\n"
														   "					\"/user/hand/right/input/trigger/value\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"side_button\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/squeeze/click\",\n"
														   "					\"/user/hand/right/input/squeeze/click\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"joystick_button\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/thumbstick/click\",\n"
														   "					\"/user/hand/right/input/thumbstick/click\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"haptic\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/output/haptic\",\n"
														   "					\"/user/hand/right/output/haptic\"\n"
														   "				]\n"
														   "			},\n"
														   "		],\n"
														   "	},\n"
														   "	{\n"
														   "		\"path\": \"/interaction_profiles/oculus/touch_controller\",\n"
														   "		\"bindings\": [\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"aim_pose\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/aim/pose\",\n"
														   "					\"/user/hand/right/input/aim/pose\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"grip_pose\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/grip/pose\",\n"
														   "					\"/user/hand/right/input/grip/pose\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"front_trigger\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/trigger/value\",\n"
														   "					\"/user/hand/right/input/trigger/value\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"side_trigger\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/squeeze/value\",\n"
														   "					\"/user/hand/right/input/squeeze/value\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"joystick\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/thumbstick\",\n"
														   "					\"/user/hand/right/input/thumbstick\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"ax_buttons\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/x/click\",\n"
														   "					\"/user/hand/right/input/a/click\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"bym_button\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/y/click\",\n"
														   "					\"/user/hand/right/input/b/click\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"front_button\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/trigger/value\",\n"
														   "					\"/user/hand/right/input/trigger/value\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"side_button\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/squeeze/value\",\n"
														   "					\"/user/hand/right/input/squeeze/value\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"joystick_button\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/thumbstick/click\",\n"
														   "					\"/user/hand/right/input/thumbstick/click\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"haptic\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/output/haptic\",\n"
														   "					\"/user/hand/right/output/haptic\"\n"
														   "				]\n"
														   "			},\n"
														   "		],\n"
														   "	},\n"
														   "	{\n"
														   "		\"path\": \"/interaction_profiles/valve/index_controller\",\n"
														   "		\"bindings\": [\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"aim_pose\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/aim/pose\",\n"
														   "					\"/user/hand/right/input/aim/pose\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"grip_pose\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/grip/pose\",\n"
														   "					\"/user/hand/right/input/grip/pose\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"front_trigger\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/trigger/value\",\n"
														   "					\"/user/hand/right/input/trigger/value\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"side_trigger\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/squeeze/value\",\n"
														   "					\"/user/hand/right/input/squeeze/value\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"joystick\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/thumbstick\",\n"
														   "					\"/user/hand/right/input/thumbstick\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"ax_buttons\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/a/click\",\n"
														   "					\"/user/hand/right/input/a/click\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"bym_button\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/b/click\",\n"
														   "					\"/user/hand/right/input/b/click\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"front_button\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/trigger/click\",\n"
														   "					\"/user/hand/right/input/trigger/click\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"side_button\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/squeeze/value\",\n"
														   "					\"/user/hand/right/input/squeeze/value\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"joystick_button\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/input/thumbstick/click\",\n"
														   "					\"/user/hand/right/input/thumbstick/click\"\n"
														   "				]\n"
														   "			},\n"
														   "			{\n"
														   "				\"set\": \"godot\",\n"
														   "				\"action\": \"haptic\",\n"
														   "				\"paths\": [\n"
														   "					\"/user/hand/left/output/haptic\",\n"
														   "					\"/user/hand/right/output/haptic\"\n"
														   "				]\n"
														   "			},\n"
														   "		],\n"
														   "	},\n"
														   "]\n";

////////////////////////////////////////////////////////////////////////////////
// Session states
const char *session_states[] = {
	"XR_SESSION_STATE_UNKNOWN",
	"XR_SESSION_STATE_IDLE",
	"XR_SESSION_STATE_READY",
	"XR_SESSION_STATE_SYNCHRONIZED",
	"XR_SESSION_STATE_VISIBLE",
	"XR_SESSION_STATE_FOCUSED",
	"XR_SESSION_STATE_STOPPING",
	"XR_SESSION_STATE_LOSS_PENDING",
	"XR_SESSION_STATE_EXITING",
};

////////////////////////////////////////////////////////////////////////////////
// Singleton management

OpenXRApi *OpenXRApi::singleton = nullptr;

void OpenXRApi::openxr_release_api() {
	if (singleton == nullptr) {
		// nothing to release
#ifdef DEBUG
		Godot::print("OpenXR: tried to release non-existent OpenXR context\n");
#endif
	} else if (singleton->use_count > 1) {
		// decrease use count
		singleton->use_count--;

#ifdef DEBUG
		Godot::print("OpenXR: decreased use count to {0}", singleton->use_count);
#endif
	} else {
		// cleanup openxr
#ifdef DEBUG
		Godot::print("OpenXR releasing OpenXR context");
#endif

		delete singleton;
		singleton = nullptr;
	};
};

OpenXRApi *OpenXRApi::openxr_get_api() {
	if (singleton != nullptr) {
		// increase use count
		singleton->use_count++;

#ifdef DEBUG
		Godot::print("OpenXR increased use count to {0}", singleton->use_count);
#endif
	} else {
		singleton = new OpenXRApi();
		if (singleton == nullptr) {
			Godot::print_error("OpenXR interface creation failed", __FUNCTION__, __FILE__, __LINE__);
#ifdef DEBUG
		} else {
			Godot::print("OpenXR interface creation successful");
#endif
		}
	}

	return singleton;
};

////////////////////////////////////////////////////////////////////////////////
// OpenXRApi

bool OpenXRApi::isExtensionSupported(const char *extensionName, XrExtensionProperties *instanceExtensionProperties, uint32_t instanceExtensionCount) {
	for (uint32_t supportedIndex = 0; supportedIndex < instanceExtensionCount; supportedIndex++) {
		if (!strcmp(extensionName, instanceExtensionProperties[supportedIndex].extensionName)) {
			return true;
		}
	}
	return false;
}

bool OpenXRApi::isViewConfigSupported(XrViewConfigurationType type, XrSystemId systemId) {
	XrResult result;
	uint32_t viewConfigurationCount;

	result = xrEnumerateViewConfigurations(instance, systemId, 0, &viewConfigurationCount, nullptr);
	if (!xr_result(result, "Failed to get view configuration count")) {
		return false;
	}

	// Damn you microsoft for not supporting this!!
	// XrViewConfigurationType viewConfigurations[viewConfigurationCount];
	XrViewConfigurationType *viewConfigurations = (XrViewConfigurationType *)malloc(sizeof(XrViewConfigurationType) * viewConfigurationCount);
	if (viewConfigurations == nullptr) {
		Godot::print_error("Couldn't allocate memory for view configurations", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	result = xrEnumerateViewConfigurations(instance, systemId, viewConfigurationCount, &viewConfigurationCount, viewConfigurations);
	if (!xr_result(result, "Failed to enumerate view configurations!")) {
		free(viewConfigurations);
		return false;
	}

	for (uint32_t i = 0; i < viewConfigurationCount; ++i) {
		if (viewConfigurations[i] == type) {
			free(viewConfigurations);
			return true;
		}
	}

	free(viewConfigurations);
	return false;
}

bool OpenXRApi::isReferenceSpaceSupported(XrReferenceSpaceType type) {
	XrResult result;
	uint32_t referenceSpacesCount;

	result = xrEnumerateReferenceSpaces(session, 0, &referenceSpacesCount, nullptr);
	if (!xr_result(result, "Getting number of reference spaces failed!")) {
		return false;
	}

	// Damn you microsoft for not supporting this!!
	// XrReferenceSpaceType referenceSpaces[referenceSpacesCount];
	XrReferenceSpaceType *referenceSpaces = (XrReferenceSpaceType *)malloc(sizeof(XrReferenceSpaceType) * referenceSpacesCount);
	if (referenceSpaces == nullptr) {
		Godot::print_error("OpenXR Couldn't allocate memory for reference spaces", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	result = xrEnumerateReferenceSpaces(session, referenceSpacesCount, &referenceSpacesCount, referenceSpaces);
	if (!xr_result(result, "Enumerating reference spaces failed!")) {
		free(referenceSpaces);
		return false;
	}

	for (uint32_t i = 0; i < referenceSpacesCount; i++) {
		if (referenceSpaces[i] == type) {
			free(referenceSpaces);
			return true;
		}
	}

	free(referenceSpaces);
	return false;
}

bool OpenXRApi::initialiseInstance() {
	XrResult result;

#ifdef DEBUG
	Godot::print("OpenXR initialiseInstance");
#endif

#ifdef ANDROID
	// Initialize the loader
	PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
	result = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction *)(&xrInitializeLoaderKHR));
	if (!xr_result(result, "Failed to retrieve pointer to xrInitializeLoaderKHR")) {
		return false;
	}

	JNIEnv *env = android_api->godot_android_get_env();
	JavaVM *vm;
	env->GetJavaVM(&vm);
	jobject activity_object = env->NewGlobalRef(android_api->godot_android_get_activity());

	XrLoaderInitInfoAndroidKHR loader_init_info_android = {
		.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR,
		.next = nullptr,
		.applicationVM = vm,
		.applicationContext = activity_object
	};
	xrInitializeLoaderKHR((const XrLoaderInitInfoBaseHeaderKHR *)&loader_init_info_android);
#endif

	// Log available layers.
	PFN_xrEnumerateApiLayerProperties xrEnumerateApiLayerProperties;
	result = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrEnumerateApiLayerProperties", (PFN_xrVoidFunction *)&xrEnumerateApiLayerProperties);
	if (!xr_result(result, "Failed to retrieve pointer to xrEnumerateApiLayerProperties")) {
		return false;
	}

	uint32_t num_input_layers = 0;
	uint32_t num_output_layers = 0;
	result = xrEnumerateApiLayerProperties(num_input_layers, &num_output_layers, nullptr);
	if (!xr_result(result, "Failed to enumerate number of api layer properties")) {
		return false;
	}

	num_input_layers = num_output_layers;
	XrApiLayerProperties *layer_properties = (XrApiLayerProperties *)malloc(sizeof(XrApiLayerProperties) * num_output_layers);
	if (layer_properties == nullptr) {
		Godot::print_error("Couldn't allocate memory for api layer properties", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	for (uint32_t i = 0; i < num_output_layers; i++) {
		layer_properties[i].type = XR_TYPE_API_LAYER_PROPERTIES;
		layer_properties[i].next = nullptr;
	}

	result = xrEnumerateApiLayerProperties(num_input_layers, &num_output_layers, layer_properties);
	if (!xr_result(result, "Failed to enumerate api layer properties")) {
		free(layer_properties);
		return false;
	}

	for (uint32_t i = 0; i < num_output_layers; i++) {
		Godot::print("Found layer {0}", layer_properties[i].layerName);
	}

	free(layer_properties);

	uint32_t extensionCount = 0;
	result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);

	/* TODO: instance null will not be able to convert XrResult to string */
	if (!xr_result(result, "Failed to enumerate number of extension properties")) {
		return false;
	}

	// Damn you microsoft for not supporting this!!
	// XrExtensionProperties extensionProperties[extensionCount];
	XrExtensionProperties *extensionProperties = (XrExtensionProperties *)malloc(sizeof(XrExtensionProperties) * extensionCount);
	if (extensionProperties == nullptr) {
		Godot::print_error("OpenXR Couldn't allocate memory for extension properties", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
	for (uint32_t i = 0; i < extensionCount; i++) {
		extensionProperties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
		extensionProperties[i].next = nullptr;
	}

	result = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties);
	if (!xr_result(result, "Failed to enumerate extension properties")) {
		free(extensionProperties);
		return false;
	}

	// Map the extensions we need. Set bool pointer to nullptr for mandatory extensions
	std::map<const char *, bool *> request_extensions;

#ifdef ANDROID
	request_extensions[XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME] = nullptr;
	request_extensions[XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME] = nullptr;
	request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_OPENGL_ES_EXTENSION_NAME] = &fb_swapchain_update_state_opengles_ext;
#else
	request_extensions[XR_KHR_OPENGL_ENABLE_EXTENSION_NAME] = nullptr;
#endif

	// If we have these, we use them, if not we skip related logic..
	request_extensions[XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME] = &performance_settings_ext;
	request_extensions[XR_EXT_HAND_TRACKING_EXTENSION_NAME] = &hand_tracking_ext;
	request_extensions[XR_EXT_HAND_JOINTS_MOTION_RANGE_EXTENSION_NAME] = &hand_motion_range_ext;
	request_extensions[XR_MND_BALL_ON_STICK_EXTENSION_NAME] = &monado_stick_on_ball_ext;

	// These might be FB extensions but other vendors may implement them in due time as well.
	request_extensions[XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME] = &fb_display_refresh_rate_ext;
	request_extensions[XR_FB_COLOR_SPACE_EXTENSION_NAME] = &fb_color_space_ext;
	request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME] = &fb_swapchain_update_state_ext;
	request_extensions[XR_FB_FOVEATION_EXTENSION_NAME] = &fb_foveation_ext;
	request_extensions[XR_FB_FOVEATION_CONFIGURATION_EXTENSION_NAME] = &fb_foveation_configuration_ext;

	for (auto &requested_extension : request_extensions) {
		if (!isExtensionSupported(requested_extension.first, extensionProperties, extensionCount)) {
			if (requested_extension.second == nullptr) {
				Godot::print_error("OpenXR Runtime does not support OpenGL extension!", __FUNCTION__, __FILE__, __LINE__);
				free(extensionProperties);
				return false;
			} else {
				*requested_extension.second = false;
			}
		} else if (requested_extension.second != nullptr) {
			*requested_extension.second = true;
		}
	}

	free(extensionProperties);

	enabled_extensions.clear();
	for (auto &requested_extension : request_extensions) {
		if (requested_extension.second == nullptr) {
			// required extension, if we got this far, include it
			enabled_extensions.push_back(requested_extension.first);
		} else if (*requested_extension.second) {
			// supported optional extension, include it
			enabled_extensions.push_back(requested_extension.first);
		}
	}

// https://stackoverflow.com/a/55926503
#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#define __GCC__
#endif

	// Microsoft wants fields in definition to be in order or it will have a hissy fit!
	XrInstanceCreateInfo instanceCreateInfo = {
		.type = XR_TYPE_INSTANCE_CREATE_INFO,
		.next = nullptr,
		.createFlags = 0,
		.applicationInfo = {
#ifdef __GCC__ // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55227
				{ .applicationName = "Godot OpenXR Plugin" },
#else
				.applicationName = "Godot OpenXR Plugin",
#endif
				.applicationVersion = 1, // We don't maintain a version for the application at this time.
#ifdef __GCC__ // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55227
				{ .engineName = "Godot Game Engine" },
#else
				.engineName = "Godot Game Engine",
#endif
				.engineVersion = 3, // TODO: establish godot version -> uint32_t versioning
				.apiVersion = XR_CURRENT_API_VERSION,
		},
		.enabledApiLayerCount = 0,
		.enabledApiLayerNames = nullptr,
		.enabledExtensionCount = (uint32_t)enabled_extensions.size(),
		.enabledExtensionNames = enabled_extensions.data(),
	};

	// Check if we can get a project name from our project...
	String project_name = ProjectSettings::get_singleton()->get_setting("application/config/name");
	if (project_name.length() > 0) {
		CharString name_cs = project_name.utf8();
		int32_t len = name_cs.length();
		if (len < XR_MAX_APPLICATION_NAME_SIZE - 1) {
			strcpy(instanceCreateInfo.applicationInfo.applicationName, name_cs.get_data());
		} else {
			memcpy(instanceCreateInfo.applicationInfo.applicationName, name_cs.get_data(), XR_MAX_APPLICATION_NAME_SIZE - 1);
			instanceCreateInfo.applicationInfo.applicationName[XR_MAX_APPLICATION_NAME_SIZE - 1] = '\0';
		}
	}

	result = xrCreateInstance(&instanceCreateInfo, &instance);
	if (!xr_result(result, "Failed to create XR instance.")) {
		return false;
	}

	XrInstanceProperties instanceProps = {
		.type = XR_TYPE_INSTANCE_PROPERTIES,
		.next = nullptr
	};
	result = xrGetInstanceProperties(instance, &instanceProps);
	if (!xr_result(result, "Failed to get XR instance properties.")) {
		// not fatal probably
		return true;
	}

	Godot::print("Running on OpenXR runtime: {0} {1}.{2}.{3}",
			instanceProps.runtimeName,
			XR_VERSION_MAJOR(instanceProps.runtimeVersion),
			XR_VERSION_MINOR(instanceProps.runtimeVersion),
			XR_VERSION_PATCH(instanceProps.runtimeVersion));

	if (strcmp(instanceProps.runtimeName, "SteamVR/OpenXR") == 0) {
#ifdef WIN32
		// not applicable
#elif ANDROID
		// not applicable
#elif __linux__
		Godot::print("Running on Linux, using SteamVR workaround for issue https://github.com/ValveSoftware/SteamVR-for-Linux/issues/421");
#endif
		is_steamvr = true;
	}

	return true;
}

bool OpenXRApi::initialiseExtensions() {
	XrResult result;

	// Maybe we should remove the error checking here, if the extension is not supported, we won't be doing anything with this.

#ifdef DEBUG
	Godot::print("OpenXR initialiseExtensions");
#endif

	if (hand_tracking_ext) {
		// TODO move this into hand tracker source
		result = xrGetInstanceProcAddr(instance, "xrCreateHandTrackerEXT", (PFN_xrVoidFunction *)&xrCreateHandTrackerEXT_ptr);
		if (!xr_result(result, "Failed to obtain xrCreateHandTrackerEXT function pointer")) {
			return false;
		}

		result = xrGetInstanceProcAddr(instance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction *)&xrDestroyHandTrackerEXT_ptr);
		if (!xr_result(result, "Failed to obtain xrDestroyHandTrackerEXT function pointer")) {
			return false;
		}

		result = xrGetInstanceProcAddr(instance, "xrLocateHandJointsEXT", (PFN_xrVoidFunction *)&xrLocateHandJointsEXT_ptr);
		if (!xr_result(result, "Failed to obtain xrLocateHandJointsEXT function pointer")) {
			return false;
		}
	}

	return true;
}

bool OpenXRApi::initialiseSession() {
	XrResult result;

#ifdef DEBUG
	Godot::print("OpenXR initialiseSession");
#endif

	// TODO: Support AR?
	XrSystemGetInfo systemGetInfo = {
		.type = XR_TYPE_SYSTEM_GET_INFO,
		.next = nullptr,
		.formFactor = form_factor,
	};

	result = xrGetSystem(instance, &systemGetInfo, &systemId);
	if (!xr_result(result, "Failed to get system for our form factor.")) {
		return false;
	}

	// always define, we ignore this if it's not used.
	XrSystemColorSpacePropertiesFB color_space_properties_fb = {
		.type = XR_TYPE_SYSTEM_COLOR_SPACE_PROPERTIES_FB,
	};

	XrSystemProperties systemProperties = {
		.type = XR_TYPE_SYSTEM_PROPERTIES,
		.next = nullptr,
		.graphicsProperties = { 0 },
		.trackingProperties = { 0 },
	};

	if (fb_color_space_ext) {
		// if our color space extension is availale, read our color space.
		systemProperties.next = &color_space_properties_fb;
	}

	result = xrGetSystemProperties(instance, systemId, &systemProperties);
	if (!xr_result(result, "Failed to get System properties")) {
		return false;
	}

	if (fb_color_space_ext) {
		// TODO color_space_properties_fb.colorSpace should now contain our current color space, store it somewhere...
	}

	if (!isViewConfigSupported(view_config_type, systemId)) {
		// TODO in stead of erroring out if the set configuration type is unsupported
		// (it may simply be on its default setting)
		// we should change this so it uses the first support type.
		// That does mean checking if WE support it (i.e. we don't support Varjo yet for instance).
		Godot::print_error("OpenXR View Configuration not supported!", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	result = xrEnumerateViewConfigurationViews(instance, systemId, view_config_type, 0, &view_count, nullptr);
	if (!xr_result(result, "Failed to get view configuration view count!")) {
		return false;
	}

	configuration_views = (XrViewConfigurationView *)malloc(sizeof(XrViewConfigurationView) * view_count);
	for (uint32_t i = 0; i < view_count; i++) {
		configuration_views[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
		configuration_views[i].next = nullptr;
	}

	result = xrEnumerateViewConfigurationViews(instance, systemId, view_config_type, view_count, &view_count, configuration_views);
	if (!xr_result(result, "Failed to enumerate view configuration views!")) {
		return false;
	}

	buffer_index = (uint32_t *)malloc(sizeof(uint32_t) * view_count);

	if (!check_graphics_requirements_gl(systemId)) {
		return false;
	}

	OS *os = OS::get_singleton();

	// TODO: support wayland
	// TODO: maybe support xcb separately?
	// TODO: support vulkan

#ifdef WIN32
	graphics_binding_gl = XrGraphicsBindingOpenGLWin32KHR{
		.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
		.next = NULL,
	};

	graphics_binding_gl.hDC = (HDC)os->get_native_handle(OS::WINDOW_VIEW);
	graphics_binding_gl.hGLRC = (HGLRC)os->get_native_handle(OS::OPENGL_CONTEXT);

	if ((graphics_binding_gl.hDC == 0) || (graphics_binding_gl.hGLRC == 0)) {
		Godot::print_error("OpenXR Windows native handle API is missing, please use a newer version of Godot!", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
#elif ANDROID
	graphics_binding_gl = XrGraphicsBindingOpenGLESAndroidKHR{
		.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
		.next = NULL,
	};

	graphics_binding_gl.display = eglGetCurrentDisplay();
	graphics_binding_gl.config = (EGLConfig)0; // https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/master/src/tests/hello_xr/graphicsplugin_opengles.cpp#L122
	graphics_binding_gl.context = eglGetCurrentContext();
#else
	graphics_binding_gl = (XrGraphicsBindingOpenGLXlibKHR){
		.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR,
		.next = NULL,
	};

	void *display_handle = (void *)os->get_native_handle(OS::DISPLAY_HANDLE);
	void *glxcontext_handle = (void *)os->get_native_handle(OS::OPENGL_CONTEXT);
	void *glxdrawable_handle = (void *)os->get_native_handle(OS::WINDOW_HANDLE);

	graphics_binding_gl.xDisplay = (Display *)display_handle;
	graphics_binding_gl.glxContext = (GLXContext)glxcontext_handle;
	graphics_binding_gl.glxDrawable = (GLXDrawable)glxdrawable_handle;

	if (graphics_binding_gl.xDisplay == NULL) {
		Godot::print("OpenXR Failed to get xDisplay from Godot, using XOpenDisplay(NULL)");
		graphics_binding_gl.xDisplay = XOpenDisplay(NULL);
	}
	if (graphics_binding_gl.glxContext == NULL) {
		Godot::print("OpenXR Failed to get glxContext from Godot, using glXGetCurrentContext()");
		graphics_binding_gl.glxContext = glXGetCurrentContext();
	}
	if (graphics_binding_gl.glxDrawable == 0) {
		Godot::print("OpenXR Failed to get glxDrawable from Godot, using glXGetCurrentDrawable()");
		graphics_binding_gl.glxDrawable = glXGetCurrentDrawable();
	}

	// spec says to use proper values but runtimes don't care
	graphics_binding_gl.visualid = 0;
	graphics_binding_gl.glxFBConfig = 0;

	Godot::print("OpenXR Graphics: Display %p, Context %" PRIxPTR ", Drawable %" PRIxPTR,
			graphics_binding_gl.xDisplay,
			(uintptr_t)graphics_binding_gl.glxContext,
			(uintptr_t)graphics_binding_gl.glxDrawable);
#endif

	Godot::print("OpenXR Using OpenGL version: {0}", (char *)glGetString(GL_VERSION));
	Godot::print("OpenXR Using OpenGL renderer: {0}", (char *)glGetString(GL_RENDERER));

	XrSessionCreateInfo session_create_info = {
		.type = XR_TYPE_SESSION_CREATE_INFO,
		.next = &graphics_binding_gl,
		.createFlags = 0,
		.systemId = systemId
	};

	result = xrCreateSession(instance, &session_create_info, &session);
	if (!xr_result(result, "Failed to create session")) {
		return false;
	}

	return true;
}

bool OpenXRApi::initialiseSpaces() {
	XrResult result;

#ifdef DEBUG
	Godot::print("OpenXR initialiseSpaces");
#endif

	XrPosef identityPose = {
		.orientation = { .x = 0, .y = 0, .z = 0, .w = 1.0 },
		.position = { .x = 0, .y = 0, .z = 0 }
	};

	{
		// most runtimes will support local and stage
		if (!isReferenceSpaceSupported(play_space_type)) {
			Godot::print("OpenXR runtime does not support play space type {0}!", play_space_type);
			return false;
		}

		XrReferenceSpaceCreateInfo localSpaceCreateInfo = {
			.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
			.next = nullptr,
			.referenceSpaceType = play_space_type,
			.poseInReferenceSpace = identityPose
		};

		result = xrCreateReferenceSpace(session, &localSpaceCreateInfo, &play_space);
		if (!xr_result(result, "Failed to create local space!")) {
			return false;
		}
	}

	{
		// all runtimes should support this
		if (!isReferenceSpaceSupported(XR_REFERENCE_SPACE_TYPE_VIEW)) {
			Godot::print_error("OpenXR runtime does not support view space!", __FUNCTION__, __FILE__, __LINE__);
			return false;
		}

		XrReferenceSpaceCreateInfo view_space_create_info = {
			.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
			.next = nullptr,
			.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW,
			.poseInReferenceSpace = identityPose
		};

		result = xrCreateReferenceSpace(session, &view_space_create_info, &view_space);
		if (!xr_result(result, "Failed to create local space!")) {
			return false;
		}
	}

	return true;
}

void OpenXRApi::cleanupSpaces() {
	// destroy our spaces
	if (play_space != XR_NULL_HANDLE) {
		xrDestroySpace(play_space);
		play_space = XR_NULL_HANDLE;
	}
	if (view_space != XR_NULL_HANDLE) {
		xrDestroySpace(view_space);
		view_space = XR_NULL_HANDLE;
	}
}

bool OpenXRApi::initialiseSwapChains() {
	XrResult result;

#ifdef DEBUG
	Godot::print("OpenXR initialiseSwapChains");
#endif

	uint32_t swapchainFormatCount;
	result = xrEnumerateSwapchainFormats(session, 0, &swapchainFormatCount, nullptr);
	if (!xr_result(result, "Failed to get number of supported swapchain formats")) {
		return false;
	}

	// Damn you microsoft for not supporting this!!
	// int64_t swapchainFormats[swapchainFormatCount];
	int64_t *swapchainFormats = (int64_t *)malloc(sizeof(int64_t) * swapchainFormatCount);
	if (swapchainFormats == nullptr) {
		Godot::print_error("OpenXR Couldn't allocate memory for swap chain formats", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	result = xrEnumerateSwapchainFormats(session, swapchainFormatCount, &swapchainFormatCount, swapchainFormats);
	if (!xr_result(result, "Failed to enumerate swapchain formats")) {
		free(swapchainFormats);
		return false;
	}

	// int64_t swapchainFormatToUse = swapchainFormats[0];
	int64_t swapchainFormatToUse = 0;

	// With the GLES2 driver we're rendering directly into this buffer with a pipeline that assumes an RGBA8 buffer.
	// With the GLES3 driver rendering happens into an RGBA16F buffer with all rendering happening in linear color space.
	// This buffer is then copied into the texture we supply here during the post process stage where tone mapping, glow, DOF, screenspace reflection and conversion to sRGB is applied.
	// As such we should chose an RGBA8 buffer here (note that an SRGB variant would allow automatic Linear to SRGB conversion but not sure if that is actually used)

	// We grab the first applicable one we find, OpenXR sorts these from best to worst choice..

	keep_3d_linear = true; // This will only work correctly for GLES2 from Godot 3.4 onwards

	Godot::print("OpenXR Swapchain Formats");
	for (uint64_t i = 0; i < swapchainFormatCount && swapchainFormatToUse == 0; i++) {
		// Godot::print("Found {0}\n", swapchainFormats[i]);
#ifdef WIN32
		/* disabling SRGB for now, we're rendering in linear color space...
		if (swapchainFormats[i] == GL_SRGB8_ALPHA8 && video_driver == OS::VIDEO_DRIVER_GLES3) {
			swapchainFormatToUse = swapchainFormats[i];
			Godot::print("OpenXR Using SRGB swapchain.");
			keep_3d_linear = false; // no the hardware will do conversions so we can supply sRGB values
		}
		*/
		if (swapchainFormats[i] == GL_RGBA8) {
			swapchainFormatToUse = swapchainFormats[i];
			Godot::print("OpenXR Using RGBA swapchain.");
		}
#elif ANDROID
		/* disabling SRGB for now, we're rendering in linear color space...
		if (swapchainFormats[i] == GL_SRGB8_ALPHA8 && video_driver == OS::VIDEO_DRIVER_GLES3) {
			swapchainFormatToUse = swapchainFormats[i];
			Godot::print("OpenXR Using SRGB swapchain.");
			keep_3d_linear = false; // no the hardware will do conversions so we can supply sRGB values
		}
		*/
		if (swapchainFormats[i] == GL_RGBA8) {
			swapchainFormatToUse = swapchainFormats[i];
			Godot::print("OpenXR Using RGBA swapchain.");
		}
#else
		/* disabling SRGB for now, we're rendering in linear color space...
		if (swapchainFormats[i] == GL_SRGB8_ALPHA8_EXT && video_driver == OS::VIDEO_DRIVER_GLES3) {
			swapchainFormatToUse = swapchainFormats[i];
			Godot::print("OpenXR Using SRGB swapchain.");
			keep_3d_linear = false; // no the hardware will do conversions so we can supply sRGB values
		}
		*/
		if (swapchainFormats[i] == GL_RGBA8_EXT) {
			swapchainFormatToUse = swapchainFormats[i];
			Godot::print("OpenXR Using RGBA swapchain.");
		}
#endif
	}

	// Couldn't find any we want? use the first one.
	// If this is a RGBA16F texture OpenXR on Steam atleast expects linear color space and we'll end up with a too bright display
	if (swapchainFormatToUse == 0) {
		swapchainFormatToUse = swapchainFormats[0];
		Godot::print("OpenXR Couldn't find prefered swapchain format, using {0}", swapchainFormatToUse);
	}

	free(swapchainFormats);

	swapchains = (XrSwapchain *)malloc(sizeof(XrSwapchain) * view_count);
	if (swapchains == nullptr) {
		Godot::print_error("OpenXR Couldn't allocate memory for swap chains", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	// Damn you microsoft for not supporting this!!
	// uint32_t swapchainLength[view_count];
	uint32_t *swapchainLength = (uint32_t *)malloc(sizeof(uint32_t) * view_count);
	if (swapchainLength == nullptr) {
		Godot::print_error("OpenXR Couldn't allocate memory for swap chain lengths", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	for (uint32_t i = 0; i < view_count; i++) {
		// again Microsoft wants these in order!
		XrSwapchainCreateInfo swapchainCreateInfo = {
			.type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
			.next = nullptr,
			.createFlags = 0,
			.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
			.format = swapchainFormatToUse,
			.sampleCount = configuration_views->recommendedSwapchainSampleCount, // 1,
			.width = configuration_views[i].recommendedImageRectWidth,
			.height = configuration_views[i].recommendedImageRectHeight,
			.faceCount = 1,
			.arraySize = 1,
			.mipCount = 1,
		};

		// Enable foveation on this swapchain
		XrSwapchainCreateInfoFoveationFB swapchain_create_info_foveation_fb = {
			.type = XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB
		};
		if (fb_swapchain_update_state_ext) {
			swapchainCreateInfo.next = &swapchain_create_info_foveation_fb;
		}

		result = xrCreateSwapchain(session, &swapchainCreateInfo, &swapchains[i]);
		if (!xr_result(result, "Failed to create swapchain {0}!", i)) {
			free(swapchainLength);
			return false;
		}

		result = xrEnumerateSwapchainImages(swapchains[i], 0, &swapchainLength[i], nullptr);
		if (!xr_result(result, "Failed to enumerate swapchains")) {
			free(swapchainLength);
			return false;
		}
	}

#ifdef ANDROID
	images = (XrSwapchainImageOpenGLESKHR **)malloc(sizeof(XrSwapchainImageOpenGLESKHR **) * view_count);
#else
	images = (XrSwapchainImageOpenGLKHR **)malloc(sizeof(XrSwapchainImageOpenGLKHR **) * view_count);
#endif
	if (images == nullptr) {
		Godot::print_error("OpenXR Couldn't allocate memory for swap chain images", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	// reset so if we fail we don't try to free memory we never allocated
	for (uint32_t i = 0; i < view_count; i++) {
		images[i] = nullptr;
	}

	for (uint32_t i = 0; i < view_count; i++) {
#ifdef ANDROID
		images[i] = (XrSwapchainImageOpenGLESKHR *)malloc(sizeof(XrSwapchainImageOpenGLESKHR) * swapchainLength[i]);
#else
		images[i] = (XrSwapchainImageOpenGLKHR *)malloc(sizeof(XrSwapchainImageOpenGLKHR) * swapchainLength[i]);
#endif
		if (images[i] == nullptr) {
			Godot::print_error("OpenXR Couldn't allocate memory for swap chain image", __FUNCTION__, __FILE__, __LINE__);
			return false;
		}

		for (uint64_t j = 0; j < swapchainLength[i]; j++) {
#ifdef ANDROID
			images[i][j].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
#else
			images[i][j].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
#endif
			images[i][j].next = nullptr;
		}
	}

	for (uint32_t i = 0; i < view_count; i++) {
		result = xrEnumerateSwapchainImages(swapchains[i], swapchainLength[i], &swapchainLength[i], (XrSwapchainImageBaseHeader *)images[i]);
		if (!xr_result(result, "Failed to enumerate swapchain images")) {
			return false;
		}
	}

	free(swapchainLength);

	// only used for OpenGL depth testing
	/*
	glGenTextures(1, &depthbuffer);
	glBindTexture(GL_TEXTURE_2D, depthbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
		configuration_views[0].recommendedImageRectWidth,
		configuration_views[0].recommendedImageRectHeight, 0,
		GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
	*/

	projectionLayer = (XrCompositionLayerProjection *)malloc(sizeof(XrCompositionLayerProjection));
	projectionLayer->type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
	projectionLayer->next = NULL;
	projectionLayer->layerFlags = 0;
	projectionLayer->space = play_space;
	projectionLayer->viewCount = view_count;
	projectionLayer->views = NULL;

	frameState.type = XR_TYPE_FRAME_STATE;
	frameState.next = NULL;

	views = (XrView *)malloc(sizeof(XrView) * view_count);
	if (views == NULL) {
		Godot::print_error("OpenXR Couldn't allocate memory for views", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	projection_views = (XrCompositionLayerProjectionView *)malloc(sizeof(XrCompositionLayerProjectionView) * view_count);
	if (projection_views == NULL) {
		Godot::print_error("OpenXR Couldn't allocate memory for projection views", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	for (uint32_t i = 0; i < view_count; i++) {
		views[i].type = XR_TYPE_VIEW;
		views[i].next = NULL;

		projection_views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
		projection_views[i].next = NULL;
		projection_views[i].subImage.swapchain = swapchains[i];
		projection_views[i].subImage.imageArrayIndex = 0;
		projection_views[i].subImage.imageRect.offset.x = 0;
		projection_views[i].subImage.imageRect.offset.y = 0;
		projection_views[i].subImage.imageRect.extent.width = configuration_views[i].recommendedImageRectWidth;
		projection_views[i].subImage.imageRect.extent.height = configuration_views[i].recommendedImageRectHeight;
	};

	return true;
}

void OpenXRApi::cleanupSwapChains() {
	if (swapchains != NULL) {
		free(swapchains);
		swapchains = NULL;
	}
	if (projection_views != NULL) {
		free(projection_views);
		projection_views = NULL;
	}
	if (images != NULL) {
		for (uint32_t i = 0; i < view_count; i++) {
			free(images[i]);
		}
		free(images);
		images = NULL;
	}
	if (projectionLayer != NULL) {
		free(projectionLayer);
		projectionLayer = NULL;
	}
	if (views != NULL) {
		free(views);
		views = NULL;
	}
}

bool OpenXRApi::initialiseHandTracking() {
	XrResult result;

	if (!hand_tracking_ext) {
		return false;
	}

#ifdef DEBUG
	Godot::print("OpenXR initialiseHandTracking");
#endif

	XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties = {
		.type = XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT,
	};

	XrSystemProperties systemProperties = {
		.type = XR_TYPE_SYSTEM_PROPERTIES,
		.next = &handTrackingSystemProperties,
	};

	result = xrGetSystemProperties(instance, systemId, &systemProperties);
	if (!xr_result(result, "Failed to obtain hand tracking information")) {
		return false;
	}

	if (!handTrackingSystemProperties.supportsHandTracking) {
		// The system does not support hand tracking
		printf("Hand tracking is not supported\n");
		return false;
	}

	for (int i = 0; i < 2; i++) {
		// we'll do this later
		hand_trackers[i].is_initialised = false;
		hand_trackers[i].hand_tracker = XR_NULL_HANDLE;
	}

	printf("Hand tracking is supported\n");

	hand_tracking_supported = true;
	return true;
}

void OpenXRApi::cleanupHandTracking() {
	for (int i = 0; i < 2; i++) {
		if (hand_trackers[i].hand_tracker != XR_NULL_HANDLE) {
			xrDestroyHandTrackerEXT(hand_trackers[i].hand_tracker);

			hand_trackers[i].is_initialised = false;
			hand_trackers[i].hand_tracker = XR_NULL_HANDLE;
		}
	}
}

bool OpenXRApi::loadActionSets() {
#ifdef DEBUG
	Godot::print("OpenXR loadActionSets");
#endif

	parse_action_sets(action_sets_json);
	parse_interaction_profiles(interaction_profiles_json);

	return true;
}

bool OpenXRApi::bindActionSets() {
#ifdef DEBUG
	Godot::print("OpenXR bindActionSets");
#endif

	// finally attach our action sets, that locks everything in place
	for (uint64_t i = 0; i < action_sets.size(); i++) {
		ActionSet *action_set = action_sets[i];

		if (!action_set->attach()) {
			// Just report this
			Godot::print("Couldn't attach action set {0}", action_set->get_name());
		} else {
			Godot::print("Attached action set {0}", action_set->get_name());
		}
	}

	// NOTE: outputting what we find here for debugging, should probably make this silent in due time or just have one line with missing actions.
	// a developer that is not using the internal actions but defines their own may not care about these missing

	// Init our input paths and godot controllers for our mapping to
	for (uint64_t i = 0; i < USER_INPUT_MAX; i++) {
		XrResult res = xrStringToPath(instance, inputmaps[i].name, &inputmaps[i].toplevel_path);
		xr_result(res, "OpenXR Couldn't obtain path for {0}", inputmaps[i].name);
	}

	// find our default actions
	for (uint64_t i = 0; i < ACTION_MAX; i++) {
		default_actions[i].action = get_action(default_actions[i].name);
		if (default_actions[i].action != NULL) {
			Godot::print("OpenXR found internal action {0}", default_actions[i].name);
		} else {
			Godot::print("OpenXR didn't find internal action {0}", default_actions[i].name);
		}
	}

	return true;
}

void OpenXRApi::unbindActionSets() {
	// cleanup our controller mapping
	for (uint64_t i = 0; i < USER_INPUT_MAX; i++) {
		inputmaps[i].toplevel_path = XR_NULL_PATH;
		inputmaps[i].active_profile = XR_NULL_PATH;
		if (inputmaps[i].godot_controller >= 0) {
			arvr_api->godot_arvr_remove_controller(inputmaps[i].godot_controller);
			inputmaps[i].godot_controller = -1;
		}
	}

	// reset our default actions
	for (uint64_t i = 0; i < ACTION_MAX; i++) {
		default_actions[i].action = NULL;
	}

	// reset our spaces
	for (uint64_t i = 0; i < action_sets.size(); i++) {
		ActionSet *action_set = action_sets[i];

		action_set->reset_spaces();
	}
}

void OpenXRApi::cleanupActionSets() {
	unbindActionSets();

	// clear out our action sets
	while (!action_sets.empty()) {
		ActionSet *action_set = action_sets.back();
		delete action_set;

		action_sets.pop_back();
	}
}

OpenXRApi::OpenXRApi() {
	// We set this to true if we init everything correctly
	initialised = false;

	// set our defaults
	action_sets_json = default_action_sets_json;
	interaction_profiles_json = default_interaction_profiles_json;
}

bool OpenXRApi::initialize() {
	if (initialised) {
		// Already initialised, shouldn't be called in this case..
#ifdef DEBUG
		Godot::print("Initialize called when interface is already initialized.");
#endif
		return true;
	}

#ifdef WIN32
	if (!gladLoadGL()) {
		Godot::print_error("OpenXR Failed to initialize GLAD", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
#endif

	// get our video driver setting from Godot.
	OS *os = OS::get_singleton();
	video_driver = os->get_current_video_driver();

	if (!initialiseInstance()) {
		// cleanup and exit
		uninitialize();
		return false;
	}

	if (!initialiseExtensions()) {
		// cleanup and exit
		uninitialize();
		return false;
	}

	if (!initialiseSession()) {
		// cleanup and exit
		uninitialize();
		return false;
	}

	if (!loadActionSets()) {
		// cleanup and exit
		uninitialize();
		return false;
	}

	// We've made it!
	initialised = true;
	return true;
}

OpenXRApi::~OpenXRApi() {
	uninitialize();
}

void OpenXRApi::uninitialize() {
	if (running && session != XR_NULL_HANDLE) {
		xrEndSession(session);
		// we destroy this further down..
	}

	cleanupActionSets();
	cleanupHandTracking();
	cleanupSwapChains();
	cleanupSpaces();

	// cleanup our session and instance
	if (configuration_views) {
		free(configuration_views);
		configuration_views = NULL;
	}
	if (buffer_index != NULL) {
		free(buffer_index);
		buffer_index = NULL;
	}
	if (session != XR_NULL_HANDLE) {
		xrDestroySession(session);
		session = XR_NULL_HANDLE;
	}
	if (instance != XR_NULL_HANDLE) {
		xrDestroyInstance(instance);
		instance = XR_NULL_HANDLE;
	}
	enabled_extensions.clear();

	// reset a bunch of things
	state = XR_SESSION_STATE_UNKNOWN;
	view_pose_valid = false;
	head_pose_valid = false;
	hand_tracking_ext = false;
	hand_motion_range_ext = false;
	monado_stick_on_ball_ext = false;
	hand_tracking_supported = false;
	running = false;
	initialised = false;
}

bool OpenXRApi::is_running() {
	if (!initialised) {
		return false;
	} else {
		return running;
	}
}

void OpenXRApi::on_resume() {
}

void OpenXRApi::on_pause() {
	// On Android, process_openxr stops being called before the events queue is
	// exhausted, so we invoke here one last time to empty it out.
	// Doing so allows to end in the proper state, and thus successfully resume.
	poll_events();
}

bool OpenXRApi::on_state_idle() {
	Godot::print("On state idle");
	return true;
}

bool OpenXRApi::on_state_ready() {
	Godot::print("On state ready");
	XrSessionBeginInfo sessionBeginInfo = {
		.type = XR_TYPE_SESSION_BEGIN_INFO,
		.next = NULL,
		.primaryViewConfigurationType = view_config_type
	};

	XrResult result = xrBeginSession(session, &sessionBeginInfo);
	if (!xr_result(result, "Failed to begin session!")) {
		return false;
	}

	// ignore failure on these for now, may need to improve this..
	// also need to find out if some of these should be moved further on..
	initialiseSpaces();
	initialiseSwapChains();
	initialiseHandTracking();

	bindActionSets();

	running = true;

	return true;
}

bool OpenXRApi::on_state_synchronized() {
	Godot::print("On state synchronized");
	return true;
}

bool OpenXRApi::on_state_visible() {
	Godot::print("On state visible");
	return true;
}

bool OpenXRApi::on_state_focused() {
	Godot::print("On state focused");
	return true;
}

bool OpenXRApi::on_state_stopping() {
	Godot::print("On state stopping");
	if (running) {
		XrResult result = xrEndSession(session);
		xr_result(result, "Failed to end session!");
		running = false;
	}

	// need to cleanup various things which would otherwise be re-allocated if we have a state change back to ready
	// note that cleaning up our action sets will invalidate many of the OpenXR nodes so we need to improve that as well.
	unbindActionSets();
	cleanupHandTracking();
	cleanupSwapChains();
	cleanupSpaces();

	return true;
}

bool OpenXRApi::on_state_loss_pending() {
	Godot::print("On state loss pending");
	uninitialize();
	return true;
}

bool OpenXRApi::on_state_exiting() {
	// we may want to trigger a signal back to the application to tell it, it should quit.
	Godot::print("On state exiting");
	uninitialize();
	return true;
}

bool OpenXRApi::is_initialised() {
	return initialised;
}

// hand tracking
const HandTracker *OpenXRApi::get_hand_tracker(uint32_t p_hand) const {
	if (p_hand < MAX_TRACKED_HANDS) {
		return &hand_trackers[p_hand];
	} else {
		return nullptr;
	}
}

XrHandJointsMotionRangeEXT OpenXRApi::get_motion_range(uint32_t p_hand) const {
	if (p_hand < MAX_TRACKED_HANDS) {
		return hand_trackers[p_hand].motion_range;
	} else {
		// just return this as the default
		return XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT;
	}
}

void OpenXRApi::set_motion_range(uint32_t p_hand, XrHandJointsMotionRangeEXT p_motion_range) {
	if (p_hand < MAX_TRACKED_HANDS) {
		hand_trackers[p_hand].motion_range = p_motion_range;
	}
}

// config
XrViewConfigurationType OpenXRApi::get_view_configuration_type() const {
	return view_config_type;
}

void OpenXRApi::set_view_configuration_type(const XrViewConfigurationType p_view_configuration_type) {
	view_config_type = p_view_configuration_type;
}

XrFormFactor OpenXRApi::get_form_factor() const {
	return form_factor;
}

void OpenXRApi::set_form_factor(const XrFormFactor p_form_factor) {
	if (is_initialised()) {
		Godot::print("OpenXR can't change form factor once OpenXR is initialised.");
		return;
	} else if (p_form_factor > (XrFormFactor)0 && p_form_factor <= (XrFormFactor)2) {
		form_factor = p_form_factor;
		return;
	} else {
		Godot::print("OpenXR form factor out of bounds");
		return;
	}
}

godot::Array OpenXRApi::get_enabled_extensions() const {
	godot::Array arr;

	for (int i = 0; i < enabled_extensions.size(); i++) {
		arr.push_back(String(enabled_extensions[i]));
	}

	return arr;
}

godot::String OpenXRApi::get_action_sets_json() const {
	return action_sets_json;
}

void OpenXRApi::set_action_sets_json(const godot::String &p_action_sets_json) {
	if (is_initialised()) {
		Godot::print("OpenXR can't change the action sets once OpenXR is initialised.");
		return;
	} else {
		action_sets_json = p_action_sets_json;
	}
}

godot::String OpenXRApi::get_interaction_profiles_json() const {
	return interaction_profiles_json;
}

void OpenXRApi::set_interaction_profiles_json(const godot::String &p_interaction_profiles_json) {
	if (is_initialised()) {
		Godot::print("OpenXR can't change the interaction profiles once OpenXR is initialised.");
		return;
	} else {
		interaction_profiles_json = p_interaction_profiles_json;
	}
}

// actions

ActionSet *OpenXRApi::get_action_set(const godot::String &p_name) {
	// Find it...
	for (uint64_t i = 0; i < action_sets.size(); i++) {
		if (action_sets[i]->get_name() == p_name) {
			return action_sets[i];
		}
	}

	return NULL;
}

Action *OpenXRApi::get_action(const char *p_name) {
	// Find this action within our action sets (assuming we don't have duplication)
	for (uint64_t i = 0; i < action_sets.size(); i++) {
		Action *action = action_sets[i]->get_action(p_name);
		if (action != NULL) {
			return action;
		}
	}

	return nullptr;
}

bool OpenXRApi::parse_action_sets(const godot::String &p_json) {
	// we'll use Godot's built-in JSON parser, good enough for this :)

	if (instance == XR_NULL_HANDLE) {
		Godot::print("OpenXR can't parse the action sets before OpenXR is initialised.");
		return false;
	}

	JSON *json_parser = JSON::get_singleton();
	Ref<JSONParseResult> parse_result = json_parser->parse(p_json);
	if (parse_result->get_error() != Error::OK) {
		Godot::print("Couldn't parse action set JSON {0}", parse_result->get_error_string());
		return false;
	}

	Variant json = parse_result->get_result();
	if (json.get_type() != Variant::ARRAY) {
		Godot::print("JSON is not formatted correctly");
		return false;
	}

	Array asets = json;
	for (int i = 0; i < asets.size(); i++) {
		if (asets[i].get_type() != Variant::DICTIONARY) {
			Godot::print("JSON is not formatted correctly");
			return false;
		}

		Dictionary action_set = asets[i];
		String action_set_name = action_set["name"];
		String localised_name = action_set["localised_name"];
		int priority = action_set["priority"];

		// Godot::print("New action set {0} - {1} ({2})", action_set_name, localised_name, priority);

		ActionSet *new_action_set = get_action_set(action_set_name);

		new_action_set = new ActionSet(this, action_set_name, localised_name, priority);
		if (new_action_set == NULL) {
			Godot::print("Couldn't create action set {0}", action_set_name);
			continue;
		}
		action_sets.push_back(new_action_set);

		Array actions = action_set["actions"];
		for (int a = 0; a < actions.size(); a++) {
			Dictionary action = actions[a];
			String type = action["type"];
			String name = action["name"];
			String localised_name = action["localised_name"];

			XrActionType action_type;
			if (type == "bool") {
				action_type = XR_ACTION_TYPE_BOOLEAN_INPUT;
			} else if (type == "float") {
				action_type = XR_ACTION_TYPE_FLOAT_INPUT;
			} else if (type == "vector2") {
				action_type = XR_ACTION_TYPE_VECTOR2F_INPUT;
			} else if (type == "pose") {
				action_type = XR_ACTION_TYPE_POSE_INPUT;
			} else if (type == "vibration") {
				action_type = XR_ACTION_TYPE_VIBRATION_OUTPUT;
			} else {
				Godot::print("Unknown action type {0} for action {1}", type, name);
				continue;
			}

			// Godot::print("New action {0} - {1} ({2}: {3})", name, localised_name, action_type, type);

			Array paths = action["paths"];
			std::vector<XrPath> toplevel_paths;
			for (int p = 0; p < paths.size(); p++) {
				String path = paths[p];
				XrPath new_path;
				XrResult res = xrStringToPath(instance, path.utf8().get_data(), &new_path);
				if (xr_result(res, "OpenXR couldn't register path {0}", path)) {
					toplevel_paths.push_back(new_path);
				}
			}

			Action *new_action = new_action_set->add_action(action_type, name, localised_name, toplevel_paths.size(), toplevel_paths.data());
			if (new_action == NULL) {
				Godot::print("Couldn't create action {0}", name);

				continue;
			}
		}
	}

	return true;
}

bool OpenXRApi::parse_interaction_profiles(const godot::String &p_json) {
	// We can push our interaction profiles directly to OpenXR. No need to keep them in memory.

	if (instance == XR_NULL_HANDLE) {
		Godot::print("OpenXR can't parse the interaction profiles before OpenXR is initialised.");
		return false;
	}

	JSON *json_parser = JSON::get_singleton();
	Ref<JSONParseResult> parse_result = json_parser->parse(p_json);
	if (parse_result->get_error() != Error::OK) {
		Godot::print("Couldn't parse interaction profile JSON {0}", parse_result->get_error_string());
		return false;
	}

	Variant json = parse_result->get_result();
	if (json.get_type() != Variant::ARRAY) {
		Godot::print("JSON is not formatted correctly");
		return false;
	}

	Array interaction_profiles = json;
	for (int i = 0; i < interaction_profiles.size(); i++) {
		if (interaction_profiles[i].get_type() != Variant::DICTIONARY) {
			Godot::print("JSON is not formatted correctly");
			return false;
		}

		Dictionary profile = interaction_profiles[i];
		String path_string = profile["path"];

		// Godot::print("Interaction profile {0}", path_string);

		XrPath interaction_profile_path;
		XrResult res = xrStringToPath(instance, path_string.utf8().get_data(), &interaction_profile_path);
		if (!xr_result(res, "OpenXR couldn't create path for {0}", path_string)) {
			continue;
		}

		std::vector<XrActionSuggestedBinding> xr_bindings;
		Array bindings = profile["bindings"];
		for (int b = 0; b < bindings.size(); b++) {
			Dictionary binding = bindings[b];

			String action_set_name = binding["set"];
			String action_name = binding["action"];
			Array io_paths = binding["paths"];

			ActionSet *action_set = get_action_set(action_set_name);
			if (action_set == NULL) {
				Godot::print("OpenXR Couldn't find set {0}", action_set_name);
				continue;
			}
			Action *action = action_set->get_action(action_name.utf8().get_data());
			if (action == NULL) {
				Godot::print("OpenXR Couldn't find action {0}", action);
				continue;
			}
			XrAction xr_action = action->get_action();
			if (xr_action == XR_NULL_HANDLE) {
				Godot::print("OpenXR Missing XrAction for {0}", action);
				continue;
			}
			for (int p = 0; p < io_paths.size(); p++) {
				String io_path_str = io_paths[p];
				XrPath io_path;
				XrResult res = xrStringToPath(instance, io_path_str.utf8().get_data(), &io_path);
				if (!xr_result(res, "OpenXR couldn't create path for {0}", io_path_str)) {
					continue;
				}

				XrActionSuggestedBinding bind = { xr_action, io_path };
				xr_bindings.push_back(bind);

				// Godot::print(" - Binding {0}/{1} - {2}", action_set_name, action_name, io_path_str);
			}
		}

		// update our profile
		const XrInteractionProfileSuggestedBinding suggestedBindings = {
			.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
			.next = NULL,
			.interactionProfile = interaction_profile_path,
			.countSuggestedBindings = (uint32_t)xr_bindings.size(),
			.suggestedBindings = xr_bindings.data()
		};

		XrResult result = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
		if (!xr_result(result, "failed to suggest bindings for {0}", path_string)) {
			// reporting is enough...
		}
	}

	return true;
}

bool OpenXRApi::check_graphics_requirements_gl(XrSystemId system_id) {
#ifdef ANDROID
	XrGraphicsRequirementsOpenGLESKHR opengl_reqs = {
		.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR,
		.next = nullptr
	};

	PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = nullptr;
	XrResult result = xrGetInstanceProcAddr(instance, "xrGetOpenGLESGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&pfnGetOpenGLESGraphicsRequirementsKHR);

	if (!xr_result(result, "Failed to get xrGetOpenGLESGraphicsRequirementsKHR fp!")) {
		return false;
	}

	result = pfnGetOpenGLESGraphicsRequirementsKHR(instance, system_id, &opengl_reqs);
	if (!xr_result(result, "Failed to get OpenGL graphics requirements!")) {
		return false;
	}
#else
	XrGraphicsRequirementsOpenGLKHR opengl_reqs = {
		.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR,
		.next = NULL
	};

	PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = NULL;
	XrResult result = xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&pfnGetOpenGLGraphicsRequirementsKHR);

	if (!xr_result(result, "Failed to get xrGetOpenGLGraphicsRequirementsKHR fp!")) {
		return false;
	}

	result = pfnGetOpenGLGraphicsRequirementsKHR(instance, system_id, &opengl_reqs);
	if (!xr_result(result, "Failed to get OpenGL graphics requirements!")) {
		return false;
	}
#endif

	XrVersion desired_opengl_version = XR_MAKE_VERSION(3, 3, 0);
	if (desired_opengl_version > opengl_reqs.maxApiVersionSupported || desired_opengl_version < opengl_reqs.minApiVersionSupported) {
		Godot::print(
				"OpenXR Runtime only supports OpenGL version {0}.{1} - {2}.{3}!", XR_VERSION_MAJOR(opengl_reqs.minApiVersionSupported), XR_VERSION_MINOR(opengl_reqs.minApiVersionSupported), XR_VERSION_MAJOR(opengl_reqs.maxApiVersionSupported), XR_VERSION_MINOR(opengl_reqs.maxApiVersionSupported));
		// it might still work
		return true;
	}
	return true;
}

XrResult OpenXRApi::acquire_image(int eye) {
	XrResult result;
	XrSwapchainImageAcquireInfo swapchainImageAcquireInfo = {
		.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, .next = nullptr
	};
	result = xrAcquireSwapchainImage(swapchains[eye], &swapchainImageAcquireInfo, &buffer_index[eye]);
	if (!xr_result(result, "failed to acquire swapchain image!")) {
		return result;
	}

	XrSwapchainImageWaitInfo swapchainImageWaitInfo = {
		.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
		.next = nullptr,
		.timeout = 17000000, /* timeout in nanoseconds */
	};
	result = xrWaitSwapchainImage(swapchains[eye], &swapchainImageWaitInfo);
	if (!xr_result(result, "failed to wait for swapchain image!")) {
		return result;
	}
	return XR_SUCCESS;
}

void OpenXRApi::render_openxr(int eye, uint32_t texid, bool has_external_texture_support) {
	if (!initialised) {
		return;
	}

	// printf("Render eye %d texture %d\n", eye, texid);
	XrResult result;

	// TODO: save resources don't react on rendering if we're not running (session hasn't begun or has ended)
	if (!running)
		return;

	// must have valid view pose for projection_views[eye].pose to submit layer
	if (!frameState.shouldRender || !view_pose_valid) {
		/* Godot 3.1: we acquire and release the image below in this function.
		 * Godot 3.2+: get_external_texture_for_eye() on acquires the image,
		 * therefore we have to release it here.
		 * TODO: Tell godot not to call get_external_texture_for_eye() when
		 * frameState.shouldRender is false, then remove the image release here
		 */
		if (has_external_texture_support) {
			XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
				.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
				.next = nullptr
			};
			result = xrReleaseSwapchainImage(swapchains[eye], &swapchainImageReleaseInfo);
			if (!xr_result(result, "failed to release swapchain image!")) {
				return;
			}
		}

		if (eye == 1) {
			// MS wants these in order..
			// submit 0 layers when we shouldn't render
			XrFrameEndInfo frameEndInfo = {
				.type = XR_TYPE_FRAME_END_INFO,
				.next = nullptr,
				.displayTime = frameState.predictedDisplayTime,
				.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
				.layerCount = 0,
				.layers = nullptr,
			};
			result = xrEndFrame(session, &frameEndInfo);
			xr_result(result, "failed to end frame!");
		}

		// neither eye is rendered
		return;
	}

	if (!has_external_texture_support) {
		result = acquire_image(eye);
		if (!xr_result(result, "failed to acquire swapchain image!")) {
			return;
		}

		glBindTexture(GL_TEXTURE_2D, texid);
#ifdef WIN32
		glCopyTexSubImage2D(
#elif ANDROID
		glCopyTexSubImage2D(
#else
		glCopyTextureSubImage2D(
#endif
				images[eye][buffer_index[eye]].image, 0, 0, 0,
				0, 0,
				configuration_views[eye].recommendedImageRectWidth,
				configuration_views[eye].recommendedImageRectHeight);
		glBindTexture(GL_TEXTURE_2D, 0);
		// printf("Copy godot texture %d into XR texture %d\n", texid,
		// images[eye][bufferIndex].image);
	} else {
		// printf("Godot already rendered into our textures\n");
	}

	XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
		.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
		.next = nullptr
	};
	result = xrReleaseSwapchainImage(swapchains[eye], &swapchainImageReleaseInfo);
	if (!xr_result(result, "failed to release swapchain image!")) {
		return;
	}

	projection_views[eye].fov = views[eye].fov;
	projection_views[eye].pose = views[eye].pose;

	if (eye == 1) {
		projectionLayer->views = projection_views;

		const XrCompositionLayerBaseHeader *const projectionlayers[1] = { (const XrCompositionLayerBaseHeader *const)
					projectionLayer };
		XrFrameEndInfo frameEndInfo = {
			.type = XR_TYPE_FRAME_END_INFO,
			.next = nullptr,
			.displayTime = frameState.predictedDisplayTime,
			.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
			.layerCount = 1,
			.layers = projectionlayers,
		};
		result = xrEndFrame(session, &frameEndInfo);
		if (!xr_result(result, "failed to end frame!")) {
			return;
		}
	}

#ifdef WIN32
	// not applicable
#elif ANDROID
	// not applicable
#elif __linux__
	// TODO: should not be necessary, but is for SteamVR since 1.16.x
	if (is_steamvr) {
		glXMakeCurrent(graphics_binding_gl.xDisplay, graphics_binding_gl.glxDrawable, graphics_binding_gl.glxContext);
	}
#endif
}

void OpenXRApi::fill_projection_matrix(int eye, godot_real p_z_near, godot_real p_z_far, godot_real *p_projection) {
	XrMatrix4x4f matrix;

	if (!initialised || !running) {
		CameraMatrix *cm = (CameraMatrix *)p_projection;

		cm->set_perspective(60.0, 1.0, p_z_near, p_z_far, false);

		return;
	}

	// TODO duplicate xrLocateViews call in fill_projection_matrix and process_openxr
	// fill_projection_matrix is called first, so we definitely need it here.
	XrViewLocateInfo viewLocateInfo = {
		.type = XR_TYPE_VIEW_LOCATE_INFO,
		.next = NULL,
		.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
		.displayTime = frameState.predictedDisplayTime,
		.space = play_space
	};
	XrViewState viewState = {
		.type = XR_TYPE_VIEW_STATE,
		.next = NULL
	};
	uint32_t viewCountOutput;
	XrResult result;
	result = xrLocateViews(session, &viewLocateInfo, &viewState, view_count, &viewCountOutput, views);
	if (!xr_result(result, "Could not locate views")) {
		return;
	}
	if (!xr_result(result, "Could not locate views")) {
		return;
	}

	XrMatrix4x4f_CreateProjectionFov(&matrix, GRAPHICS_OPENGL, views[eye].fov, p_z_near, p_z_far);

	for (int i = 0; i < 16; i++) {
		p_projection[i] = matrix.m[i];
	}
}

void OpenXRApi::update_actions() {
	XrResult result;

	if (!initialised || !running) {
		return;
	}

	// xrWaitFrame not run yet
	if (frameState.predictedDisplayTime == 0) {
		return;
	}

	if (state != XR_SESSION_STATE_FOCUSED) {
		// we must be in focused state in order to update our actions
		return;
	}

	// loop through our action sets
	std::vector<XrActiveActionSet> active_sets;
	for (uint64_t s = 0; s < action_sets.size(); s++) {
		if (action_sets[s]->is_active()) {
			XrActionSet action_set = action_sets[s]->get_action_set();
			if (action_set != XR_NULL_HANDLE) {
				XrActiveActionSet active_set = {
					.actionSet = action_set,
					.subactionPath = XR_NULL_PATH
				};
				active_sets.push_back(active_set);
			}
		}
	}

	if (active_sets.size() == 0) {
		// no active sets, no reason to sync.
		return;
	}

	// Godot::print("Synching {0} active action sets", active_sets.size());

	XrActionsSyncInfo syncInfo = {
		.type = XR_TYPE_ACTIONS_SYNC_INFO,
		.countActiveActionSets = (uint32_t)active_sets.size(),
		.activeActionSets = active_sets.data()
	};

	result = xrSyncActions(session, &syncInfo);
	xr_result(result, "failed to sync actions!");

	// Godot::print("Synched");

	/*
	// now handle our actions
	for (uint64_t s = 0; s < action_sets.size(); s++) {
		if (action_sets[s]->is_active()) {
			XrActionSet action_set = action_sets[s]->get_action_set();
			if (action_set != XR_NULL_HANDLE) {
				// Here we should do our generic handling of actions,
				// but this is not supported in Godot 3 yet,
				// we may add an intermediate solution
				// In Godot 4 we'll be sending out events depending on state changes

				// The problem here is that Godots action system is based on receiving events for our trigger points.
				// But OpenXR is already doing this for us and is already providing us with action
				// So we'll have to see how we make this work...
			}
		}
	}
	*/

	// now loop through our controllers, updated our positional trackers
	// and perform our backwards compatibility layer

	const float ws = ARVRServer::get_singleton()->get_world_scale();

	for (uint64_t i = 0; i < USER_INPUT_MAX; i++) {
		XrPath input_path = inputmaps[i].toplevel_path;
		if (input_path == XR_NULL_PATH) {
			// no path, skip this
			// Godot::print("Skipping {0}", inputmaps[i].name);
		} else {
			bool is_active = false;

			// Godot::print("Checking {0}", inputmaps[i].name);

			// If our aim pose is active, our controller is active
			// note, if the user has removed this action then our old controller approach becomes defunct
			if (default_actions[ACTION_AIM_POSE].action != NULL) {
				is_active = default_actions[ACTION_AIM_POSE].action->is_pose_active(input_path);
			}

			if (is_active) {
				if (inputmaps[i].godot_controller == -1) {
					// hate using const_cast here but godot_arvr_add_controller should have it's parameter defined as const, it doesn't change it...
					inputmaps[i].godot_controller = arvr_api->godot_arvr_add_controller(const_cast<char *>(inputmaps[i].name), (godot_int)i + 1, true, true);

					Godot::print("OpenXR mapped {0} to {1}", inputmaps[i].name, inputmaps[i].godot_controller);
				}

				// copy for readability
				int godot_controller = inputmaps[i].godot_controller;

				// Start with our pose, we put our ARVRController on our aim pose (may need to change this to our grip pose...)
				godot_transform controller_transform;
				Transform *t = (Transform *)&controller_transform;
				*t = default_actions[ACTION_AIM_POSE].action->get_as_pose(input_path, ws);

				arvr_api->godot_arvr_set_controller_transform(godot_controller, &controller_transform, true, true);

				// OK, so OpenXR will tell us if the value has changed and we could skip sending our value
				// but Godot also checks it so... just let Godot do it

				// Button and axis are hardcoded..
				if (default_actions[ACTION_FRONT_TRIGGER].action != NULL) {
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 2, default_actions[ACTION_FRONT_TRIGGER].action->get_as_float(input_path), false); // 0.0 -> 1.0
				}
				if (default_actions[ACTION_SIDE_TRIGGER].action != NULL) {
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 4, default_actions[ACTION_SIDE_TRIGGER].action->get_as_float(input_path), false); // 0.0 -> 1.0
				}
				if (default_actions[ACTION_JOYSTICK].action != NULL) {
					Vector2 v = default_actions[ACTION_JOYSTICK].action->get_as_vector(input_path);
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 0, v.x, true); // -1.0 -> 1.0
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 1, v.y, true); // -1.0 -> 1.0
				}
				if (default_actions[ACTION_AX_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 7, default_actions[ACTION_AX_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_BYM_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 1, default_actions[ACTION_BYM_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_FRONT_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 15, default_actions[ACTION_FRONT_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_SIDE_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 2, default_actions[ACTION_SIDE_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_JOYSTICK_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 14, default_actions[ACTION_JOYSTICK_BUTTON].action->get_as_bool(input_path));
				}

				if (default_actions[ACTION_HAPTIC].action != NULL) {
					// Godot currently only gives us a float between 0.0 and 1.0 for rumble strength.
					// Full haptic control will be offered through another object
					float haptic = arvr_api->godot_arvr_get_controller_rumble(godot_controller);
					if (haptic > 0.0) {
						// 17000.0 nanoseconds is slightly more then the duration of one frame if we're outputting at 60fps
						// so if we sustain our pulse we should be issuing a new pulse before the old one ends
						default_actions[ACTION_HAPTIC].action->do_haptic_pulse(input_path, 17000.0, XR_FREQUENCY_UNSPECIFIED, haptic);
					}
				}
			} else if (inputmaps[i].godot_controller != -1) {
				// Remove our controller, it's no longer active
				arvr_api->godot_arvr_remove_controller(inputmaps[i].godot_controller);
				inputmaps[i].godot_controller = -1;
			}
		}
	}
}

void OpenXRApi::update_handtracking() {
	if (!initialised || !running) {
		return;
	}

	if (!hand_tracking_supported) {
		return;
	}

	const XrTime time = frameState.predictedDisplayTime;
	XrResult result;

	for (int i = 0; i < 2; i++) {
		if (hand_trackers[i].hand_tracker == XR_NULL_HANDLE) {
			XrHandTrackerCreateInfoEXT createInfo = {
				.type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
				.next = nullptr,
				.hand = i == 0 ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT,
				.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT,
			};

			result = xrCreateHandTrackerEXT(session, &createInfo, &hand_trackers[i].hand_tracker);
			if (!xr_result(result, "Failed to obtain hand tracking information")) {
				// not successful? then we do nothing.
				hand_trackers[i].is_initialised = false;
			} else {
				hand_trackers[i].velocities.type = XR_TYPE_HAND_JOINT_VELOCITIES_EXT;
				hand_trackers[i].velocities.jointCount = XR_HAND_JOINT_COUNT_EXT;
				hand_trackers[i].velocities.jointVelocities = hand_trackers[i].joint_velocities;

				hand_trackers[i].locations.type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT;
				hand_trackers[i].locations.next = &hand_trackers[i].velocities;
				hand_trackers[i].locations.isActive = false;
				hand_trackers[i].locations.jointCount = XR_HAND_JOINT_COUNT_EXT;
				hand_trackers[i].locations.jointLocations = hand_trackers[i].joint_locations;

				hand_trackers[i].is_initialised = true;
			}
		}

		if (hand_trackers[i].is_initialised) {
			XrHandJointsLocateInfoEXT locateInfo = {
				.type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
				.next = nullptr,
				.baseSpace = play_space,
				.time = time,
			};
			XrHandJointsMotionRangeInfoEXT motionRangeInfo;

			if (hand_motion_range_ext) {
				motionRangeInfo.type = XR_TYPE_HAND_JOINTS_MOTION_RANGE_INFO_EXT;
				motionRangeInfo.next = nullptr;
				motionRangeInfo.handJointsMotionRange = hand_trackers[i].motion_range;

				locateInfo.next = &motionRangeInfo;
			}

			// Godot::print("Obtaining hand joint info for {0}", i);

			result = xrLocateHandJointsEXT(hand_trackers[i].hand_tracker, &locateInfo, &hand_trackers[i].locations);
			if (xr_result(result, "failed to get tracking for hand {0}!", i)) {
				// For some reason an inactive controller isn't coming back as inactive but has coordinates either as NAN or very large
				const XrPosef &palm = hand_trackers[i].joint_locations[XR_HAND_JOINT_PALM_EXT].pose;
				if (
						!hand_trackers[i].locations.isActive || isnan(palm.position.x) || palm.position.x < -1000000.00 || palm.position.x > 1000000.00) {
					hand_trackers[i].locations.isActive = false; // workaround, make sure its inactive
					// printf("Hand %i inactive\n", i);
				} else {
					// we have our hand tracking info....

					// Godot::print("Hand {0}: ({1}, {2}, {3})\n", i, palm.position.x, palm.position.y, palm.position.z);
				}
			}
		}
	}
}

void OpenXRApi::recommended_rendertarget_size(uint32_t *width, uint32_t *height) {
	if (!initialised) {
		*width = 0;
		*height = 0;
	} else {
		*width = configuration_views[0].recommendedImageRectWidth;
		*height = configuration_views[0].recommendedImageRectHeight;
	}
}

void OpenXRApi::transform_from_matrix(godot_transform *p_dest, XrMatrix4x4f *matrix, float p_world_scale) {
	godot_basis basis;
	godot_vector3 origin;
	float *basis_ptr =
			(float *)&basis; // Godot can switch between real_t being
	// double or float.. which one is used...
	float m[4][4];

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			m[i][j] = matrix->m[(i * 4) + j];
		}
	}

	int k = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			basis_ptr[k++] = m[i][j];
		};
	};

	api->godot_vector3_new(&origin, -m[3][0] * p_world_scale,
			-m[3][1] * p_world_scale,
			-m[3][2] * p_world_scale);
	// printf("Origin %f %f %f\n", origin.x, origin.y, origin.z);
	api->godot_transform_new(p_dest, &basis, &origin);
};

bool OpenXRApi::get_view_transform(int eye, float world_scale, godot_transform *transform_for_eye) {
	if (!initialised || !running) {
		return false;
	}

	// xrWaitFrame not run yet
	if (frameState.predictedDisplayTime == 0) {
		return false;
	}

	if (views == NULL || !view_pose_valid) {
		return false;
	}

	Transform *t = (Transform *)transform_for_eye;
	*t = transform_from_pose(views[eye].pose, world_scale);

	return true;
}

bool OpenXRApi::get_head_center(float world_scale, godot_transform *transform) {
	if (!initialised || !running) {
		return false;
	}

	// xrWaitFrame not run yet
	if (frameState.predictedDisplayTime == 0) {
		return false;
	}

	XrResult result;
	XrSpaceLocation location = {
		.type = XR_TYPE_SPACE_LOCATION,
		.next = NULL
	};
	result = xrLocateSpace(view_space, play_space, frameState.predictedDisplayTime, &location);
	if (!xr_result(result, "Failed to locate view space in play space!")) {
		return false;
	}

	bool pose_valid = (location.locationFlags & (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT + XR_SPACE_LOCATION_POSITION_VALID_BIT)) == (XR_SPACE_LOCATION_ORIENTATION_VALID_BIT + XR_SPACE_LOCATION_POSITION_VALID_BIT);
	if (head_pose_valid != pose_valid) {
		// prevent error spam
		head_pose_valid = pose_valid;
		if (!head_pose_valid) {
			Godot::print_error("OpenXR head space location not valid (check tracking?)", __FUNCTION__, __FILE__, __LINE__);
#ifdef DEBUG
		} else {
			Godot::print("OpenVR Head pose is now valid");
#endif
		}
	}

	if (!head_pose_valid) {
		return false;
	}

	Transform *t = (Transform *)transform;
	*t = transform_from_pose(location.pose, world_scale);

	return true;
}

int OpenXRApi::get_external_texture_for_eye(int eye, bool *has_support) {
	if (!initialised) {
		return 0;
	}

	// this won't prevent us from rendering but we won't output to OpenXR
	if (!running || state >= XR_SESSION_STATE_STOPPING)
		return 0;

	// this only gets called from Godot 3.2 and newer, allows us to use
	// OpenXR swapchain directly.

	XrResult result = acquire_image(eye);
	if (!xr_result(result, "failed to acquire swapchain image!")) {
		return 0;
	}

#ifdef WIN32
	// not applicable
#elif ANDROID
	// not applicable
#elif __linux__
	// TODO: should not be necessary, but is for SteamVR since 1.16.x
	if (is_steamvr) {
		glXMakeCurrent(graphics_binding_gl.xDisplay, graphics_binding_gl.glxDrawable, graphics_binding_gl.glxContext);
	}
#endif

	// process should be called by now but just in case...
	if (state > XR_SESSION_STATE_UNKNOWN && buffer_index != NULL) {
		// make sure we know that we're rendering directly to our
		// texture chain
		*has_support = true;
		// printf("eye %d: get texture %d\n", eye, buffer_index[eye]);
		return images[eye][buffer_index[eye]].image;
	}

	return 0;
}

bool OpenXRApi::poll_events() {
	XrEventDataBuffer runtimeEvent = {
		.type = XR_TYPE_EVENT_DATA_BUFFER,
		.next = nullptr
	};

	XrResult pollResult = xrPollEvent(instance, &runtimeEvent);
	while (pollResult == XR_SUCCESS) {
		switch (runtimeEvent.type) {
			case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
				XrEventDataEventsLost *event = (XrEventDataEventsLost *)&runtimeEvent;

				Godot::print("OpenXR EVENT: {0} event data lost!", event->lostEventCount);
				// we probably didn't poll fast enough'
			} break;
			case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR: {
				XrEventDataVisibilityMaskChangedKHR *event = (XrEventDataVisibilityMaskChangedKHR *)&runtimeEvent;
				Godot::print("OpenXR EVENT: STUB: visibility mask changed");
			} break;
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
				XrEventDataInstanceLossPending *event = (XrEventDataInstanceLossPending *)&runtimeEvent;
				Godot::print("OpenXR EVENT: instance loss pending at {0}!", event->lossTime);
				// running = false;
				return false;
			} break;
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				XrEventDataSessionStateChanged *event = (XrEventDataSessionStateChanged *)&runtimeEvent;

				state = event->state;
				if (state >= XR_SESSION_STATE_MAX_ENUM) {
					Godot::print("OpenXR EVENT: session state changed to UNKNOWN - {0}", state);
				} else {
					Godot::print("OpenXR EVENT: session state changed to {0}", session_states[state]);

					switch (state) {
						case XR_SESSION_STATE_IDLE:
							on_state_idle();
							break;
						case XR_SESSION_STATE_READY:
							on_state_ready();
							break;
						case XR_SESSION_STATE_SYNCHRONIZED:
							on_state_synchronized();
							break;
						case XR_SESSION_STATE_VISIBLE:
							on_state_visible();
							break;
						case XR_SESSION_STATE_FOCUSED:
							on_state_focused();
							break;
						case XR_SESSION_STATE_STOPPING:
							on_state_stopping();
							break;
						case XR_SESSION_STATE_LOSS_PENDING:
							on_state_loss_pending();
							break;
						case XR_SESSION_STATE_EXITING:
							on_state_exiting();
							break;
						default:
							break;
					}
				}
			} break;
			case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
				XrEventDataReferenceSpaceChangePending *event = (XrEventDataReferenceSpaceChangePending *)&runtimeEvent;
				Godot::print("OpenXR EVENT: reference space type {0} change pending!", event->referenceSpaceType);
				// TODO: do something
			} break;
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
				Godot::print("OpenXR EVENT: interaction profile changed!");

				XrEventDataInteractionProfileChanged *event = (XrEventDataInteractionProfileChanged *)&runtimeEvent;

				XrInteractionProfileState profile_state = {
					.type = XR_TYPE_INTERACTION_PROFILE_STATE,
					.next = nullptr
				};

				for (int i = 0; i < USER_INPUT_MAX; i++) {
					XrPath input_path = inputmaps[i].toplevel_path;
					if (input_path == XR_NULL_PATH) {
						// incorrect path
						continue;
					}

					// Godot::print("Checking {0} ({1})", inputmaps[i].name, (uint64_t)input_path);

					XrResult res = xrGetCurrentInteractionProfile(event->session, input_path, &profile_state);
					if (!xr_result(res, "Failed to get interaction profile for {0}", inputmaps[i].name)) {
						continue;
					}

					XrPath new_profile = profile_state.interactionProfile;
					if (inputmaps[i].active_profile != new_profile) {
						inputmaps[i].active_profile = new_profile;
						if (new_profile == XR_NULL_PATH) {
							Godot::print("OpenXR No interaction profile for {0}", inputmaps[i].name);
							continue;
						}

						uint32_t strl;
						char profile_str[XR_MAX_PATH_LENGTH];
						res = xrPathToString(instance, new_profile, XR_MAX_PATH_LENGTH, &strl, profile_str);
						if (!xr_result(res, "Failed to get interaction profile path str for {0}", inputmaps[i].name)) {
							continue;
						}

						Godot::print("OpenXR Event: Interaction profile changed for {0}: {1}", inputmaps[i].name, profile_str);
					}
				}

				// TODO: do something
			} break;
			default:
				Godot::print_warning(String("OpenXR Unhandled event type ") + String::num_int64(runtimeEvent.type), __FUNCTION__, __FILE__, __LINE__);
				break;
		}

		runtimeEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
		pollResult = xrPollEvent(instance, &runtimeEvent);
	}
	if (pollResult == XR_EVENT_UNAVAILABLE) {
		// processed all events in the queue
		return true;
	} else {
		Godot::print_error("OpenXR Failed to poll events!", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
}

void OpenXRApi::process_openxr() {
	if (!initialised) {
		return;
	}

	if (!poll_events()) {
		return;
	}

	XrResult result;

	if (!running) {
		return;
	}

	XrFrameWaitInfo frameWaitInfo = {
		.type = XR_TYPE_FRAME_WAIT_INFO,
		.next = nullptr
	};
	result = xrWaitFrame(session, &frameWaitInfo, &frameState);
	if (!xr_result(result, "xrWaitFrame() was not successful, exiting...")) {
		return;
	}

	update_actions();
	// TODO: Tends to crash randomly on Quest, needs to investigate.
#ifndef ANDROID
	update_handtracking();
#endif

	XrViewLocateInfo viewLocateInfo = {
		.type = XR_TYPE_VIEW_LOCATE_INFO,
		.next = nullptr,
		.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
		.displayTime = frameState.predictedDisplayTime,
		.space = play_space
	};
	XrViewState viewState = {
		.type = XR_TYPE_VIEW_STATE,
		.next = nullptr
	};
	uint32_t viewCountOutput;
	result = xrLocateViews(session, &viewLocateInfo, &viewState, view_count, &viewCountOutput, views);
	if (!xr_result(result, "Could not locate views")) {
		return;
	}

	bool pose_valid = true;
	for (uint64_t i = 0; i < viewCountOutput; i++) {
		if ((viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0 ||
				(viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0) {
			pose_valid = false;
		}
	}
	if (view_pose_valid != pose_valid) {
		view_pose_valid = pose_valid;
		if (!view_pose_valid) {
			Godot::print_warning("OpenXR View pose became invalid", __FUNCTION__, __FILE__, __LINE__);
#ifdef DEBUG
		} else {
			Godot::print("OpenXR View pose became valid");
#endif
		}
	}

	// let's start our frame..
	XrFrameBeginInfo frameBeginInfo = {
		.type = XR_TYPE_FRAME_BEGIN_INFO,
		.next = nullptr
	};

	result = xrBeginFrame(session, &frameBeginInfo);
	if (!xr_result(result, "failed to begin frame!")) {
		return;
	}

	if (frameState.shouldRender) {
		// TODO: Tell godot not do render VR to save resources.
		// See render_openxr() for the corresponding early exit.
	}

#ifdef WIN32
	// not applicable
#elif ANDROID
	// not applicable
#elif __linux__
	// TODO: should not be necessary, but is for SteamVR since 1.16.x
	if (is_steamvr) {
		glXMakeCurrent(graphics_binding_gl.xDisplay, graphics_binding_gl.glxDrawable, graphics_binding_gl.glxContext);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Utility functions

Transform OpenXRApi::transform_from_pose(const XrPosef &p_pose, float p_world_scale) {
	Quat q(p_pose.orientation.x, p_pose.orientation.y, p_pose.orientation.z, p_pose.orientation.w);
	Basis basis(q);
	Vector3 origin(p_pose.position.x * p_world_scale, p_pose.position.y * p_world_scale, p_pose.position.z * p_world_scale);

	return Transform(basis, origin);
}

template <typename T>
Transform _transform_from_space_location(OpenXRApi &api, const T &p_location, float p_world_scale) {
	Basis basis;
	Vector3 origin;
	const auto &pose = p_location.pose;
	if (p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
		Quat q(pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w);
		basis = Basis(q);
	}
	if (p_location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
		origin = Vector3(pose.position.x, pose.position.y, pose.position.z) * p_world_scale;
	}
	return Transform(basis, origin);
}

Transform OpenXRApi::transform_from_space_location(const XrSpaceLocation &p_location, float p_world_scale) {
	return _transform_from_space_location(*this, p_location, p_world_scale);
}

Transform OpenXRApi::transform_from_space_location(const XrHandJointLocationEXT &p_location, float p_world_scale) {
	return _transform_from_space_location(*this, p_location, p_world_scale);
}
