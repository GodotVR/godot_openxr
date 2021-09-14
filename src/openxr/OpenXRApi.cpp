////////////////////////////////////////////////////////////////////////////////////////////////
// Helper calls and singleton container for accessing openxr

#include <godot_cpp/classes/xr_server.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

// these are no longer supported..
// #include <CameraMatrix.hpp>
// #include <godot_cpp/classes/json_parse_result.hpp>

#include "openxr/OpenXRApi.h"
#include "openxr/include/signals_util.h"

#include <algorithm>
#include <cmath>
#include <map>

#ifdef ANDROID
#include <jni/openxr_plugin_wrapper.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// Default action set configuration

// TODO: it makes sense to include this in source because we'll store any user defined version in Godot scenes
// but there has to be a nicer way to embed it :)

const char *OpenXRApi::default_action_sets_json = R"===(
[
	{
		"name": "godot",
		"localised_name": "Action Set Used by Godot",
		"priority": 0,
		"actions": [
			{
				"type": "pose",
				"name": "aim_pose",
				"localised_name": "Aim Pose",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "pose",
				"name": "grip_pose",
				"localised_name": "Grip Pose",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "float",
				"name": "front_trigger",
				"localised_name": "Front trigger",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "float",
				"name": "side_trigger",
				"localised_name": "Side trigger",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "vector2",
				"name": "primary",
				"localised_name": "Primary joystick/thumbstick/trackpad",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "vector2",
				"name": "secondary",
				"localised_name": "Secondary joystick/thumbstick/trackpad",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "ax_button",
				"localised_name": "A and X buttons",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "by_button",
				"localised_name": "B, Y buttons",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "ax_touch",
				"localised_name": "A and X touch",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "by_touch",
				"localised_name": "B, Y touch",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "menu_button",
				"localised_name": "Menu button",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "select_button",
				"localised_name": "Select button",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "front_button",
				"localised_name": "Front trigger as a button",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "front_touch",
				"localised_name": "Finger on front trigger",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "side_button",
				"localised_name": "Side trigger as a button",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "primary_button",
				"localised_name": "Primary joystick/thumbstick/trackpad click",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "secondary_button",
				"localised_name": "Secondary joystick/thumbstick/trackpad click",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "primary_touch",
				"localised_name": "Primary joystick/thumbstick/trackpad touch",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "bool",
				"name": "secondary_touch",
				"localised_name": "Secondary joystick/thumbstick/trackpad touch",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			},
			{
				"type": "vibration",
				"name": "haptic",
				"localised_name": "Controller haptic vibration",
				"paths": [
					"/user/hand/left",
					"/user/hand/right"
				]
			}
		]
	}
]
)===";

