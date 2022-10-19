////////////////////////////////////////////////////////////////////////////////////////////////
// Helper calls and singleton container for accessing openxr

#include <ARVRServer.hpp>
#include <CameraMatrix.hpp>
#include <JSON.hpp>
#include <JSONParseResult.hpp>
#include <ProjectSettings.hpp>

#include "openxr/OpenXRApi.h"
#include "openxr/include/signals_util.h"
#include "openxr/include/util.h"

#include <algorithm>
#include <cmath>
#include <map>

#ifdef ANDROID
#include <jni/openxr_plugin_wrapper.h>
#endif

#ifndef GL_FRAMEBUFFER_SRGB_EXT
#define GL_FRAMEBUFFER_SRGB_EXT 0x8DB9
#endif

////////////////////////////////////////////////////////////////////////////////
// Default action set configuration

// TODO: it makes sense to include this in source because we'll store any user defined version in Godot scenes
// but there has to be a nicer way to embed it :)

// clang-format off
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
				"type": "pose",
				"name": "palm_pose",
				"localised_name": "Palm Pose",
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
				"action": "palm_pose",
				"paths": [
					"/user/hand/left/input/palm_ext/pose",
					"/user/hand/right/input/palm_ext/pose"
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
	},)==="
	R"===({
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
				"action": "palm_pose",
				"paths": [
					"/user/hand/left/input/palm_ext/pose",
					"/user/hand/right/input/palm_ext/pose"
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
	},)==="
	R"===({
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
				"action": "palm_pose",
				"paths": [
					"/user/hand/left/input/palm_ext/pose",
					"/user/hand/right/input/palm_ext/pose"
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
	},)==="
	R"===({
		"path": "/interaction_profiles/samsung/odyssey_controller",
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
				"action": "palm_pose",
				"paths": [
					"/user/hand/left/input/palm_ext/pose",
					"/user/hand/right/input/palm_ext/pose"
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
	},)==="
	R"===({
		"path": "/interaction_profiles/hp/mixed_reality_controller",
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
				"action": "palm_pose",
				"paths": [
					"/user/hand/left/input/palm_ext/pose",
					"/user/hand/right/input/palm_ext/pose"
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
					"/user/hand/left/input/squeeze/value",
					"/user/hand/right/input/squeeze/value"
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
				"action": "haptic",
				"paths": [
					"/user/hand/left/output/haptic",
					"/user/hand/right/output/haptic"
				]
			},
		],
	},)==="
	R"===({
		"path": "/interaction_profiles/htc/vive_cosmos_controller",
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
				"action": "palm_pose",
				"paths": [
					"/user/hand/left/input/palm_ext/pose",
					"/user/hand/right/input/palm_ext/pose"
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
				"action": "menu_button",
				"paths": [
					"/user/hand/left/input/menu/click",
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
				"action": "primary_touch",
				"paths": [
					"/user/hand/left/input/thumbstick/touch",
					"/user/hand/right/input/thumbstick/touch"
				]
			},
			{
				"set": "godot",
				"action": "secondary_button",
				"paths": [
					"/user/hand/left/input/shoulder/click",
					"/user/hand/right/input/shoulder/click"
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
	},)==="
	R"===({
		"path": "/interaction_profiles/htc/vive_focus3_controller",
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
				"action": "palm_pose",
				"paths": [
					"/user/hand/left/input/palm_ext/pose",
					"/user/hand/right/input/palm_ext/pose"
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
					"/user/hand/left/input/squeeze/click",
					"/user/hand/right/input/squeeze/click"
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
				"action": "menu_button",
				"paths": [
					"/user/hand/left/input/menu/click",
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
					"/user/hand/left/input/thumbrest/touch",
					"/user/hand/right/input/thumbrest/touch"
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
	},)==="
	/* There is a home, back, volume up and down button on the huawei controller that we don't have actions for (yet) */
	R"===({
		"path": "/interaction_profiles/huawei/controller",
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
				"action": "palm_pose",
				"paths": [
					"/user/hand/left/input/palm_ext/pose",
					"/user/hand/right/input/palm_ext/pose"
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
				"action": "front_button",
				"paths": [
					"/user/hand/left/input/trigger/click",
					"/user/hand/right/input/trigger/click"
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
	},)==="
	R"===({
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
				"action": "palm_pose",
				"paths": [
					"/user/hand/left/input/palm_ext/pose",
					"/user/hand/right/input/palm_ext/pose"
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
	},)==="
	R"===({
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
				"action": "palm_pose",
				"paths": [
					"/user/hand/left/input/palm_ext/pose",
					"/user/hand/right/input/palm_ext/pose"
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
// clang-format on

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
	} else {
		// cleanup openxr
		delete singleton;
		singleton = nullptr;
	};
};