// documentated interaction profiles can be found here: https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#semantic-path-interaction-profiles
const char *OpenXRApi::default_interaction_profiles_json = R"===(
[
	{
		"path": "/interaction_profiles/khr/simple_controller",
		"bindings": [
			{
				"set": "godot",
				"action": "aim_pose",
				"paths": [
					"/user/hand/left/input/aim/pose",
					"/user/hand/right/input/aim/pose"
				]
			},
			{
				"set": "godot",
				"action": "grip_pose",
				"paths": [
					"/user/hand/left/input/grip/pose",
					"/user/hand/right/input/grip/pose"
				]
			},
			{
				"set": "godot",
				"action": "menu_button",
				"paths": [
					"/user/hand/left/input/menu/click",
					"/user/hand/right/input/menu/click"
				]
			},
			{
				"set": "godot",
				"action": "select_button",
				"paths": [
					"/user/hand/left/input/select/click",
					"/user/hand/right/input/select/click"
				]
			},
			{
				"set": "godot",
				"action": "haptic",
				"paths": [
					"/user/hand/left/output/haptic",
					"/user/hand/right/output/haptic"
				]
			},
		],
	},
	{
		"path": "/interaction_profiles/htc/vive_controller",
		"bindings": [
			{
				"set": "godot",
				"action": "aim_pose",
				"paths": [
					"/user/hand/left/input/aim/pose",
					"/user/hand/right/input/aim/pose"
				]
			},
			{
				"set": "godot",
				"action": "grip_pose",
				"paths": [
					"/user/hand/left/input/grip/pose",
					"/user/hand/right/input/grip/pose"
				]
			},
			{
				"set": "godot",
				"action": "select_button",
				"paths": [
					"/user/hand/left/input/system/click",
					"/user/hand/right/input/system/click"
				]
			},
			{
				"set": "godot",
				"action": "front_trigger",
				"paths": [
					"/user/hand/left/input/trigger/value",
					"/user/hand/right/input/trigger/value"
				]
			},
			{
				"set": "godot",
				"action": "side_trigger",
				"paths": [
					"/user/hand/left/input/squeeze/click",
					"/user/hand/right/input/squeeze/click"
				]
			},
			{
				"set": "godot",
				"action": "primary",
				"paths": [
					"/user/hand/left/input/trackpad",
					"/user/hand/right/input/trackpad"
				]
			},
			{
				"set": "godot",
				"action": "front_button",
				"paths": [
					"/user/hand/left/input/trigger/click",
					"/user/hand/right/input/trigger/click"
				]
			},
			{
				"set": "godot",
				"action": "side_button",
				"paths": [
					"/user/hand/left/input/squeeze/click",
					"/user/hand/right/input/squeeze/click"
				]
			},
			{
				"set": "godot",
				"action": "menu_button",
				"paths": [
					"/user/hand/left/input/menu/click",
					"/user/hand/right/input/menu/click"
				]
			},
			{
				"set": "godot",
				"action": "primary_button",
				"paths": [
					"/user/hand/left/input/trackpad/click",
					"/user/hand/right/input/trackpad/click"
				]
			},
			{
				"set": "godot",
				"action": "primary_touch",
				"paths": [
					"/user/hand/left/input/trackpad/touch",
					"/user/hand/right/input/trackpad/touch"
				]
			},
			{
				"set": "godot",
				"action": "haptic",
				"paths": [
					"/user/hand/left/output/haptic",
					"/user/hand/right/output/haptic"
				]
			},
		],
	},
	{
		"path": "/interaction_profiles/microsoft/motion_controller",
		"bindings": [
			{
				"set": "godot",
				"action": "aim_pose",
				"paths": [
					"/user/hand/left/input/aim/pose",
					"/user/hand/right/input/aim/pose"
				]
			},
			{
				"set": "godot",
				"action": "grip_pose",
				"paths": [
					"/user/hand/left/input/grip/pose",
					"/user/hand/right/input/grip/pose"
				]
			},
			{
				"set": "godot",
				"action": "front_trigger",
				"paths": [
					"/user/hand/left/input/trigger/value",
					"/user/hand/right/input/trigger/value"
				]
			},
			{
				"set": "godot",
				"action": "side_trigger",
				"paths": [
					"/user/hand/left/input/squeeze/click",
					"/user/hand/right/input/squeeze/click"
				]
			},
			{
				"set": "godot",
				"action": "primary",
				"paths": [
					"/user/hand/left/input/thumbstick",
					"/user/hand/right/input/thumbstick"
				]
			},
			{
				"set": "godot",
				"action": "secondary",
				"paths": [
					"/user/hand/left/input/trackpad",
					"/user/hand/right/input/trackpad"
				]
			},
			{
				"set": "godot",
				"action": "front_button",
				"paths": [
					"/user/hand/left/input/trigger/value",
					"/user/hand/right/input/trigger/value"
				]
			},
			{
				"set": "godot",
				"action": "side_button",
				"paths": [
					"/user/hand/left/input/squeeze/click",
					"/user/hand/right/input/squeeze/click"
				]
			},
			{
				"set": "godot",
				"action": "menu_button",
				"paths": [
					"/user/hand/left/input/menu/click",
					"/user/hand/right/input/menu/click"
				]
			},
			{
				"set": "godot",
				"action": "primary_button",
				"paths": [
					"/user/hand/left/input/thumbstick/click",
					"/user/hand/right/input/thumbstick/click"
				]
			},
			{
				"set": "godot",
				"action": "secondary_button",
				"paths": [
					"/user/hand/left/input/trackpad/click",
					"/user/hand/right/input/trackpad/click"
				]
			},
			{
				"set": "godot",
				"action": "secondary_touch",
				"paths": [
					"/user/hand/left/input/trackpad/touch",
					"/user/hand/right/input/trackpad/touch"
				]
			},
			{
				"set": "godot",
				"action": "haptic",
				"paths": [
					"/user/hand/left/output/haptic",
					"/user/hand/right/output/haptic"
				]
			},
		],
	},
	{
		"path": "/interaction_profiles/oculus/touch_controller",
		"bindings": [
			{
				"set": "godot",
				"action": "aim_pose",
				"paths": [
					"/user/hand/left/input/aim/pose",
					"/user/hand/right/input/aim/pose"
				]
			},
			{
				"set": "godot",
				"action": "grip_pose",
				"paths": [
					"/user/hand/left/input/grip/pose",
					"/user/hand/right/input/grip/pose"
				]
			},
			{
				"set": "godot",
				"action": "front_trigger",
				"paths": [
					"/user/hand/left/input/trigger/value",
					"/user/hand/right/input/trigger/value"
				]
			},
			{
				"set": "godot",
				"action": "side_trigger",
				"paths": [
					"/user/hand/left/input/squeeze/value",
					"/user/hand/right/input/squeeze/value"
				]
			},
			{
				"set": "godot",
				"action": "primary",
				"paths": [
					"/user/hand/left/input/thumbstick",
					"/user/hand/right/input/thumbstick"
				]
			},
			{
				"set": "godot",
				"action": "ax_button",
				"paths": [
					"/user/hand/left/input/x/click",
					"/user/hand/right/input/a/click"
				]
			},
			{
				"set": "godot",
				"action": "by_button",
				"paths": [
					"/user/hand/left/input/y/click",
					"/user/hand/right/input/b/click"
				]
			},
			{
				"set": "godot",
				"action": "ax_touch",
				"paths": [
					"/user/hand/left/input/x/touch",
					"/user/hand/right/input/a/touch"
				]
			},
			{
				"set": "godot",
				"action": "by_touch",
				"paths": [
					"/user/hand/left/input/y/touch",
					"/user/hand/right/input/b/touch"
				]
			},
			{
				"set": "godot",
				"action": "front_button",
				"paths": [
					"/user/hand/left/input/trigger/value",
					"/user/hand/right/input/trigger/value"
				]
			},
			{
				"set": "godot",
				"action": "front_touch",
				"paths": [
					"/user/hand/left/input/trigger/touch",
					"/user/hand/right/input/trigger/touch"
				]
			},
			{
				"set": "godot",
				"action": "side_button",
				"paths": [
					"/user/hand/left/input/squeeze/value",
					"/user/hand/right/input/squeeze/value"
				]
			},
			{
				"set": "godot",
				"action": "menu_button",
				"paths": [
					"/user/hand/left/input/menu/click",
				]
			},
			{
				"set": "godot",
				"action": "primary_button",
				"paths": [
					"/user/hand/left/input/thumbstick/click",
					"/user/hand/right/input/thumbstick/click"
				]
			},
			{
				"set": "godot",
				"action": "primary_touch",
				"paths": [
					"/user/hand/left/input/thumbstick/touch",
					"/user/hand/right/input/thumbstick/touch"
				]
			},
			{
				"set": "godot",
				"action": "haptic",
				"paths": [
					"/user/hand/left/output/haptic",
					"/user/hand/right/output/haptic"
				]
			},
		],
	},
	{
		"path": "/interaction_profiles/valve/index_controller",
		"bindings": [
			{
				"set": "godot",
				"action": "aim_pose",
				"paths": [
					"/user/hand/left/input/aim/pose",
					"/user/hand/right/input/aim/pose"
				]
			},
			{
				"set": "godot",
				"action": "grip_pose",
				"paths": [
					"/user/hand/left/input/grip/pose",
					"/user/hand/right/input/grip/pose"
				]
			},
			{
				"set": "godot",
				"action": "front_trigger",
				"paths": [
					"/user/hand/left/input/trigger/value",
					"/user/hand/right/input/trigger/value"
				]
			},
			{
				"set": "godot",
				"action": "side_trigger",
				"paths": [
					"/user/hand/left/input/squeeze/value",
					"/user/hand/right/input/squeeze/value"
				]
			},
			{
				"set": "godot",
				"action": "primary",
				"paths": [
					"/user/hand/left/input/thumbstick",
					"/user/hand/right/input/thumbstick"
				]
			},
			{
				"set": "godot",
				"action": "secondary",
				"paths": [
					"/user/hand/left/input/trackpad",
					"/user/hand/right/input/trackpad"
				]
			},
			{
				"set": "godot",
				"action": "ax_button",
				"paths": [
					"/user/hand/left/input/a/click",
					"/user/hand/right/input/a/click"
				]
			},
			{
				"set": "godot",
				"action": "by_button",
				"paths": [
					"/user/hand/left/input/b/click",
					"/user/hand/right/input/b/click"
				]
			},
			{
				"set": "godot",
				"action": "ax_touch",
				"paths": [
					"/user/hand/left/input/a/touch",
					"/user/hand/right/input/a/touch"
				]
			},
			{
				"set": "godot",
				"action": "by_touch",
				"paths": [
					"/user/hand/left/input/b/touch",
					"/user/hand/right/input/b/touch"
				]
			},
			{
				"set": "godot",
				"action": "front_button",
				"paths": [
					"/user/hand/left/input/trigger/click",
					"/user/hand/right/input/trigger/click"
				]
			},
			{
				"set": "godot",
				"action": "front_touch",
				"paths": [
					"/user/hand/left/input/trigger/touch",
					"/user/hand/right/input/trigger/touch"
				]
			},
			{
				"set": "godot",
				"action": "side_button",
				"paths": [
					"/user/hand/left/input/squeeze/value",
					"/user/hand/right/input/squeeze/value"
				]
			},
			{
				"set": "godot",
				"action": "menu_button",
				"paths": [
					"/user/hand/left/input/system/click",
					"/user/hand/right/input/system/click"
				]
			},
			{
				"set": "godot",
				"action": "primary_button",
				"paths": [
					"/user/hand/left/input/thumbstick/click",
					"/user/hand/right/input/thumbstick/click"
				]
			},
			{
				"set": "godot",
				"action": "secondary_button",
				"paths": [
					"/user/hand/left/input/trackpad/force",
					"/user/hand/right/input/trackpad/force"
				]
			},
			{
				"set": "godot",
				"action": "primary_touch",
				"paths": [
					"/user/hand/left/input/thumbstick/touch",
					"/user/hand/right/input/thumbstick/touch"
				]
			},
			{
				"set": "godot",
				"action": "secondary_touch",
				"paths": [
					"/user/hand/left/input/trackpad/touch",
					"/user/hand/right/input/trackpad/touch"
				]
			},
			{
				"set": "godot",
				"action": "haptic",
				"paths": [
					"/user/hand/left/output/haptic",
					"/user/hand/right/output/haptic"
				]
			}
		]
	}
]
)===";

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
		UtilityFunctions::print("OpenXR: tried to release non-existent OpenXR context\n");
#endif
	} else if (singleton->use_count > 1) {
		// decrease use count
		singleton->use_count--;

#ifdef DEBUG
		UtilityFunctions::print("OpenXR: decreased use count to {0}", singleton->use_count);
#endif
	} else {
		// cleanup openxr
#ifdef DEBUG
		UtilityFunctions::print("OpenXR releasing OpenXR context");
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
		UtilityFunctions::print("OpenXR increased use count to {0}", singleton->use_count);
#endif
	} else {
		singleton = new OpenXRApi();
		if (singleton == nullptr) {
			UtilityFunctions::printerr("OpenXR interface creation failed");
#ifdef DEBUG
		} else {
			UtilityFunctions::print("OpenXR interface creation successful");
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
		UtilityFunctions::printerr("Couldn't allocate memory for view configurations");
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
	ERR_FAIL_NULL_V_MSG(referenceSpaces, false, "OpenXR Couldn't allocate memory for reference spaces");

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
	UtilityFunctions::print("OpenXR initialiseInstance");
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
	ERR_FAIL_NULL_V_MSG(layer_properties, false, "Couldn't allocate memory for api layer properties");

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
		UtilityFunctions::print("Found layer {0}", layer_properties[i].layerName);
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
	ERR_FAIL_NULL_V_MSG(extensionProperties, false, "OpenXR Couldn't allocate memory for extension properties");
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

	// Append the extensions requested by the registered extension wrappers.
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		std::map<const char *, bool *> wrapper_request_extensions = wrapper->get_request_extensions();
		request_extensions.insert(wrapper_request_extensions.begin(), wrapper_request_extensions.end());
	}

	// TODO OpenGL isn't supported yet in Godot 4, once it is we probably need to change this
	if (video_driver == godot::OS::VIDEO_DRIVER_VULKAN) {
		request_extensions[XR_KHR_VULKAN_ENABLE_EXTENSION_NAME] = nullptr;
#ifdef OPENXR_INCLUDE_OPENGL
	} else if (video_driver == godot::OS::VIDEO_DRIVER_OPENGL_3) {
#ifdef ANDROID
	request_extensions[XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME] = nullptr;
#else
	request_extensions[XR_KHR_OPENGL_ENABLE_EXTENSION_NAME] = nullptr;
#endif
#endif
	} else {
		UtilityFunctions::printerr("Unsupported video driver.");
		return false;
	}

#ifdef ANDROID
	request_extensions[XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME] = nullptr;
#endif

	// If we have these, we use them, if not we skip related logic..
	request_extensions[XR_MND_BALL_ON_STICK_EXTENSION_NAME] = &monado_stick_on_ball_ext;

	for (auto &requested_extension : request_extensions) {
		if (!isExtensionSupported(requested_extension.first, extensionProperties, extensionCount)) {
			if (requested_extension.second == nullptr) {
				UtilityFunctions::printerr("OpenXR Runtime does not support extension ", requested_extension.first);
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

		// FIXME length is defined but not implemented, fix upstream, for now we cheat, this will fail if upper characters are used, we'll loose part of the name
		// int32_t len = name_cs.length();
		int32_t len = (int32_t) project_name.length();
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

	UtilityFunctions::print("Running on OpenXR runtime: ",
			instanceProps.runtimeName,
			" ",
			XR_VERSION_MAJOR(instanceProps.runtimeVersion),
			".",
			XR_VERSION_MINOR(instanceProps.runtimeVersion),
			".",
			XR_VERSION_PATCH(instanceProps.runtimeVersion));

	if (strcmp(instanceProps.runtimeName, "SteamVR/OpenXR") == 0) {
#ifdef WIN32
		// not applicable
#elif ANDROID
		// not applicable
#elif __linux__
		UtilityFunctions::print("Running on Linux, using SteamVR workaround for issue https://github.com/ValveSoftware/SteamVR-for-Linux/issues/421");
#endif
		is_steamvr = true;
	}

	return true;
}

bool OpenXRApi::initialiseSession() {
	XrResult result;

#ifdef DEBUG
	UtilityFunctions::print("OpenXR initialiseSession");
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

	XrSystemProperties systemProperties = {
		.type = XR_TYPE_SYSTEM_PROPERTIES,
		.next = nullptr,
		.graphicsProperties = { 0 },
		.trackingProperties = { 0 },
	};

	void **property_pointer = &systemProperties.next;
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		void **property_next_pointer = wrapper->set_system_properties_and_get_next_pointer(property_pointer);
		if (*property_pointer && property_next_pointer && !*property_next_pointer) {
			property_pointer = property_next_pointer;
		} else {
			// Invalid return values.
			// Reset the value stored by the property_pointer so it can be reused in the next loop.
			*property_pointer = nullptr;
		}
	}

	result = xrGetSystemProperties(instance, systemId, &systemProperties);
	if (!xr_result(result, "Failed to get System properties")) {
		return false;
	}

	system_name = systemProperties.systemName;
	vendor_id = systemProperties.vendorId;

	// TODO consider calling back into our extension wrappers if they need to react on data we just loaded....

	if (!isViewConfigSupported(view_config_type, systemId)) {
		// TODO in stead of erroring out if the set configuration type is unsupported
		// (it may simply be on its default setting)
		// we should change this so it uses the first support type.
		// That does mean checking if WE support it (i.e. we don't support Varjo yet for instance).
		UtilityFunctions::printerr("OpenXR View Configuration not supported!");
		return false;
	}

	result = xrEnumerateViewConfigurationViews(instance, systemId, view_config_type, 0, &view_count, nullptr);
	if (!xr_result(result, "Failed to get view configuration view count!")) {
		return false;
	}

	XrViewConfigurationView *configuration_views = (XrViewConfigurationView *)malloc(sizeof(XrViewConfigurationView) * view_count);
	for (uint32_t i = 0; i < view_count; i++) {
		configuration_views[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
		configuration_views[i].next = nullptr;
	}

	result = xrEnumerateViewConfigurationViews(instance, systemId, view_config_type, view_count, &view_count, configuration_views);
	if (!xr_result(result, "Failed to enumerate view configuration views!")) {
		return false;
	}

	render_target_width = (uint32_t)((float) configuration_views[0].recommendedImageRectWidth * render_target_size_multiplier);
	render_target_width = (std::min)(render_target_width, configuration_views[0].maxImageRectWidth);

	render_target_height = (uint32_t)((float) configuration_views[0].recommendedImageRectHeight * render_target_size_multiplier);
	render_target_height = (std::min)(render_target_height, configuration_views[0].maxImageRectHeight);

	swapchain_sample_count = configuration_views[0].recommendedSwapchainSampleCount;

	free(configuration_views);
	configuration_views = nullptr;

	XrSessionCreateInfo session_create_info = {
		.type = XR_TYPE_SESSION_CREATE_INFO,
		.next = nullptr,
		.createFlags = 0,
		.systemId = systemId
	};

	if (video_driver == godot::OS::VIDEO_DRIVER_VULKAN) {
		RenderingServer *rendering_server = RenderingServer::get_singleton();
		ERR_FAIL_NULL_V(rendering_server, false);
		RenderingDevice *rendering_device = rendering_server->get_rendering_device();
		ERR_FAIL_NULL_V(rendering_device, false);

		if (!check_graphics_requirements_vulkan(systemId)) {
			return false;
		}

#ifdef DEBUG
		UtilityFunctions::print("OpenXR obtain Vulkan graphics device");
#endif
		// Should encapsulate these into an extension
		PFN_xrGetVulkanInstanceExtensionsKHR xrGetVulkanInstanceExtensions = nullptr;
		result = xrGetInstanceProcAddr(instance, "xrGetVulkanInstanceExtensionsKHR", (PFN_xrVoidFunction*)&xrGetVulkanInstanceExtensions);
		if (!xr_result(result, "Failed to get XR vulkan instance extensions function.")) {
			// not fatal probably
			return true;
		}

		PFN_xrGetVulkanDeviceExtensionsKHR xrGetVulkanDeviceExtensions = nullptr;
		result = xrGetInstanceProcAddr(instance, "xrGetVulkanDeviceExtensionsKHR", (PFN_xrVoidFunction*)&xrGetVulkanDeviceExtensions);
		if (!xr_result(result, "Failed to get XR vulkan device extensions function.")) {
			// not fatal probably
			return true;
		}

		PFN_xrGetVulkanGraphicsDeviceKHR xrGetVulkanGraphicsDevice = nullptr;
		result = xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDeviceKHR", (PFN_xrVoidFunction*)&xrGetVulkanGraphicsDevice);
		if (!xr_result(result, "Failed to get XR vulkan graphics device function.")) {
			// not fatal probably
			return true;
		}

		{
			uint32_t size;
			char * buffer = nullptr;

			result = xrGetVulkanInstanceExtensions(instance, systemId, 0, &size, nullptr);

			buffer = (char *)malloc(sizeof(char) * size);
			result = xrGetVulkanInstanceExtensions(instance, systemId, size, &size, buffer);

			UtilityFunctions::print("OpenXR Vulkan required instance extensions: ", buffer);


			free(buffer);
		}

		{
			uint32_t size;
			char * buffer = nullptr;

			result = xrGetVulkanDeviceExtensions(instance, systemId, 0, &size, nullptr);

			buffer = (char *)malloc(sizeof(char) * size);
			result = xrGetVulkanDeviceExtensions(instance, systemId, size, &size, buffer);

			UtilityFunctions::print("OpenXR Vulkan required device extensions: ", buffer);


			free(buffer);
		}

		VkInstance vk_instance = (VkInstance)rendering_device->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_INSTANCE, RID(), 0);
		VkPhysicalDevice vk_physical_device = (VkPhysicalDevice)rendering_device->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_PHYSICAL_DEVICE, RID(), 0);
		VkDevice vk_device = (VkDevice)rendering_device->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_DEVICE, RID(), 0);
		uint32_t vk_queue_family_index = (uint32_t)rendering_device->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_QUEUE_FAMILY_INDEX, RID(), 0);
		uint32_t vk_queue_index = (uint32_t)rendering_device->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_QUEUE, RID(), 0);

		VkPhysicalDevice xr_physical_device = nullptr;

		// We obtain the physical device related to our headset, I'm guessing this should be the same as the one we're already using in Godot (it refers to the graphics card I think)
		// wonder what hell will break loose if they aren't the same, should we check?

		result = xrGetVulkanGraphicsDevice(instance, systemId, vk_instance, &xr_physical_device);
		if (!xr_result(result, "Failed to get XR vulkan graphics device.")) {
			// not fatal probably
			return true;
		}

		if (xr_physical_device != vk_physical_device) {
			UtilityFunctions::print("OpenXR Vulkan graphics device doesn't match between Godot and OpenXR");
		}

		graphics_binding_vulkan.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
		graphics_binding_vulkan.next = nullptr;
		graphics_binding_vulkan.instance = vk_instance; 
		graphics_binding_vulkan.physicalDevice = vk_physical_device;
		graphics_binding_vulkan.device = vk_device;
		graphics_binding_vulkan.queueFamilyIndex = vk_queue_family_index;
		graphics_binding_vulkan.queueIndex = vk_queue_index;

		// TODO look into the vulkan2 bindings

		session_create_info.next = &graphics_binding_vulkan;