OpenXRApi *OpenXRApi::openxr_get_api() {
	if (singleton != nullptr) {
		// increase use count
		singleton->use_count++;
	} else {
		singleton = new OpenXRApi();
		if (singleton == nullptr) {
			Godot::print_error("OpenXR interface creation failed", __FUNCTION__, __FILE__, __LINE__);
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

#ifdef DEBUG
	if (num_output_layers > 0) {
		String layer_names;
		for (uint32_t i = 0; i < num_output_layers; i++) {
			if (i != 0) {
				layer_names = layer_names + ", ";
			}
			layer_names = layer_names + layer_properties[i].layerName;
		}
		Godot::print("OpenXR: Found layer(s) {0}", layer_names);
	} else {
		Godot::print("OpenXR: No layers found");
	}
#endif

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

	// Append the extensions requested by the registered extension wrappers.
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		std::map<const char *, bool *> wrapper_request_extensions = wrapper->get_request_extensions();
		request_extensions.insert(wrapper_request_extensions.begin(), wrapper_request_extensions.end());
	}

#ifdef ANDROID
	request_extensions[XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME] = nullptr;
	request_extensions[XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME] = nullptr;
#else
	request_extensions[XR_KHR_OPENGL_ENABLE_EXTENSION_NAME] = nullptr;
#endif

	// If we have these, we use them, if not we skip related logic..
	request_extensions[XR_MND_BALL_ON_STICK_EXTENSION_NAME] = &monado_stick_on_ball_ext;

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
		Godot::print_error("OpenXR View Configuration not supported!", __FUNCTION__, __FILE__, __LINE__);
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

	render_target_width = configuration_views[0].recommendedImageRectWidth * render_target_size_multiplier;
	render_target_width = (std::min)(render_target_width, configuration_views[0].maxImageRectWidth);

	render_target_height = configuration_views[0].recommendedImageRectHeight * render_target_size_multiplier;
	render_target_height = (std::min)(render_target_height, configuration_views[0].maxImageRectHeight);

	swapchain_sample_count = configuration_views[0].recommendedSwapchainSampleCount;

	free(configuration_views);
	configuration_views = nullptr;

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
		.next = nullptr,
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
		.next = nullptr,
	};

	graphics_binding_gl.display = eglGetCurrentDisplay();
	graphics_binding_gl.config = (EGLConfig)0; // https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/master/src/tests/hello_xr/graphicsplugin_opengles.cpp#L122
	graphics_binding_gl.context = eglGetCurrentContext();
#else
	graphics_binding_gl = (XrGraphicsBindingOpenGLXlibKHR){
		.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR,
		.next = nullptr,
	};

	void *display_handle = (void *)os->get_native_handle(OS::DISPLAY_HANDLE);
	void *glxcontext_handle = (void *)os->get_native_handle(OS::OPENGL_CONTEXT);
	void *glxdrawable_handle = (void *)os->get_native_handle(OS::WINDOW_HANDLE);

	graphics_binding_gl.xDisplay = (Display *)display_handle;
	graphics_binding_gl.glxContext = (GLXContext)glxcontext_handle;
	graphics_binding_gl.glxDrawable = (GLXDrawable)glxdrawable_handle;

	if (graphics_binding_gl.xDisplay == nullptr) {
		Godot::print("OpenXR Failed to get xDisplay from Godot, using XOpenDisplay(nullptr)");
		graphics_binding_gl.xDisplay = XOpenDisplay(nullptr);
	}
	if (graphics_binding_gl.glxContext == nullptr) {
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

	Godot::print("OpenXR Graphics: Display {0}, Context {1} Drawable {2}",
			(uintptr_t)graphics_binding_gl.xDisplay,
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

void OpenXRApi::set_play_space_type(XrReferenceSpaceType p_type) {
	if (is_initialised()) {
		Godot::print_error("Setting the play space type is only allowed prior to initialization.", __FUNCTION__, __FILE__, __LINE__);
	} else {
		play_space_type = p_type;
	}
}

bool OpenXRApi::set_render_target_size_multiplier(float multiplier) {
	if (is_initialised()) {
		Godot::print_error("Setting the render target size multiplier is only allowed prior to initialization.", __FUNCTION__, __FILE__, __LINE__);
		return false;
	} else {
		if (multiplier <= 0) {
			Godot::print_error("Only positive values greater than 0 are supported.", __FUNCTION__, __FILE__, __LINE__);
			return false;
		}

		this->render_target_size_multiplier = multiplier;
		return true;
	}
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

#ifdef DEBUG
	// lets report on what we found...
	Godot::print("OpenXR Supported Swapchain Formats");
	String swapchain_formats;
	for (uint64_t i = 0; i < swapchainFormatCount; i++) {
		if (i != 0) {
			swapchain_formats = swapchain_formats + ", ";
		}
		swapchain_formats = swapchain_formats + get_swapchain_format_name(swapchainFormats[i]);
	}
	Godot::print("OpenXR: Found(s) {0}", swapchain_formats);
#endif

	// With the GLES2 driver we're rendering directly into this buffer with a pipeline that assumes a 32bit uniform buffer (i.e 0.0 - 1.0 = 0 - 255).

	// With the GLES3 driver rendering happens into an 64bit floating point buffer with all rendering happening in linear color space.
	// This buffer is then copied into the texture we supply here during the post process stage where tone mapping, glow, DOF, screenspace reflection and conversion to sRGB is optionally applied.
	// Again we're expected to supply a 32bit uniform buffer

	// Note that OpenXR expects sRGB content if an sRGB buffer is used, else it expects linear, however sRGB buffers don't seem to work properly under GLES2.
	// this is a problem especially on Quest where we have to use a 32bit buffer yet 8bits per color leads to awefull banding when linear color space is used.

	// We start by defining a list of formats we'd like to use from most to least, doesn't help every platform has slightly different names for these...

	std::vector<RequestedSwapchainFormat> requested_swapchain_formats;

#ifdef WIN32
	requested_swapchain_formats.push_back({ GL_SRGB8_ALPHA8, false });
	requested_swapchain_formats.push_back({ GL_RGBA8, true });
#elif ANDROID
	requested_swapchain_formats.push_back({ GL_SRGB8_ALPHA8, false });
	requested_swapchain_formats.push_back({ GL_RGBA8, true });
#else
	requested_swapchain_formats.push_back({ GL_SRGB8_ALPHA8_EXT, false });
	requested_swapchain_formats.push_back({ GL_RGBA8_EXT, true });
#endif

	int64_t swapchain_format_to_use = 0;

	for (uint64_t i = 0; i < requested_swapchain_formats.size() && swapchain_format_to_use == 0; i++) {
		for (uint64_t s = 0; s < swapchainFormatCount && swapchain_format_to_use == 0; s++) {
			if (swapchainFormats[s] == requested_swapchain_formats[i].swapchain_format) {
				swapchain_format_to_use = requested_swapchain_formats[i].swapchain_format;
				keep_3d_linear = requested_swapchain_formats[i].is_linear;
			}
		}
	}

	// Couldn't find any we want? Use the first one. We assume this is in linear color space.
	if (swapchain_format_to_use == 0) {
		swapchain_format_to_use = swapchainFormats[0];
		keep_3d_linear = true;
		Godot::print("OpenXR Couldn't find prefered swapchain format, using {0} in linear color space", get_swapchain_format_name(swapchain_format_to_use));
#ifdef DEBUG
	} else {
		Godot::print("OpenXR Using swapchain format {0} in {1} color space", get_swapchain_format_name(swapchain_format_to_use), keep_3d_linear ? "linear" : "sRGB");
#endif
	}

	if (!keep_3d_linear) {
		// Make sure we keep our data in sRGB by turning linear to sRGB conversion off for the frame buffer. We are supplying data in sRGB.
		glDisable(GL_FRAMEBUFFER_SRGB_EXT);
	}

	free(swapchainFormats);

	swapchains = (XrSwapchain *)malloc(sizeof(XrSwapchain) * view_count);
	if (swapchains == nullptr) {
		Godot::print_error("OpenXR Couldn't allocate memory for swap chains", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	swapchain_acquired = (bool *)malloc(sizeof(bool) * view_count);
	if (swapchain_acquired == nullptr) {
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
		swapchain_acquired[i] = false;

		// again Microsoft wants these in order!
		XrSwapchainCreateInfo swapchainCreateInfo = {
			.type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
			.next = nullptr,
			.createFlags = 0,
			.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
			.format = swapchain_format_to_use,
			.sampleCount = swapchain_sample_count, // 1,
			.width = render_target_width,
			.height = render_target_height,
			.faceCount = 1,
			.arraySize = 1,
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
	projectionLayer->next = nullptr;
	projectionLayer->layerFlags = XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
	projectionLayer->space = play_space;
	projectionLayer->viewCount = view_count;
	projectionLayer->views = nullptr;

	frameState.type = XR_TYPE_FRAME_STATE;
	frameState.next = nullptr;

	views = (XrView *)malloc(sizeof(XrView) * view_count);
	if (views == nullptr) {
		Godot::print_error("OpenXR Couldn't allocate memory for views", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	projection_views = (XrCompositionLayerProjectionView *)malloc(sizeof(XrCompositionLayerProjectionView) * view_count);
	if (projection_views == nullptr) {
		Godot::print_error("OpenXR Couldn't allocate memory for projection views", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	for (uint32_t i = 0; i < view_count; i++) {
		views[i].type = XR_TYPE_VIEW;
		views[i].next = nullptr;

		projection_views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
		projection_views[i].next = nullptr;
		projection_views[i].subImage.swapchain = swapchains[i];
		projection_views[i].subImage.imageArrayIndex = 0;
		projection_views[i].subImage.imageRect.offset.x = 0;
		projection_views[i].subImage.imageRect.offset.y = 0;
		projection_views[i].subImage.imageRect.extent.width = render_target_width;
		projection_views[i].subImage.imageRect.extent.height = render_target_height;
	};

	return true;
}

void OpenXRApi::cleanupSwapChains() {
	if (swapchain_acquired != nullptr) {
		free(swapchain_acquired);
		swapchain_acquired = nullptr;
	}
	if (swapchains != nullptr) {
		for (uint32_t i = 0; i < view_count; i++) {
			if (swapchains[i] != XR_NULL_HANDLE) {
				xrDestroySwapchain(swapchains[i]);
			}
		}
		free(swapchains);
		swapchains = nullptr;
	}
	if (projection_views != nullptr) {
		free(projection_views);
		projection_views = nullptr;
	}
	if (images != nullptr) {
		for (uint32_t i = 0; i < view_count; i++) {
			free(images[i]);
		}
		free(images);
		images = nullptr;
	}
	if (projectionLayer != nullptr) {
		free(projectionLayer);
		projectionLayer = nullptr;
	}
	if (views != nullptr) {
		free(views);
		views = nullptr;
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
		if (default_actions[i].action == nullptr) {
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
		default_actions[i].action = nullptr;
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
	if (buffer_index != nullptr) {
		free(buffer_index);
		buffer_index = nullptr;
	}
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
#ifdef DEBUG
	Godot::print("OpenXR:On state idle");
#endif
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_idle();
	}

	emit_plugin_signal(SIGNAL_SESSION_IDLE);

	return true;
}

bool OpenXRApi::on_state_ready() {
#ifdef DEBUG
	Godot::print("OpenXR: On state ready");
#endif
	XrSessionBeginInfo sessionBeginInfo = {
		.type = XR_TYPE_SESSION_BEGIN_INFO,
		.next = nullptr,
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
#ifdef DEBUG
	Godot::print("OpenXR: On state synchronized");
#endif
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_synchronized();
	}

	emit_plugin_signal(SIGNAL_SESSION_SYNCHRONIZED);

	return true;
}

bool OpenXRApi::on_state_visible() {
#ifdef DEBUG
	Godot::print("OpenXR: On state visible");
#endif
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
#ifdef DEBUG
	Godot::print("OpenXR: On state focused");
#endif
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
#ifdef DEBUG
	Godot::print("OpenXR: On state stopping");
#endif

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
#ifdef DEBUG
	Godot::print("OpenXR: On state loss pending");
#endif
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_loss_pending();
	}
	uninitialize();

	emit_plugin_signal(SIGNAL_SESSION_LOSS_PENDING);

	return true;
}

bool OpenXRApi::on_state_exiting() {
	// we may want to trigger a signal back to the application to tell it, it should quit.
#ifdef DEBUG
	Godot::print("OpenXR: On state exiting");
#endif
	for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
		wrapper->on_state_exiting();
	}
	uninitialize();

	emit_plugin_signal(SIGNAL_SESSION_EXITING);

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

Size2 OpenXRApi::get_play_space_bounds() {
	Size2 ret;

	if (is_initialised()) {
		XrExtent2Df extends;

		XrResult result = xrGetReferenceSpaceBoundsRect(session, play_space_type, &extends);
		if (!xr_result(result, "Couldn't obtain play space bounds!")) {
			return ret;
		}

		ret.width = extends.width;
		ret.height = extends.height;
	}

	return ret;
}

godot::Array OpenXRApi::get_enabled_extensions() const {
	godot::Array arr;

	for (int i = 0; i < enabled_extensions.size(); i++) {
		arr.push_back(String(enabled_extensions[i]));
	}

	return arr;
}

bool OpenXRApi::is_input_map_controller(int p_godot_controller) {
	for (const auto &inputmap : inputmaps) {
		if (inputmap.godot_controller == p_godot_controller) {
			return true;
		}
	}
	return false;
}

TrackingConfidence OpenXRApi::get_controller_tracking_confidence(const int p_godot_controller) const {
	for (const auto &inputmap : inputmaps) {
		if (inputmap.godot_controller == p_godot_controller) {
			return inputmap.tracking_confidence;
		}
	}

	return TRACKING_CONFIDENCE_NONE;
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

	return nullptr;
}

Action *OpenXRApi::get_action(const char *p_name) {
	// Find this action within our action sets (assuming we don't have duplication)
	for (uint64_t i = 0; i < action_sets.size(); i++) {
		Action *action = action_sets[i]->get_action(p_name);
		if (action != nullptr) {
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

		ActionSet *new_action_set = get_action_set(action_set_name);
		if (new_action_set == nullptr) {
			new_action_set = new ActionSet(this, action_set_name, localised_name, priority);
			if (new_action_set == nullptr) {
				Godot::print("Couldn't create action set {0}", action_set_name);
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
				Godot::print("Unknown action type {0} for action {1}", type, name);
				continue;
			}

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
			if (new_action == nullptr) {
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
			if (action_set == nullptr) {
				Godot::print("OpenXR Couldn't find set {0}", action_set_name);
				continue;
			}
			Action *action = action_set->get_action(action_name.utf8().get_data());
			if (action == nullptr) {
				Godot::print("OpenXR Couldn't find action {0}", action_name);
				continue;
			}
			XrAction xr_action = action->get_action();
			if (xr_action == XR_NULL_HANDLE) {
				Godot::print("OpenXR Missing XrAction for {0}", action_name);
				continue;
			}
			for (int p = 0; p < io_paths.size(); p++) {
				String io_path_str = io_paths[p];
				XrPath io_path = XR_NULL_PATH;

				bool is_supported = true;
				for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
					if (!wrapper->path_is_supported(io_path_str)) {
						// Only the extension that controlls optional io paths will return false if the path is not supported.
						is_supported = false;
						break;
					}
				}

				if (!is_supported) {
					// If we include these interaction profiles can be rejected.
					// This kind of makes sense but in our case seeing we're using a fixed action set we will exclude the entries.
					Godot::print_warning(String("OpenXR ") + io_path_str + String(" is not supported by this runtime."), __FUNCTION__, __FILE__, __LINE__);
					continue;
				}

				XrResult res = xrStringToPath(instance, io_path_str.utf8().get_data(), &io_path);
				if (!xr_result(res, "OpenXR couldn't create path for {0}", io_path_str)) {
					continue;
				} else if (io_path == XR_NULL_PATH) {
					Godot::print_warning(String("OpenXR ") + io_path_str + String(" is not supported by this runtime."), __FUNCTION__, __FILE__, __LINE__);
					continue;
				}

				XrActionSuggestedBinding bind = { xr_action, io_path };
				xr_bindings.push_back(bind);
			}
		}

		// update our profile
		const XrInteractionProfileSuggestedBinding suggestedBindings = {
			.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
			.next = nullptr,
			.interactionProfile = interaction_profile_path,
			.countSuggestedBindings = (uint32_t)xr_bindings.size(),
			.suggestedBindings = xr_bindings.data()
		};

		XrResult result = xrSuggestInteractionProfileBindings(instance, &suggestedBindings);
		if (result == XR_ERROR_PATH_UNSUPPORTED) {
			Godot::print_warning(String("OpenXR Interaction profile ") + path_string + String(" is not supported on this runtime"), __FUNCTION__, __FILE__, __LINE__);
		} else if (!xr_result(result, "failed to suggest bindings for {0}", path_string)) {
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
		.next = nullptr
	};

	PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
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
				"OpenXR Runtime only supports OpenGL version {0}.{1} - {2}.{3}! Usually this is not a problem, continuing...",
				XR_VERSION_MAJOR(opengl_reqs.minApiVersionSupported), XR_VERSION_MINOR(opengl_reqs.minApiVersionSupported),
				XR_VERSION_MAJOR(opengl_reqs.maxApiVersionSupported), XR_VERSION_MINOR(opengl_reqs.maxApiVersionSupported));
		// it might still work
		return true;
	}
	return true;
}

XrTime OpenXRApi::get_next_frame_time() const {
	if (!initialised || !running) {
		return 0;
	}

	// xrWaitFrame not run yet
	if (frameState.predictedDisplayTime == 0) {
		return 0;
	}

	// We retrieve our tracking information right before we render.
	// We use the current frames predicted display time while rendering.
	// However when position nodes in our scene, we update this while processing the next frame.
	// We thus need to advance our frame timing by one frame when retreiving this data.
	return frameState.predictedDisplayTime + frameState.predictedDisplayPeriod;
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

	swapchain_acquired[eye] = true;

	return XR_SUCCESS;
}

bool OpenXRApi::release_swapchain(int eye) {
	if (swapchain_acquired[eye]) {
		swapchain_acquired[eye] = false; // mark as false whether we succeed or not...

		XrSwapchainImageReleaseInfo swapchainImageReleaseInfo = {
			.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
			.next = nullptr
		};
		XrResult result = xrReleaseSwapchainImage(swapchains[eye], &swapchainImageReleaseInfo);

		// Workaround for dealing with swapchain not getting released properly after screen recording
		if (result != XR_SUCCESS) {
			swapchain_error = true;
		}

		return xr_result(result, "failed to release swapchain image!");
	} else {
		return XR_SUCCESS;
	}
}

void OpenXRApi::end_frame(uint32_t p_layer_count, const XrCompositionLayerBaseHeader *const *p_layers) {
	// MS wants these in order..
	// submit 0 layers when we shouldn't render
	XrFrameEndInfo frameEndInfo = {
		.type = XR_TYPE_FRAME_END_INFO,
		.next = nullptr,
		.displayTime = frameState.predictedDisplayTime,
		.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
		.layerCount = p_layer_count,
		.layers = p_layers,
	};
	XrResult result = xrEndFrame(session, &frameEndInfo);
	xr_result(result, "failed to end frame!"); // just report the error
}

void OpenXRApi::render_openxr(int eye, uint32_t texid, bool has_external_texture_support) {
	if (!initialised) {
		return;
	}

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
		release_swapchain(eye); // just report the error and ignore

		if (eye == 1) {
			// we must always end our frame, even if we don't have an image to submit...
			end_frame(0, nullptr);
		}

		// neither eye is rendered
		return;
	}

	if (!has_external_texture_support) {
		result = acquire_image(eye);
		if (!xr_result(result, "failed to acquire swapchain image!")) {
			if (eye == 1) {
				// we must always end our frame, even if we don't have an image to submit...
				end_frame(0, nullptr);
			}

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
				render_target_width,
				render_target_height);
		glBindTexture(GL_TEXTURE_2D, 0);
		// printf("Copy godot texture %d into XR texture %d\n", texid,
		// images[eye][bufferIndex].image);
	} else {
		// printf("Godot already rendered into our textures\n");
	}

	if (!release_swapchain(eye)) {
		if (eye == 1) {
			// we must always end our frame, even if we don't have an image to submit...
			end_frame(0, nullptr);
		}

		return;
	}

	projection_views[eye].fov = views[eye].fov;
	projection_views[eye].pose = views[eye].pose;

	if (eye == 1) {
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

		end_frame(static_cast<uint32_t>(layers_list.size()), layers_list.data());
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

	XrActionsSyncInfo syncInfo = {
		.type = XR_TYPE_ACTIONS_SYNC_INFO,
		.countActiveActionSets = (uint32_t)active_sets.size(),
		.activeActionSets = active_sets.data()
	};

	result = xrSyncActions(session, &syncInfo);
	xr_result(result, "failed to sync actions!");

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
		} else {
			bool is_active = false;

			// If our aim pose is active, our controller is active
			// note, if the user has removed this action then our old controller approach becomes defunct
			if (default_actions[ACTION_AIM_POSE].action != nullptr) {
				is_active = default_actions[ACTION_AIM_POSE].action->is_pose_active(input_path);
			}

			if (is_active) {
				if (inputmaps[i].godot_controller == -1) {
					// hate using const_cast here but godot_arvr_add_controller should have it's parameter defined as const, it doesn't change it...
					inputmaps[i].godot_controller = arvr_api->godot_arvr_add_controller(const_cast<char *>(inputmaps[i].name), (godot_int)i + 1, true, true);

#ifdef DEBUG
					Godot::print("OpenXR mapped {0} to {1}", inputmaps[i].name, inputmaps[i].godot_controller);
#endif
				}

				// copy for readability
				int godot_controller = inputmaps[i].godot_controller;

				// Start with our pose, we put our ARVRController on our aim pose (may need to change this to our grip pose...)
				godot_transform controller_transform;
				Transform *t = (Transform *)&controller_transform;
				inputmaps[i].tracking_confidence = default_actions[ACTION_AIM_POSE].action->get_as_pose(input_path, ws, *t);

				if (inputmaps[i].tracking_confidence != TRACKING_CONFIDENCE_NONE) {
					arvr_api->godot_arvr_set_controller_transform(godot_controller, &controller_transform, true, true);
				}

				// OK, so OpenXR will tell us if the value has changed and we could skip sending our value
				// but Godot also checks it so... just let Godot do it

				// Button and axis are hardcoded..
				// Axis
				if (default_actions[ACTION_FRONT_TRIGGER].action != nullptr) {
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 2, default_actions[ACTION_FRONT_TRIGGER].action->get_as_float(input_path), true); // 0.0 -> 1.0
				}
				if (default_actions[ACTION_SIDE_TRIGGER].action != nullptr) {
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 4, default_actions[ACTION_SIDE_TRIGGER].action->get_as_float(input_path), true); // 0.0 -> 1.0
				}
				if (default_actions[ACTION_PRIMARY].action != nullptr) {
					Vector2 v = default_actions[ACTION_PRIMARY].action->get_as_vector(input_path);
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 0, v.x, true); // -1.0 -> 1.0
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 1, v.y, true); // -1.0 -> 1.0
				}
				if (default_actions[ACTION_SECONDARY].action != nullptr) {
					Vector2 v = default_actions[ACTION_SECONDARY].action->get_as_vector(input_path);
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 6, v.x, true); // -1.0 -> 1.0
					arvr_api->godot_arvr_set_controller_axis(godot_controller, 7, v.y, true); // -1.0 -> 1.0
				}
				// Buttons
				if (default_actions[ACTION_AX_BUTTON].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 7, default_actions[ACTION_AX_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_BY_BUTTON].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 1, default_actions[ACTION_BY_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_AX_TOUCH].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 5, default_actions[ACTION_AX_TOUCH].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_BY_TOUCH].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 6, default_actions[ACTION_BY_TOUCH].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_MENU_BUTTON].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 3, default_actions[ACTION_MENU_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_SELECT_BUTTON].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 4, default_actions[ACTION_SELECT_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_FRONT_BUTTON].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 15, default_actions[ACTION_FRONT_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_FRONT_TOUCH].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 16, default_actions[ACTION_FRONT_TOUCH].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_SIDE_BUTTON].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 2, default_actions[ACTION_SIDE_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_PRIMARY_BUTTON].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 14, default_actions[ACTION_PRIMARY_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_SECONDARY_BUTTON].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 13, default_actions[ACTION_SECONDARY_BUTTON].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_PRIMARY_TOUCH].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 12, default_actions[ACTION_PRIMARY_TOUCH].action->get_as_bool(input_path));
				}
				if (default_actions[ACTION_SECONDARY_TOUCH].action != nullptr) {
					arvr_api->godot_arvr_set_controller_button(godot_controller, 11, default_actions[ACTION_SECONDARY_TOUCH].action->get_as_bool(input_path));
				}

				if (default_actions[ACTION_HAPTIC].action != nullptr) {
					// Godot currently only gives us a float between 0.0 and 1.0 for rumble strength.
					// Full haptic control will be offered through another object
					float haptic = arvr_api->godot_arvr_get_controller_rumble(godot_controller);
					if (haptic > 0.0) {
						// 17,000,000.0 nanoseconds (17ms) is slightly more then the duration of one frame if we're outputting at 60fps
						// so if we sustain our pulse we should be issuing a new pulse before the old one ends
						default_actions[ACTION_HAPTIC].action->do_haptic_pulse(input_path, 17.0 * 1000 * 1000, XR_FREQUENCY_UNSPECIFIED, haptic);
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

void OpenXRApi::recommended_rendertarget_size(uint32_t *width, uint32_t *height) {
	if (!initialised) {
		*width = 0;
		*height = 0;
	} else {
		*width = render_target_width;
		*height = render_target_height;
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

	if (views == nullptr || !view_pose_valid) {
		return false;
	}

	// Note that our views[eye].pose uses the current frames timing which is correct as this is what we use for rendering.
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
		.next = nullptr
	};

	XrTime time;
	if (form_factor == XR_FORM_FACTOR_HANDHELD_DISPLAY) {
		// For handheld displays we are rendering MONO and this method is called both for our rendering position
		// and to position our camera node for the next frame.
		// It is more important to get the rendering right here.
		// Note, there currently are no platforms where OpenXR is used with Godot that support this mode
		// so it's probably a non issue for the time being. This is already resolved in Godot 4.
		time = frameState.predictedDisplayTime;
	} else {
		// We retrieve our tracking information right before we render, we use the current frames predicted display time while rendering.
		// Our head center however is retrieved to place our camera node in the scene after rendering for the next frame.
		time = get_next_frame_time();
	}
	result = xrLocateSpace(view_space, play_space, time, &location);
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

	if (!frameState.shouldRender) {
		// We shouldn't be rendering at all but this prevents acquiring and rendering to our swap chain (Quest doesn't seem to like this)
		// instead we render to Godots internal buffers. Also good for desktop as we still get our preview.

		// We really should make it possible to return say -1 and have Godot skip rendering this frame. That would need to be a change upstream.
		return 0;
	}

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
	if (state > XR_SESSION_STATE_UNKNOWN && buffer_index != nullptr) {
		// make sure we know that we're rendering directly to our
		// texture chain
		*has_support = true;
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
		bool handled = false;
		for (XRExtensionWrapper *wrapper : registered_extension_wrappers) {
			handled |= wrapper->on_event_polled(runtimeEvent);
		}
		switch (runtimeEvent.type) {
			case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
				XrEventDataEventsLost *event = (XrEventDataEventsLost *)&runtimeEvent;

#ifdef DEBUG
				Godot::print("OpenXR EVENT: {0} event data lost!", event->lostEventCount);
#endif
				// we probably didn't poll fast enough'
			} break;
			case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR: {
				XrEventDataVisibilityMaskChangedKHR *event = (XrEventDataVisibilityMaskChangedKHR *)&runtimeEvent;
#ifdef DEBUG
				Godot::print("OpenXR EVENT: STUB: visibility mask changed");
#endif
			} break;
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
				XrEventDataInstanceLossPending *event = (XrEventDataInstanceLossPending *)&runtimeEvent;
#ifdef DEBUG
				Godot::print("OpenXR EVENT: instance loss pending at {0}!", event->lossTime);
#endif
				return false;
			} break;
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				XrEventDataSessionStateChanged *event = (XrEventDataSessionStateChanged *)&runtimeEvent;

				state = event->state;
				if (state >= XR_SESSION_STATE_MAX_ENUM) {
#ifdef DEBUG
					Godot::print("OpenXR EVENT: session state changed to UNKNOWN - {0}", state);
#endif
				} else {
#ifdef DEBUG
					Godot::print("OpenXR EVENT: session state changed to {0}", session_states[state]);
#endif

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
						case XR_SESSION_STATE_UNKNOWN:
							// including just for CI, this value we ignore
						case XR_SESSION_STATE_MAX_ENUM:
							// including just for CI, this value should never be used
						default:
							break;
					}
				}
			} break;
			case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
				XrEventDataReferenceSpaceChangePending *event = (XrEventDataReferenceSpaceChangePending *)&runtimeEvent;
#ifdef DEBUG
				Godot::print("OpenXR EVENT: reference space type {0} change pending!", event->referenceSpaceType);
#endif
				if (event->poseValid) {
					emit_plugin_signal(SIGNAL_POSE_RECENTERED);
				}
			} break;
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
#ifdef DEBUG
				Godot::print("OpenXR EVENT: interaction profile changed!");
#endif

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

#ifdef DEBUG
						Godot::print("OpenXR Event: Interaction profile changed for {0}: {1}", inputmaps[i].name, profile_str);
#endif
					}
				}

				// TODO: do something
			} break;
			default:
				if (!handled) {
					Godot::print_warning(String("OpenXR Unhandled event type ") + String::num_int64(runtimeEvent.type), __FUNCTION__, __FILE__, __LINE__);
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
	frameState.type = XR_TYPE_FRAME_STATE;
	frameState.next = nullptr;
	frameState.predictedDisplayTime = 0;
	frameState.predictedDisplayPeriod = 0;
	frameState.shouldRender = false;
	result = xrWaitFrame(session, &frameWaitInfo, &frameState);
	if (!xr_result(result, "xrWaitFrame() was not successful, exiting...")) {
		// reset just in case
		frameState.predictedDisplayTime = 0;
		frameState.predictedDisplayPeriod = 0;
		frameState.shouldRender = false;
		return;
	}

	if (frameState.predictedDisplayPeriod > 500000000) {
		// display period more then 0.5 seconds? must be wrong data
#ifdef DEBUG
		Godot::print("OpenXR resetting invalid display period {0}", frameState.predictedDisplayPeriod);
#endif
		frameState.predictedDisplayPeriod = 0;
	}

	// Workaround for dealing with swapchain not getting released properly after screen recording
	if (swapchain_error) {
		swapchain_error = false;
		Godot::print("Swapchains need reinitialization");
		cleanupSwapChains();
		initialiseSwapChains();
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
TrackingConfidence _transform_from_location(const T &p_location, Transform &r_transform) {
	Basis basis;
	Vector3 origin;
	TrackingConfidence confidence = TRACKING_CONFIDENCE_NONE;
	const auto &pose = p_location.pose;

	// Check orientation
	if (p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
		Quat q(pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w);
		r_transform.basis = Basis(q);

		if (p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) {
			// Fully valid orientation, so either 3DOF or 6DOF tracking with high confidence so default to HIGH_TRACKING
			confidence = TRACKING_CONFIDENCE_HIGH;
		} else {
			// Orientation is being tracked but we're using old/predicted data, so low tracking confidence
			confidence = TRACKING_CONFIDENCE_LOW;
		}
	} else {
		r_transform.basis = Basis();
	}

	// Check location
	if (p_location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
		r_transform.origin = Vector3(pose.position.x, pose.position.y, pose.position.z);

		if (!(p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT)) {
			// Location is being tracked but we're using old/predicted data, so low tracking confidence
			confidence = TRACKING_CONFIDENCE_LOW;
		} else if (confidence == TRACKING_CONFIDENCE_NONE) {
			// Position tracking without orientation tracking?
			confidence = TRACKING_CONFIDENCE_HIGH;
		}
	} else {
		// No tracking or 3DOF I guess..
		r_transform.origin = Vector3();
	}

	return confidence;
}

TrackingConfidence OpenXRApi::transform_from_location(const XrSpaceLocation &p_location, float p_world_scale, Transform &r_transform) {
	Transform t;
	TrackingConfidence confidence = _transform_from_location(p_location, t);
	if (confidence != TRACKING_CONFIDENCE_NONE) {
		// only update if we have tracking data
		r_transform = t;
	}
	return confidence;
}

TrackingConfidence OpenXRApi::transform_from_location(const XrHandJointLocationEXT &p_location, float p_world_scale, Transform &r_transform) {
	Transform t;
	TrackingConfidence confidence = _transform_from_location(p_location, t);
	if (confidence != TRACKING_CONFIDENCE_NONE) {
		// only update if we have tracking data
		r_transform = t;
	}
	return confidence;
}

godot::String OpenXRApi::get_swapchain_format_name(int64_t p_swapchain_format) {
	// These are somewhat different per platform, will need to weed some stuff out...
	switch (p_swapchain_format) {
#ifdef WIN32
		// using definitions from GLAD
		ENUM_TO_STRING_CASE(GL_R8_SNORM)
		ENUM_TO_STRING_CASE(GL_RG8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGBA8_SNORM)
		ENUM_TO_STRING_CASE(GL_R16_SNORM)
		ENUM_TO_STRING_CASE(GL_RG16_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB16_SNORM)
		ENUM_TO_STRING_CASE(GL_RGBA16_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB4)
		ENUM_TO_STRING_CASE(GL_RGB5)
		ENUM_TO_STRING_CASE(GL_RGB8)
		ENUM_TO_STRING_CASE(GL_RGB10)
		ENUM_TO_STRING_CASE(GL_RGB12)
		ENUM_TO_STRING_CASE(GL_RGB16)
		ENUM_TO_STRING_CASE(GL_RGBA2)
		ENUM_TO_STRING_CASE(GL_RGBA4)
		ENUM_TO_STRING_CASE(GL_RGB5_A1)
		ENUM_TO_STRING_CASE(GL_RGBA8)
		ENUM_TO_STRING_CASE(GL_RGB10_A2)
		ENUM_TO_STRING_CASE(GL_RGBA12)
		ENUM_TO_STRING_CASE(GL_RGBA16)
		ENUM_TO_STRING_CASE(GL_RGBA32F)
		ENUM_TO_STRING_CASE(GL_RGB32F)
		ENUM_TO_STRING_CASE(GL_RGBA16F)
		ENUM_TO_STRING_CASE(GL_RGB16F)
		ENUM_TO_STRING_CASE(GL_RGBA32UI)
		ENUM_TO_STRING_CASE(GL_RGB32UI)
		ENUM_TO_STRING_CASE(GL_RGBA16UI)
		ENUM_TO_STRING_CASE(GL_RGB16UI)
		ENUM_TO_STRING_CASE(GL_RGBA8UI)
		ENUM_TO_STRING_CASE(GL_RGB8UI)
		ENUM_TO_STRING_CASE(GL_RGBA32I)
		ENUM_TO_STRING_CASE(GL_RGB32I)
		ENUM_TO_STRING_CASE(GL_RGBA16I)
		ENUM_TO_STRING_CASE(GL_RGB16I)
		ENUM_TO_STRING_CASE(GL_RGBA8I)
		ENUM_TO_STRING_CASE(GL_RGB8I)
		ENUM_TO_STRING_CASE(GL_RGB10_A2UI)
		ENUM_TO_STRING_CASE(GL_SRGB)
		ENUM_TO_STRING_CASE(GL_SRGB8)
		ENUM_TO_STRING_CASE(GL_SRGB_ALPHA)
		ENUM_TO_STRING_CASE(GL_SRGB8_ALPHA8)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT16)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT24)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT32)
		ENUM_TO_STRING_CASE(GL_DEPTH24_STENCIL8)
		ENUM_TO_STRING_CASE(GL_R11F_G11F_B10F)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT32F)
		ENUM_TO_STRING_CASE(GL_DEPTH32F_STENCIL8)