#ifdef OPENXR_INCLUDE_OPENGL
	} else if (video_driver == godot::OS::VIDEO_DRIVER_OPENGL_3) {
		if (!check_graphics_requirements_gl(systemId)) {
			return false;
		}

		// FIXME OpenGL doesn't support XR yet in Godot 4, once we get further with this we should fix this up:

#ifdef WIN32

		graphics_binding_gl = XrGraphicsBindingOpenGLWin32KHR{
			.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
			.next = NULL,
		};

		// graphics_binding_gl.hDC = (HDC)os->get_native_handle(OS::WINDOW_VIEW);
		// graphics_binding_gl.hGLRC = (HGLRC)os->get_native_handle(OS::OPENGL_CONTEXT);

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

		void *display_handle = nullptr; // (void *)os->get_native_handle(OS::DISPLAY_HANDLE);
		void *glxcontext_handle = nullptr; // (void *)os->get_native_handle(OS::OPENGL_CONTEXT);
		void *glxdrawable_handle = nullptr; // (void *)os->get_native_handle(OS::WINDOW_HANDLE);

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

		session_create_info.next = &graphics_binding_gl;
#endif // OPENXR_INCLUDE_OPENGL
	} else {

		UtilityFunctions::printerr("Unsupported video driver.");
		return false;
	}

#ifdef DEBUG
	UtilityFunctions::print("OpenXR Create Session");
#endif

	result = xrCreateSession(instance, &session_create_info, &session);
	if (!xr_result(result, "Failed to create session")) {
		return false;
	}

	return true;
}

bool OpenXRApi::set_render_target_size_multiplier(float multiplier) {
	if (is_initialised()) {
		UtilityFunctions::printerr("Setting the render target size multiplier is only allowed prior to initialization.");
		return false;
	} else {
		if (multiplier <= 0) {
			UtilityFunctions::printerr("Only positive values greater than 0 are supported.");
			return false;
		}

		this->render_target_size_multiplier = multiplier;
		return true;
	}
}

bool OpenXRApi::initialiseSpaces() {
	XrResult result;

#ifdef DEBUG
	UtilityFunctions::print("OpenXR initialiseSpaces");
#endif

	XrPosef identityPose = {
		.orientation = { .x = 0, .y = 0, .z = 0, .w = 1.0 },
		.position = { .x = 0, .y = 0, .z = 0 }
	};

	{
		// most runtimes will support local and stage
		if (!isReferenceSpaceSupported(play_space_type)) {
			UtilityFunctions::print("OpenXR runtime does not support play space type {0}!", play_space_type);
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
			UtilityFunctions::printerr("OpenXR runtime does not support view space!");
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
	UtilityFunctions::print("OpenXR initialiseSwapChains");
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
		UtilityFunctions::printerr("OpenXR Couldn't allocate memory for swap chain formats");
		return false;
	}

	result = xrEnumerateSwapchainFormats(session, swapchainFormatCount, &swapchainFormatCount, swapchainFormats);
	if (!xr_result(result, "Failed to enumerate swapchain formats")) {
		free(swapchainFormats);
		return false;
	}

	UtilityFunctions::print("Selecting OpenXR Swapchain Format");

	for (uint64_t i = 0; i < swapchainFormatCount; i++) {
		UtilityFunctions::print("- Found {0}\n", swapchainFormats[i]);
	}

	// int64_t swapchainFormatToUse = swapchainFormats[0];
	int64_t swapchainFormatToUse = 0;

	keep_3d_linear = true; // This will only work correctly for GLES2 from Godot 3.4 onwards

	std::vector<int64_t> usable_formats;
	if (video_driver == OS::VIDEO_DRIVER_VULKAN) {
		usable_formats.push_back(VK_FORMAT_R8G8B8A8_UINT);
		usable_formats.push_back(VK_FORMAT_B8G8R8A8_UINT);
#ifdef OPENXR_INCLUDE_OPENGL
	} else (video_driver == OS::VIDEO_DRIVER_OPENGL_3) {
#ifdef WIN32
		usable_formats.push_back(GL_RGBA8);
#elif ANDROID
		usable_formats.push_back(GL_RGBA8);
#else
		usable_formats.push_back(GL_RGBA8_EXT);
#endif
#endif // OPENXR_INCLUDE_OPENGL
	}

	for (auto usable_format : usable_formats) {
		for (uint64_t i = 0; i < swapchainFormatCount && swapchainFormatToUse == 0; i++) {
			if (swapchainFormats[i] == usable_format) {
				swapchainFormatToUse = swapchainFormats[i];
			}
		}

		if (swapchainFormatToUse != 0) {
			break;
		}	
	}

	// Couldn't find any we want? use the first one.
	// If this is a RGBA16F texture OpenXR on Steam atleast expects linear color space and we'll end up with a too bright display
	if (swapchainFormatToUse == 0) {
		swapchainFormatToUse = swapchainFormats[0];
		UtilityFunctions::print("OpenXR Couldn't find prefered swapchain format, using {0}", swapchainFormatToUse);
	}

	free(swapchainFormats);

	// again Microsoft wants these in order!
	XrSwapchainCreateInfo swapchainCreateInfo = {
		.type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
		.next = nullptr,
		.createFlags = 0,
		.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
		.format = swapchainFormatToUse,
		.sampleCount = swapchain_sample_count, // 1,
		.width = render_target_width,
		.height = render_target_height,
		.faceCount = 1,
		.arraySize = view_count, // use arrays 
		.mipCount = 1,
	};

	void **swapchain_create_info_pointer = const_cast<void **>(&swapchainCreateInfo.next);
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		void **swapchain_create_info_next_pointer = wrapper->set_swapchain_create_info_and_get_next_pointer(swapchain_create_info_pointer);
		if (*swapchain_create_info_pointer && swapchain_create_info_next_pointer && !*swapchain_create_info_next_pointer) {
			swapchain_create_info_pointer = swapchain_create_info_next_pointer;
		} else {
			// Invalid return values.
			// Reset the value stored by the swapchain_create_info_pointer so it can be reused in the next loop.
			*swapchain_create_info_pointer = nullptr;
		}
	}

	result = xrCreateSwapchain(session, &swapchainCreateInfo, &swapchain);
	if (!xr_result(result, "Failed to create swapchain!")) {
		return false;
	}

	result = xrEnumerateSwapchainImages(swapchain, 0, &swapchainLength, nullptr);
	if (!xr_result(result, "Failed to enumerate swapchains")) {
		return false;
	}

	images_rids = (RID *)malloc(sizeof(RID) * swapchainLength);
	ERR_FAIL_NULL_V_MSG(images_rids, false, "OpenXR Couldn't allocate memory for swap chain rids");

	if (video_driver == godot::OS::VIDEO_DRIVER_VULKAN) {
		RenderingServer *rendering_server = RenderingServer::get_singleton();
		ERR_FAIL_NULL_V(rendering_server, false);
		RenderingDevice *rendering_device = rendering_server->get_rendering_device();
		ERR_FAIL_NULL_V(rendering_device, false);

		RenderingDevice::DataFormat format;
		if (swapchainFormatToUse == VK_FORMAT_R8G8B8A8_UINT) {
			format = RenderingDevice::DATA_FORMAT_R8G8B8A8_UINT;
		} else if (swapchainFormatToUse == VK_FORMAT_B8G8R8A8_UINT) {
			// TODO if we're using this we should probably change the swizzles as well which we currently do not support!
			// (or handle this within Godot)
			format = RenderingDevice::DATA_FORMAT_B8G8R8A8_UINT;
		}

		RenderingDevice::TextureSamples samples = RenderingDevice::TEXTURE_SAMPLES_1;
		uint64_t usage_flags = RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT | RenderingDevice::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;

		images_vulkan = (XrSwapchainImageVulkanKHR *)malloc(sizeof(XrSwapchainImageVulkanKHR) * swapchainLength);
		ERR_FAIL_NULL_V_MSG(images_vulkan, false, "OpenXR Couldn't allocate memory for swap chain image");

		for (uint64_t j = 0; j < swapchainLength; j++) {
			images_vulkan[j].next = nullptr;
		}

		result = xrEnumerateSwapchainImages(swapchain, swapchainLength, &swapchainLength, (XrSwapchainImageBaseHeader *)images_vulkan);
		if (!xr_result(result, "Failed to enumerate swapchain images")) {
			return false;
		}

		for (uint64_t j = 0; j < swapchainLength; j++) {
			images_rids[j] = rendering_device->texture_create_from_extension(
				RenderingDevice::TEXTURE_TYPE_2D_ARRAY,
				format,
				samples,
				usage_flags,
				(uint64_t) images_vulkan[j].image,
				render_target_width,
				render_target_height,
				1,
				view_count
			);
		}
#ifdef OPENXR_INCLUDE_OPENGL
	} else if (video_driver == godot::OS::VIDEO_DRIVER_VULKAN) {
	#ifdef ANDROID
		images_gl = (XrSwapchainImageOpenGLESKHR *)malloc(sizeof(XrSwapchainImageOpenGLESKHR) * swapchainLength);
	#else
		images_gl = (XrSwapchainImageOpenGLKHR *)malloc(sizeof(XrSwapchainImageOpenGLKHR) * swapchainLength);
	#endif
		ERR_FAIL_NULL_V_MSG(images_gl, false, "OpenXR Couldn't allocate memory for swap chain image");

		for (uint64_t j = 0; j < swapchainLength; j++) {
	#ifdef ANDROID
			images_gl[j].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
	#else
			images_gl[j].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
	#endif
			images_gl[j].next = nullptr;
		}

		result = xrEnumerateSwapchainImages(swapchain, swapchainLength, &swapchainLength, (XrSwapchainImageBaseHeader *)images_gl);
		if (!xr_result(result, "Failed to enumerate swapchain images")) {
			return false;
		}
#endif // OPENXR_INCLUDE_OPENGL
	} else {
		// should have already exited up above
	}

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
	projectionLayer->next = nullptr;
	projectionLayer->layerFlags = XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
	projectionLayer->space = play_space;
	projectionLayer->viewCount = view_count;
	projectionLayer->views = nullptr;

	frameState.type = XR_TYPE_FRAME_STATE;
	frameState.next = NULL;

	views = (XrView *)malloc(sizeof(XrView) * view_count);
	ERR_FAIL_NULL_V_MSG(views, false, "OpenXR Couldn't allocate memory for views");

	projection_views = (XrCompositionLayerProjectionView *)malloc(sizeof(XrCompositionLayerProjectionView) * view_count);
	ERR_FAIL_NULL_V_MSG(projection_views, false, "OpenXR Couldn't allocate memory for projection views");

	for (uint32_t i = 0; i < view_count; i++) {
		views[i].type = XR_TYPE_VIEW;
		views[i].next = NULL;

		projection_views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
		projection_views[i].next = NULL;
		projection_views[i].subImage.swapchain = swapchain;
		projection_views[i].subImage.imageArrayIndex = i;
		projection_views[i].subImage.imageRect.offset.x = 0;
		projection_views[i].subImage.imageRect.offset.y = 0;
		projection_views[i].subImage.imageRect.extent.width = render_target_width;
		projection_views[i].subImage.imageRect.extent.height = render_target_height;
	};

	return true;
}

void OpenXRApi::cleanupSwapChains() {
	if (projection_views != nullptr) {
		free(projection_views);
		projection_views = nullptr;
	}
	if (images_rids != nullptr) {
		for(uint32_t i = 0; i < swapchainLength; i++) {
			// TODO loop through them here, need to free the RID in Godot.

		}

		free(images_rids);
		images_rids = nullptr;
	}
	if (images_vulkan != nullptr) {
		free(images_vulkan);
		images_vulkan = nullptr;
	}
#ifdef OPENXR_INCLUDE_OPENGL
	if (images_gl != nullptr) {
		free(images_gl);
		images_gl = nullptr;
	}
#endif
	if (projectionLayer != nullptr) {
		free(projectionLayer);
		projectionLayer = nullptr;
	}
	if (views != nullptr) {
		free(views);
		views = nullptr;
	}

	swapchainLength = 0;
}

bool OpenXRApi::loadActionSets() {
#ifdef DEBUG
	UtilityFunctions::print("OpenXR loadActionSets");
#endif

	parse_action_sets(action_sets_json);
	parse_interaction_profiles(interaction_profiles_json);

	return true;
}

bool OpenXRApi::bindActionSets() {
#ifdef DEBUG
	UtilityFunctions::print("OpenXR bindActionSets");
#endif

	// finally attach our action sets, that locks everything in place
	for (uint64_t i = 0; i < action_sets.size(); i++) {
		ActionSet *action_set = action_sets[i];

		if (!action_set->attach()) {
			// Just report this
			UtilityFunctions::print("Couldn't attach action set {0}", action_set->get_name());
		} else {
			UtilityFunctions::print("Attached action set {0}", action_set->get_name());
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
			UtilityFunctions::print("OpenXR found internal action {0}", default_actions[i].name);
		} else {
			UtilityFunctions::print("OpenXR didn't find internal action {0}", default_actions[i].name);
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
			// TODO this needs to become a reference to Ref<XRPositionalTracker>
			// xr_api->godot_arvr_remove_controller(inputmaps[i].godot_controller);
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
		UtilityFunctions::print("Initialize called when interface is already initialized.");
#endif
		return true;
	}

	// get our video driver setting from Godot.
	OS *os = OS::get_singleton();

	// until the GLES driver becomes available againt this setting is defunct
	video_driver = godot::OS::VIDEO_DRIVER_VULKAN; // os->get_current_video_driver();

	if (video_driver == OS::VIDEO_DRIVER_VULKAN) {
		// TODO do we init volk? or can we do without and just need it for our defines?
		// I don't think we actually call any Vulkan methods

#ifdef OPENXR_INCLUDE_OPENGL
	} else if (video_driver == OS::VIDEO_DRIVER_OPENGL_3) {
#ifdef WIN32
	if (!gladLoadGL()) {
		UtilityFunctions::printerr("OpenXR Failed to initialize GLAD");
		return false;
	}
#endif
#endif
	} else {
		UtilityFunctions::printerr("This video driver is not supported");
		return false;
	}

	if (!initialiseInstance()) {
		// cleanup and exit
		uninitialize();
		return false;
	}

	// Propagate the callback to the registered extension wrappers.
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_instance_initialized(instance);
	}

	if (!initialiseSession()) {
		// cleanup and exit
		uninitialize();
		return false;
	}

	// Propagate the callback to the registered extension wrappers.
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_session_initialized(session);
	}

	if (!loadActionSets()) {
		// cleanup and exit
		uninitialize();
		return false;
	}

	// We've made it!
	initialised = true;

	register_plugin_signals();

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
	cleanupSwapChains();
	cleanupSpaces();

	// cleanup our session and instance
	if (session != XR_NULL_HANDLE) {
		for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
			wrapper->on_session_destroyed();
		}

		xrDestroySession(session);
		session = XR_NULL_HANDLE;
	}
	if (instance != XR_NULL_HANDLE) {
		for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
			wrapper->on_instance_destroyed();
		}

		xrDestroyInstance(instance);
		instance = XR_NULL_HANDLE;
	}
	enabled_extensions.clear();

	// reset a bunch of things
	state = XR_SESSION_STATE_UNKNOWN;
	view_pose_valid = false;
	head_pose_valid = false;
	monado_stick_on_ball_ext = false;
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
	UtilityFunctions::print("On state idle");
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_idle();
	}
	return true;
}

bool OpenXRApi::on_state_ready() {
	UtilityFunctions::print("On state ready");
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

	bindActionSets();

	running = true;

	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_ready();
	}

	emit_plugin_signal(SIGNAL_SESSION_BEGUN);