#elif ANDROID
		// using definitions from GLES3/gl3.h

		ENUM_TO_STRING_CASE(GL_RGBA4)
		ENUM_TO_STRING_CASE(GL_RGB5_A1)
		ENUM_TO_STRING_CASE(GL_RGB565)
		ENUM_TO_STRING_CASE(GL_RGB8)
		ENUM_TO_STRING_CASE(GL_RGBA8)
		ENUM_TO_STRING_CASE(GL_RGB10_A2)
		ENUM_TO_STRING_CASE(GL_RGBA32F)
		ENUM_TO_STRING_CASE(GL_RGB32F)
		ENUM_TO_STRING_CASE(GL_RGBA16F)
		ENUM_TO_STRING_CASE(GL_RGB16F)
		ENUM_TO_STRING_CASE(GL_R11F_G11F_B10F)
		ENUM_TO_STRING_CASE(GL_UNSIGNED_INT_10F_11F_11F_REV)
		ENUM_TO_STRING_CASE(GL_RGB9_E5)
		ENUM_TO_STRING_CASE(GL_UNSIGNED_INT_5_9_9_9_REV)
		ENUM_TO_STRING_CASE(GL_RGBA32UI)
		ENUM_TO_STRING_CASE(GL_RGB32UI)
		ENUM_TO_STRING_CASE(GL_RGBA16UI)
		ENUM_TO_STRING_CASE(GL_RGB16UI)
		ENUM_TO_STRING_CASE(GL_RGBA8UI)
		ENUM_TO_STRING_CASE(GL_RGB8UI)
		ENUM_TO_STRING_CASE(GL_RGBA32I)
		ENUM_TO_STRING_CASE(GL_RGB32I)
		ENUM_TO_STRING_CASE(GL_RGBA16I)
		ENUM_TO_STRING_CASE(GL_RGB16I)
		ENUM_TO_STRING_CASE(GL_RGBA8I)
		ENUM_TO_STRING_CASE(GL_RGB8I)
		ENUM_TO_STRING_CASE(GL_RG)
		ENUM_TO_STRING_CASE(GL_RG_INTEGER)
		ENUM_TO_STRING_CASE(GL_R8)
		ENUM_TO_STRING_CASE(GL_RG8)
		ENUM_TO_STRING_CASE(GL_R16F)
		ENUM_TO_STRING_CASE(GL_R32F)
		ENUM_TO_STRING_CASE(GL_RG16F)
		ENUM_TO_STRING_CASE(GL_RG32F)
		ENUM_TO_STRING_CASE(GL_R8I)
		ENUM_TO_STRING_CASE(GL_R8UI)
		ENUM_TO_STRING_CASE(GL_R16I)
		ENUM_TO_STRING_CASE(GL_R16UI)
		ENUM_TO_STRING_CASE(GL_R32I)
		ENUM_TO_STRING_CASE(GL_R32UI)
		ENUM_TO_STRING_CASE(GL_RG8I)
		ENUM_TO_STRING_CASE(GL_RG8UI)
		ENUM_TO_STRING_CASE(GL_RG16I)
		ENUM_TO_STRING_CASE(GL_RG16UI)
		ENUM_TO_STRING_CASE(GL_RG32I)
		ENUM_TO_STRING_CASE(GL_RG32UI)
		ENUM_TO_STRING_CASE(GL_R8_SNORM)
		ENUM_TO_STRING_CASE(GL_RG8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGBA8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB10_A2UI)
		ENUM_TO_STRING_CASE(GL_SRGB)
		ENUM_TO_STRING_CASE(GL_SRGB8)
		ENUM_TO_STRING_CASE(GL_SRGB8_ALPHA8)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_R11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SIGNED_R11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RG11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SIGNED_RG11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RGB8_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SRGB8_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RGBA8_ETC2_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT16)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT24)
		ENUM_TO_STRING_CASE(GL_DEPTH24_STENCIL8)