#ifdef ANDROID
	OpenXRPluginWrapper::on_session_begun();
#endif

	return true;
}

bool OpenXRApi::on_state_synchronized() {
	UtilityFunctions::print("On state synchronized");
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_synchronized();
	}
	return true;
}

bool OpenXRApi::on_state_visible() {
	UtilityFunctions::print("On state visible");
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_visible();
	}

	emit_plugin_signal(SIGNAL_VISIBLE_STATE);

#ifdef ANDROID
	OpenXRPluginWrapper::on_focus_lost();
#endif
	return true;
}

bool OpenXRApi::on_state_focused() {
	UtilityFunctions::print("On state focused");
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_focused();
	}

	emit_plugin_signal(SIGNAL_FOCUSED_STATE);

#ifdef ANDROID
	OpenXRPluginWrapper::on_focus_gained();
#endif
	return true;
}

bool OpenXRApi::on_state_stopping() {
	UtilityFunctions::print("On state stopping");

	emit_plugin_signal(SIGNAL_SESSION_ENDING);

#ifdef ANDROID
	OpenXRPluginWrapper::on_session_ending();
#endif

	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_stopping();
	}

	if (running) {
		XrResult result = xrEndSession(session);
		xr_result(result, "Failed to end session!");
		running = false;
	}

	// need to cleanup various things which would otherwise be re-allocated if we have a state change back to ready
	// note that cleaning up our action sets will invalidate many of the OpenXR nodes so we need to improve that as well.
	unbindActionSets();
	cleanupSwapChains();
	cleanupSpaces();

	return true;
}

bool OpenXRApi::on_state_loss_pending() {
	UtilityFunctions::print("On state loss pending");
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_loss_pending();
	}
	uninitialize();
	return true;
}

bool OpenXRApi::on_state_exiting() {
	// we may want to trigger a signal back to the application to tell it, it should quit.
	UtilityFunctions::print("On state exiting");
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_exiting();
	}
	uninitialize();
	return true;
}

bool OpenXRApi::is_initialised() {
	return initialised;
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
		UtilityFunctions::print("OpenXR can't change form factor once OpenXR is initialised.");
		return;
	} else if (p_form_factor > (XrFormFactor)0 && p_form_factor <= (XrFormFactor)2) {
		form_factor = p_form_factor;
		return;
	} else {
		UtilityFunctions::print("OpenXR form factor out of bounds");
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
		UtilityFunctions::print("OpenXR can't change the action sets once OpenXR is initialised.");
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
		UtilityFunctions::print("OpenXR can't change the interaction profiles once OpenXR is initialised.");
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

	// Just in case clean up any action sets we've currently got loaded, it should already be cleared
	cleanupActionSets();

	if (instance == XR_NULL_HANDLE) {
		UtilityFunctions::print("OpenXR can't parse the action sets before OpenXR is initialised.");
		return false;
	}

	/* TODO make this work again! or more likely we'll rewrite this to new resource classes

	JSON *json_parser = JSON::get_singleton();
	Ref<JSONParseResult> parse_result = json_parser->parse(p_json);
	if (parse_result->get_error() != Error::OK) {
		UtilityFunctions::print("Couldn't parse action set JSON {0}", parse_result->get_error_string());
		return false;
	}

	Variant json = parse_result->get_result();
	if (json.get_type() != Variant::ARRAY) {
		UtilityFunctions::print("JSON is not formatted correctly");
		return false;
	}

	Array asets = json;
	for (int i = 0; i < asets.size(); i++) {
		if (asets[i].get_type() != Variant::DICTIONARY) {
			UtilityFunctions::print("JSON is not formatted correctly");
			return false;
		}

		Dictionary action_set = asets[i];
		String action_set_name = action_set["name"];
		String localised_name = action_set["localised_name"];
		int priority = action_set["priority"];

		// UtilityFunctions::print("New action set {0} - {1} ({2})", action_set_name, localised_name, priority);

		ActionSet *new_action_set = get_action_set(action_set_name);
		if (new_action_set == NULL) {
			new_action_set = new ActionSet(this, action_set_name, localised_name, priority);
			if (new_action_set == NULL) {
				UtilityFunctions::print("Couldn't create action set {0}", action_set_name);
				continue;
			}
			action_sets.push_back(new_action_set);
		}

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
				UtilityFunctions::print("Unknown action type {0} for action {1}", type, name);
				continue;
			}

			// UtilityFunctions::print("New action {0} - {1} ({2}: {3})", name, localised_name, action_type, type);

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
				UtilityFunctions::print("Couldn't create action {0}", name);

				continue;
			}
		}
	}
	*/

	return true;
}

bool OpenXRApi::parse_interaction_profiles(const godot::String &p_json) {
	// We can push our interaction profiles directly to OpenXR. No need to keep them in memory.

	if (instance == XR_NULL_HANDLE) {
		UtilityFunctions::print("OpenXR can't parse the interaction profiles before OpenXR is initialised.");
		return false;
	}

	/* TODO make this work again! or more likely we'll rewrite this to new resource classes

	JSON *json_parser = JSON::get_singleton();
	Ref<JSONParseResult> parse_result = json_parser->parse(p_json);
	if (parse_result->get_error() != Error::OK) {
		UtilityFunctions::print("Couldn't parse interaction profile JSON {0}", parse_result->get_error_string());
		return false;
	}

	Variant json = parse_result->get_result();
	if (json.get_type() != Variant::ARRAY) {
		UtilityFunctions::print("JSON is not formatted correctly");
		return false;
	}

	Array interaction_profiles = json;
	for (int i = 0; i < interaction_profiles.size(); i++) {
		if (interaction_profiles[i].get_type() != Variant::DICTIONARY) {
			UtilityFunctions::print("JSON is not formatted correctly");
			return false;
		}

		Dictionary profile = interaction_profiles[i];
		String path_string = profile["path"];

		// UtilityFunctions::print("Interaction profile {0}", path_string);

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
				UtilityFunctions::print("OpenXR Couldn't find set {0}", action_set_name);
				continue;
			}
			Action *action = action_set->get_action(action_name.utf8().get_data());
			if (action == NULL) {
				UtilityFunctions::print("OpenXR Couldn't find action {0}", action);
				continue;
			}
			XrAction xr_action = action->get_action();
			if (xr_action == XR_NULL_HANDLE) {
				UtilityFunctions::print("OpenXR Missing XrAction for {0}", action);
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

				// UtilityFunctions::print(" - Binding {0}/{1} - {2}", action_set_name, action_name, io_path_str);
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

	*/

	return true;
}

bool OpenXRApi::check_graphics_requirements_vulkan(XrSystemId system_id) {
	XrGraphicsRequirementsVulkanKHR vulkan_reqs = {
		.type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR,
		.next = nullptr
	};

	PFN_xrGetVulkanGraphicsRequirementsKHR pfnGetVulkanGraphicsRequirementsKHR = nullptr;
	XrResult result = xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&pfnGetVulkanGraphicsRequirementsKHR);

	if (!xr_result(result, "Failed to get xrGetVulkanGraphicsRequirementsKHR fp!")) {
		return false;
	}

	result = pfnGetVulkanGraphicsRequirementsKHR(instance, system_id, &vulkan_reqs);
	if (!xr_result(result, "Failed to get Vulkan graphics requirements!")) {
		return false;
	}

	XrVersion desired_vulkan_version = XR_MAKE_VERSION(1, 2, 0);
	if (desired_vulkan_version > vulkan_reqs.maxApiVersionSupported || desired_vulkan_version < vulkan_reqs.minApiVersionSupported) {
		UtilityFunctions::print(
				"OpenXR Runtime only supports Vulkan version ",
				XR_VERSION_MAJOR(vulkan_reqs.minApiVersionSupported),".", XR_VERSION_MINOR(vulkan_reqs.minApiVersionSupported),
				" ", XR_VERSION_MAJOR(vulkan_reqs.maxApiVersionSupported), ".", XR_VERSION_MINOR(vulkan_reqs.maxApiVersionSupported), "!");
		// it might still work
		return true;
	}
	return true;
}

#ifdef OPENXR_INCLUDE_OPENGL
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
		UtilityFunctions::print(
				"OpenXR Runtime only supports OpenGL version {0}.{1} - {2}.{3}!", XR_VERSION_MAJOR(opengl_reqs.minApiVersionSupported), XR_VERSION_MINOR(opengl_reqs.minApiVersionSupported), XR_VERSION_MAJOR(opengl_reqs.maxApiVersionSupported), XR_VERSION_MINOR(opengl_reqs.maxApiVersionSupported));
		// it might still work
		return true;
	}
	return true;
}
#endif

XrResult OpenXRApi::acquire_image() {
	XrResult result;
	XrSwapchainImageAcquireInfo swapchainImageAcquireInfo = {
		.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, .next = nullptr
	};
	result = xrAcquireSwapchainImage(swapchain, &swapchainImageAcquireInfo, &buffer_index);
	if (!xr_result(result, "failed to acquire swapchain image!")) {
		return result;
	}

	XrSwapchainImageWaitInfo swapchainImageWaitInfo = {
		.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
		.next = nullptr,
		.timeout = 17000000, /* timeout in nanoseconds */
	};
	result = xrWaitSwapchainImage(swapchain, &swapchainImageWaitInfo);
	if (!xr_result(result, "failed to wait for swapchain image!")) {
		return result;
	}
	return XR_SUCCESS;
}

void OpenXRApi::render_openxr(const RID &p_render_target) {
	if (!initialised) {
		return;
	}

	// printf("Render texture %d\n", texid);
	XrResult result;

	// TODO: save resources don't react on rendering if we're not running (session hasn't begun or has ended)
	if (!running)
		return;

	// must have valid view pose for projection_views[eye].pose to submit layer
	if (!frameState.shouldRender || !view_pose_valid) {
		// external texture support should always be available in Godot 4 once we implement it fully...

		/* TODO uncomment this once we implement external texture support
			XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
				.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
				.next = nullptr
			};
			result = xrReleaseSwapchainImage(swapchain, &swapchainImageReleaseInfo);
			if (!xr_result(result, "failed to release swapchain image!")) {
				return;
			}
		*/

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

		// neither eye is rendered
		return;
	}

	if (true) { // remove this once we implement external texture support
		result = acquire_image();
		if (!xr_result(result, "failed to acquire swapchain image!")) {
			return;
		}


		if (video_driver == OS::VIDEO_DRIVER_VULKAN) {
			// TODO complete this, copy the buffers, note that we're now having both eyes rendered to an array texture

		}
	}

	XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
		.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
		.next = nullptr
	};
	result = xrReleaseSwapchainImage(swapchain, &swapchainImageReleaseInfo);
	if (!xr_result(result, "failed to release swapchain image!")) {
		return;
	}

	// QUESTION why are we doing this here? need to find out...
	for (uint32_t eye = 0; eye < view_count; eye++) {
		projection_views[eye].fov = views[eye].fov;
		projection_views[eye].pose = views[eye].pose;
	}

	projectionLayer->views = projection_views;

	std::vector<const XrCompositionLayerBaseHeader *> layers_list;

	// Add composition layers from providers
	for (XRCompositionLayerProvider *provider : composition_layer_providers) {
		XrCompositionLayerBaseHeader *layer = provider->get_composition_layer();
		if (layer) {
			layers_list.push_back(layer);
		}
	}

	layers_list.push_back((const XrCompositionLayerBaseHeader *)projectionLayer);

	projectionLayer->layerFlags = layers_list.size() > 1 ? XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT : XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;

	XrFrameEndInfo frameEndInfo = {
		.type = XR_TYPE_FRAME_END_INFO,
		.next = nullptr,
		.displayTime = frameState.predictedDisplayTime,
		.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
		.layerCount = static_cast<uint32_t>(layers_list.size()),
		.layers = layers_list.data(),
	};

	result = xrEndFrame(session, &frameEndInfo);
	if (!xr_result(result, "failed to end frame!")) {
		return;
	}

#ifdef WIN32
	// not applicable
#elif ANDROID
	// not applicable
#elif __linux__
#ifdef OPENXR_INCLUDE_OPENGL
	// TODO: should not be necessary, but is for SteamVR since 1.16.x
	if (video_driver == OS::VIDEO_DRIVER_OPENGL_3) {
		if (is_steamvr) {
			glXMakeCurrent(graphics_binding_gl.xDisplay, graphics_binding_gl.glxDrawable, graphics_binding_gl.glxContext);
		}
	}
#endif
#endif
}

void OpenXRApi::fill_projection_matrix(int eye, double p_z_near, double p_z_far, double *p_projection) {
	XrMatrix4x4f matrix;

	if (!initialised || !running) {
		/* CameraMatrix is not available here...
		CameraMatrix *cm = (CameraMatrix *)p_projection;

		cm->set_perspective(60.0, 1.0, p_z_near, p_z_far, false);
		*/

		for (int r = 0; r < 4; r++) {
			for (int c = 0; c < 4; c++) {
				p_projection[r * 4 + c] = r == c ? 1.0 : 0.0;
			}
		}

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

	XrMatrix4x4f_CreateProjectionFov(&matrix, GRAPHICS_OPENGL, views[eye].fov, (float) p_z_near, (float) p_z_far);

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

	// UtilityFunctions::print("Synching {0} active action sets", active_sets.size());

	XrActionsSyncInfo syncInfo = {
		.type = XR_TYPE_ACTIONS_SYNC_INFO,
		.countActiveActionSets = (uint32_t)active_sets.size(),
		.activeActionSets = active_sets.data()
	};

	result = xrSyncActions(session, &syncInfo);
	xr_result(result, "failed to sync actions!");

	// UtilityFunctions::print("Synched");

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

	const float ws = (float) XRServer::get_singleton()->get_world_scale();

#ifdef DISABLED
	// TODO redo this code to use XRPositionalTracker

	for (uint64_t i = 0; i < USER_INPUT_MAX; i++) {
		XrPath input_path = inputmaps[i].toplevel_path;
		if (input_path == XR_NULL_PATH) {
			// no path, skip this
			// UtilityFunctions::print("Skipping {0}", inputmaps[i].name);
		} else {
			bool is_active = false;

			// UtilityFunctions::print("Checking {0}", inputmaps[i].name);

			// If our aim pose is active, our controller is active
			// note, if the user has removed this action then our old controller approach becomes defunct
			if (default_actions[ACTION_AIM_POSE].action != NULL) {
				is_active = default_actions[ACTION_AIM_POSE].action->is_pose_active(input_path);
			}

			if (is_active) {
				if (inputmaps[i].godot_controller == -1) {
					// hate using const_cast here but godot_arvr_add_controller should have it's parameter defined as const, it doesn't change it...
					inputmaps[i].godot_controller = arvr_api->godot_arvr_add_controller(const_cast<char *>(inputmaps[i].name), (godot_int)i + 1, true, true);

					UtilityFunctions::print("OpenXR mapped {0} to {1}", inputmaps[i].name, inputmaps[i].godot_controller);
				}

				// copy for readability
				int godot_controller = inputmaps[i].godot_controller;

				// Start with our pose, we put our ARVRController on our aim pose (may need to change this to our grip pose...)
				godot_transform controller_transform;
				Transform3D *t = (Transform3D *)&controller_transform;
				*t = default_actions[ACTION_AIM_POSE].action->get_as_pose(input_path, ws);

				arvr_api->godot_arvr_set_controller_transform(godot_controller, &controller_transform, true, true);

				// OK, so OpenXR will tell us if the value has changed and we could skip sending our value
				// but Godot also checks it so... just let Godot do it

				// Button and axis are hardcoded..
				// Axis
				if (default_actions[ACTION_FRONT_TRIGGER].action != NULL) {
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 2, default_actions[ACTION_FRONT_TRIGGER].action->get_as_float(input_path), false); // 0.0 -> 1.0
				}
				if (default_actions[ACTION_SIDE_TRIGGER].action != NULL) {
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 4, default_actions[ACTION_SIDE_TRIGGER].action->get_as_float(input_path), false); // 0.0 -> 1.0
				}
				if (default_actions[ACTION_PRIMARY].action != NULL) {
					Vector2 v = default_actions[ACTION_PRIMARY].action->get_as_vector(input_path);
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 0, v.x, true); // -1.0 -> 1.0
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 1, v.y, true); // -1.0 -> 1.0
				}
				if (default_actions[ACTION_SECONDARY].action != NULL) {
					Vector2 v = default_actions[ACTION_SECONDARY].action->get_as_vector(input_path);
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 6, v.x, true); // -1.0 -> 1.0
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 7, v.y, true); // -1.0 -> 1.0
				}
				// Buttons
				if (default_actions[ACTION_AX_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 7, default_actions[ACTION_AX_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_BY_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 1, default_actions[ACTION_BY_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_AX_TOUCH].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 5, default_actions[ACTION_AX_TOUCH].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_BY_TOUCH].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 6, default_actions[ACTION_BY_TOUCH].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_MENU_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 3, default_actions[ACTION_MENU_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_SELECT_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 4, default_actions[ACTION_SELECT_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_FRONT_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 15, default_actions[ACTION_FRONT_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_FRONT_TOUCH].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 16, default_actions[ACTION_FRONT_TOUCH].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_SIDE_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 2, default_actions[ACTION_SIDE_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_PRIMARY_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 14, default_actions[ACTION_PRIMARY_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_SECONDARY_BUTTON].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 13, default_actions[ACTION_SECONDARY_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_PRIMARY_TOUCH].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 12, default_actions[ACTION_PRIMARY_TOUCH].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_SECONDARY_TOUCH].action != NULL) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 11, default_actions[ACTION_SECONDARY_TOUCH].action->get_as_bool(input_path));
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
#endif
}

void OpenXRApi::recommended_rendertarget_size(uint32_t *width, uint32_t *height) {
	if (!initialised) {
		*width = 0;
		*height = 0;
	} else {
		*width = render_target_width;
		*height = render_target_height;
	}
}

void OpenXRApi::transform_from_matrix(Transform3D &p_dest, XrMatrix4x4f *matrix, float p_world_scale) {
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
			p_dest.basis.elements[i].coord[j] = m[i][j];
		};
	};

	p_dest.origin.x = -m[3][0] * p_world_scale;
	p_dest.origin.y = -m[3][1] * p_world_scale;
	p_dest.origin.z = -m[3][2] * p_world_scale;
};