#else
		// using definitions from GL/gl.h
		ENUM_TO_STRING_CASE(GL_ALPHA4_EXT)
		ENUM_TO_STRING_CASE(GL_ALPHA8_EXT)
		ENUM_TO_STRING_CASE(GL_ALPHA12_EXT)
		ENUM_TO_STRING_CASE(GL_ALPHA16_EXT)
		ENUM_TO_STRING_CASE(GL_LUMINANCE4_EXT)
		ENUM_TO_STRING_CASE(GL_LUMINANCE8_EXT)
		ENUM_TO_STRING_CASE(GL_LUMINANCE12_EXT)
		ENUM_TO_STRING_CASE(GL_LUMINANCE16_EXT)
		ENUM_TO_STRING_CASE(GL_LUMINANCE4_ALPHA4_EXT)
		ENUM_TO_STRING_CASE(GL_LUMINANCE6_ALPHA2_EXT)
		ENUM_TO_STRING_CASE(GL_LUMINANCE8_ALPHA8_EXT)
		ENUM_TO_STRING_CASE(GL_LUMINANCE12_ALPHA4_EXT)
		ENUM_TO_STRING_CASE(GL_LUMINANCE12_ALPHA12_EXT)
		ENUM_TO_STRING_CASE(GL_LUMINANCE16_ALPHA16_EXT)
		ENUM_TO_STRING_CASE(GL_INTENSITY_EXT)
		ENUM_TO_STRING_CASE(GL_INTENSITY4_EXT)
		ENUM_TO_STRING_CASE(GL_INTENSITY8_EXT)
		ENUM_TO_STRING_CASE(GL_INTENSITY12_EXT)
		ENUM_TO_STRING_CASE(GL_INTENSITY16_EXT)
		ENUM_TO_STRING_CASE(GL_RGB2_EXT)
		ENUM_TO_STRING_CASE(GL_RGB4_EXT)
		ENUM_TO_STRING_CASE(GL_RGB5_EXT)
		ENUM_TO_STRING_CASE(GL_RGB8_EXT)
		ENUM_TO_STRING_CASE(GL_RGB10_EXT)
		ENUM_TO_STRING_CASE(GL_RGB12_EXT)
		ENUM_TO_STRING_CASE(GL_RGB16_EXT)
		ENUM_TO_STRING_CASE(GL_RGBA2_EXT)
		ENUM_TO_STRING_CASE(GL_RGBA4_EXT)
		ENUM_TO_STRING_CASE(GL_RGB5_A1_EXT)
		ENUM_TO_STRING_CASE(GL_RGBA8_EXT)
		ENUM_TO_STRING_CASE(GL_RGB10_A2_EXT)
		ENUM_TO_STRING_CASE(GL_RGBA12_EXT)
		ENUM_TO_STRING_CASE(GL_RGBA16_EXT)
		ENUM_TO_STRING_CASE(GL_SRGB_EXT)
		ENUM_TO_STRING_CASE(GL_SRGB8_EXT)
		ENUM_TO_STRING_CASE(GL_SRGB_ALPHA_EXT)
		ENUM_TO_STRING_CASE(GL_SRGB8_ALPHA8_EXT)
#endif
		default: {
			return String("Swapchain format 0x") + String::num_int64(p_swapchain_format, 16);
		} break;
	}
}