bool OpenXRApi::get_view_transform(int eye, float world_scale, Transform3D &transform_for_eye) {
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

	transform_for_eye = transform_from_pose(views[eye].pose, world_scale);

	return true;
}

bool OpenXRApi::get_head_center(float world_scale, Transform3D &transform) {
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
			UtilityFunctions::printerr("OpenXR head space location not valid (check tracking?)");
#ifdef DEBUG
		} else {
			UtilityFunctions::print("OpenVR Head pose is now valid");
#endif
		}
	}

	if (!head_pose_valid) {
		return false;
	}

	transform = transform_from_pose(location.pose, world_scale);

	return true;
}

RID OpenXRApi::get_external_color_texture() {
	if (!initialised) {
		return RID();
	}

	// this won't prevent us from rendering but we won't output to OpenXR
	if (!running || state >= XR_SESSION_STATE_STOPPING)
		return RID();
	
	if (images_rids == nullptr) {
		return RID();
	}

	XrResult result = acquire_image();
	if (!xr_result(result, "failed to acquire swapchain image!")) {
		return RID();
	}

#ifdef WIN32
	// not applicable
#elif ANDROID
	// not applicable
#elif __linux__
#ifdef OPENXR_INCLUDE_OPENGL
	if (video_driver == OS::VIDEO_DRIVER_OPENGL_3) {
		// TODO: should not be necessary, but is for SteamVR since 1.16.x
		if (is_steamvr) {
			glXMakeCurrent(graphics_binding_gl.xDisplay, graphics_binding_gl.glxDrawable, graphics_binding_gl.glxContext);
		}
	}
#endif
#endif

	// process should be called by now but just in case...
	if (state > XR_SESSION_STATE_UNKNOWN) {
		return images_rids[buffer_index];
	}

	return RID();
}

bool OpenXRApi::poll_events() {
	XrEventDataBuffer runtimeEvent = {
		.type = XR_TYPE_EVENT_DATA_BUFFER,
		.next = nullptr
	};

	XrResult pollResult = xrPollEvent(instance, &runtimeEvent);
	while (pollResult == XR_SUCCESS) {
		bool handled = false;
		for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
			handled |= wrapper->on_event_polled(runtimeEvent);
		}
		switch (runtimeEvent.type) {
			case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
				XrEventDataEventsLost *event = (XrEventDataEventsLost *)&runtimeEvent;

				UtilityFunctions::print("OpenXR EVENT: {0} event data lost!", event->lostEventCount);
				// we probably didn't poll fast enough'
			} break;
			case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR: {
				XrEventDataVisibilityMaskChangedKHR *event = (XrEventDataVisibilityMaskChangedKHR *)&runtimeEvent;
				UtilityFunctions::print("OpenXR EVENT: STUB: visibility mask changed");
			} break;
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
				XrEventDataInstanceLossPending *event = (XrEventDataInstanceLossPending *)&runtimeEvent;
				UtilityFunctions::print("OpenXR EVENT: instance loss pending at {0}!", event->lossTime);
				// running = false;
				return false;
			} break;
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				XrEventDataSessionStateChanged *event = (XrEventDataSessionStateChanged *)&runtimeEvent;

				state = event->state;
				if (state >= XR_SESSION_STATE_MAX_ENUM) {
					UtilityFunctions::print("OpenXR EVENT: session state changed to UNKNOWN - {0}", state);
				} else {
					UtilityFunctions::print("OpenXR EVENT: session state changed to {0}", session_states[state]);

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
				UtilityFunctions::print("OpenXR EVENT: reference space type {0} change pending!", event->referenceSpaceType);
				if (event->poseValid) {
					emit_plugin_signal(SIGNAL_POSE_RECENTERED);
				}
			} break;
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
				UtilityFunctions::print("OpenXR EVENT: interaction profile changed!");

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

					// UtilityFunctions::print("Checking {0} ({1})", inputmaps[i].name, (uint64_t)input_path);

					XrResult res = xrGetCurrentInteractionProfile(event->session, input_path, &profile_state);
					if (!xr_result(res, "Failed to get interaction profile for {0}", inputmaps[i].name)) {
						continue;
					}

					XrPath new_profile = profile_state.interactionProfile;
					if (inputmaps[i].active_profile != new_profile) {
						inputmaps[i].active_profile = new_profile;
						if (new_profile == XR_NULL_PATH) {
							UtilityFunctions::print("OpenXR No interaction profile for {0}", inputmaps[i].name);
							continue;
						}

						uint32_t strl;
						char profile_str[XR_MAX_PATH_LENGTH];
						res = xrPathToString(instance, new_profile, XR_MAX_PATH_LENGTH, &strl, profile_str);
						if (!xr_result(res, "Failed to get interaction profile path str for {0}", inputmaps[i].name)) {
							continue;
						}

						UtilityFunctions::print("OpenXR Event: Interaction profile changed for {0}: {1}", inputmaps[i].name, profile_str);
					}
				}

				// TODO: do something
			} break;
			default:
				if (!handled) {
					UtilityFunctions::print(String("OpenXR Unhandled event type ") + String::num(runtimeEvent.type));
				}
				break;
		}

		runtimeEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
		pollResult = xrPollEvent(instance, &runtimeEvent);
	}
	if (pollResult == XR_EVENT_UNAVAILABLE) {
		// processed all events in the queue
		return true;
	} else {
		UtilityFunctions::printerr("OpenXR Failed to poll events!");
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
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_process_openxr();
	}

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
			UtilityFunctions::print("OpenXR View pose became invalid");
#ifdef DEBUG
		} else {
			UtilityFunctions::print("OpenXR View pose became valid");
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

Transform3D OpenXRApi::transform_from_pose(const XrPosef &p_pose, float p_world_scale) {
	Quaternion q(p_pose.orientation.x, p_pose.orientation.y, p_pose.orientation.z, p_pose.orientation.w);
	Basis basis(q);
	Vector3 origin(p_pose.position.x * p_world_scale, p_pose.position.y * p_world_scale, p_pose.position.z * p_world_scale);

	return Transform3D(basis, origin);
}

template <typename T>
Transform3D _transform_from_space_location(OpenXRApi &api, const T &p_location, float p_world_scale) {
	Basis basis;
	Vector3 origin;
	const auto &pose = p_location.pose;
	if (p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
		Quaternion q(pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w);
		basis = Basis(q);
	}
	if (p_location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
		origin = Vector3(pose.position.x, pose.position.y, pose.position.z) * p_world_scale;
	}
	return Transform3D(basis, origin);
}

Transform3D OpenXRApi::transform_from_space_location(const XrSpaceLocation &p_location, float p_world_scale) {
	return _transform_from_space_location(*this, p_location, p_world_scale);
}

Transform3D OpenXRApi::transform_from_space_location(const XrHandJointLocationEXT &p_location, float p_world_scale) {
	return _transform_from_space_location(*this, p_location, p_world_scale);
}
