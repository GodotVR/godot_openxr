////////////////////////////////////////////////////////////////////////////////////////////////
// Class for OpenXR Actions

#include "openxr/OpenXRApi.h"

using namespace godot;

Action::Action(OpenXRApi *p_api, XrActionSet p_action_set, XrActionType p_type, const godot::String &p_name, const godot::String &p_localised_name, int p_toplevel_path_count, const XrPath *p_toplevel_paths) {
	xr_api = p_api;
	type = p_type;
	name = p_name;

	for (int i = 0; i < p_toplevel_path_count; i++) {
		path_with_space tlp = {
			p_toplevel_paths[i], XR_NULL_HANDLE, false
		};

		toplevel_paths.push_back(tlp);
	}

	XrActionCreateInfo actionInfo = {
		.type = XR_TYPE_ACTION_CREATE_INFO,
		.next = nullptr,
		.actionType = type,
		.countSubactionPaths = (uint32_t)p_toplevel_path_count,
		.subactionPaths = p_toplevel_paths
	};

	strcpy(actionInfo.actionName, name.utf8().get_data());
	strcpy(actionInfo.localizedActionName, p_localised_name.utf8().get_data());

	XrResult result = xrCreateAction(p_action_set, &actionInfo, &handle);
	if (!p_api->xr_result(result, "failed to create {0} action", name)) {
		return;
	}
}

Action::~Action() {
	// no need to delete our paths, we don't own them but we do need to delete our spaces
	reset_spaces();

	if (handle != XR_NULL_HANDLE) {
		xrDestroyAction(handle);
	}
}

void Action::reset_spaces() {
	for (int i = 0; i < toplevel_paths.size(); i++) {
		if (toplevel_paths[i].space != XR_NULL_HANDLE) {
			xrDestroySpace(toplevel_paths[i].space);
			toplevel_paths[i].space = XR_NULL_HANDLE;
		}
	}
}

XrActionType Action::get_type() const {
	return type;
}

godot::String Action::get_name() const {
	return name;
}

XrAction Action::get_action() const {
	return handle;
}

bool Action::get_as_bool(XrPath p_path) {
	if (!xr_api->is_running()) {
		// not running
		return false;
	} else if (handle == XR_NULL_HANDLE || p_path == XR_NULL_PATH) {
		// not initialised
		return false;
	} else if (type != XR_ACTION_TYPE_BOOLEAN_INPUT) {
		// wrong type
		return false;
	} else {
		XrActionStateGetInfo getInfo = {
			.type = XR_TYPE_ACTION_STATE_GET_INFO,
			.next = nullptr,
			.action = handle,
			.subactionPath = p_path
		};

		XrActionStateBoolean resultState;
		resultState.type = XR_TYPE_ACTION_STATE_BOOLEAN,
		resultState.next = nullptr;
		XrResult result = xrGetActionStateBoolean(xr_api->session, &getInfo, &resultState);
		if (!xr_api->xr_result(result, "failed to get boolean value")) {
			resultState.isActive = false;
		}

		// we should do something with resultState.isActive

		return resultState.currentState;
	}
}

float Action::get_as_float(XrPath p_path) {
	if (!xr_api->is_running()) {
		// not running
		return 0.0;
	} else if (handle == XR_NULL_HANDLE || p_path == XR_NULL_PATH) {
		// not initialised
		return 0.0;
	} else if (type != XR_ACTION_TYPE_FLOAT_INPUT) {
		// wrong type
		return 0.0;
	} else {
		XrActionStateGetInfo getInfo = {
			.type = XR_TYPE_ACTION_STATE_GET_INFO,
			.next = nullptr,
			.action = handle,
			.subactionPath = p_path
		};

		XrActionStateFloat resultState;
		resultState.type = XR_TYPE_ACTION_STATE_FLOAT,
		resultState.next = nullptr;
		XrResult result = xrGetActionStateFloat(xr_api->session, &getInfo, &resultState);
		if (!xr_api->xr_result(result, "failed to get float value")) {
			resultState.isActive = false;
		}

		// we should do something with resultState.isActive

		// clamp our value between -1.0 and 1.0, shouldn't be outside of this range but better safe then sorry...
		if (resultState.currentState < -1.0) {
			resultState.currentState = -1.0;
		} else if (resultState.currentState > 1.0) {
			resultState.currentState = 1.0;
		}

		return resultState.currentState;
	}
}

Vector2 Action::get_as_vector(XrPath p_path) {
	if (!xr_api->is_running()) {
		// not running
		return Vector2();
	} else if (handle == XR_NULL_HANDLE || p_path == XR_NULL_PATH) {
		// not initialised
		return Vector2();
	} else if (type != XR_ACTION_TYPE_VECTOR2F_INPUT) {
		// wrong type
		return Vector2();
	} else {
		XrActionStateGetInfo getInfo = {
			.type = XR_TYPE_ACTION_STATE_GET_INFO,
			.next = nullptr,
			.action = handle,
			.subactionPath = p_path
		};

		XrActionStateVector2f resultState;
		resultState.type = XR_TYPE_ACTION_STATE_VECTOR2F,
		resultState.next = nullptr;
		XrResult result = xrGetActionStateVector2f(xr_api->session, &getInfo, &resultState);
		if (!xr_api->xr_result(result, "failed to get vector value")) {
			resultState.isActive = false;
		}

		// we should do something with resultState.isActive

		return Vector2(resultState.currentState.x, resultState.currentState.y);
	}
}

bool Action::is_pose_active(XrPath p_path) {
	if (!xr_api->is_running()) {
		// not running
		return false;
	} else if (handle == XR_NULL_HANDLE || p_path == XR_NULL_PATH) {
		// not initialised
		Godot::print("Pose not initialised");
		return false;
	} else if (type != XR_ACTION_TYPE_POSE_INPUT) {
		// wrong type
		Godot::print("Not a pose type");
		return false;
	} else {
		XrActionStateGetInfo getInfo = {
			.type = XR_TYPE_ACTION_STATE_GET_INFO,
			.next = nullptr,
			.action = handle,
			.subactionPath = p_path
		};

		XrActionStatePose resultState;
		resultState.type = XR_TYPE_ACTION_STATE_POSE,
		resultState.next = nullptr;
		XrResult result = xrGetActionStatePose(xr_api->session, &getInfo, &resultState);
		if (!xr_api->xr_result(result, "failed to get pose state")) {
			resultState.isActive = false;
		}

		return resultState.isActive;
	}
}

TrackingConfidence Action::get_as_pose(XrPath p_path, float p_world_scale, Transform &r_transform) {
	if (!xr_api->is_running()) {
		// not running
		return TRACKING_CONFIDENCE_NONE;
	} else if (handle == XR_NULL_HANDLE || p_path == XR_NULL_PATH) {
		// not initialised or setup fully
		return TRACKING_CONFIDENCE_NONE;
	} else if (type != XR_ACTION_TYPE_POSE_INPUT) {
		// wrong type
		return TRACKING_CONFIDENCE_NONE;
	} else {
		// find out the index for our path, note, thanks to register_path we can use our pointers here
		uint64_t index = 0xFFFFFFFF;
		for (uint64_t i = 0; i < toplevel_paths.size() && index == 0xFFFFFFFF; i++) {
			if (toplevel_paths[i].toplevel_path == p_path) {
				index = i;
			}
		}

		if (index == 0xFFFFFFFF) {
			// couldn't find it?
			return TRACKING_CONFIDENCE_NONE;
		}

		if (toplevel_paths[index].space == XR_NULL_HANDLE) {
			// if this is a pose we need to define spaces

			XrActionSpaceCreateInfo actionSpaceInfo = {
				.type = XR_TYPE_ACTION_SPACE_CREATE_INFO,
				.next = nullptr,
				.action = handle,
				.poseInActionSpace = {
						.orientation = {
								.w = 1.f } },
			};

			actionSpaceInfo.subactionPath = toplevel_paths[index].toplevel_path;

			XrResult result = xrCreateActionSpace(xr_api->session, &actionSpaceInfo, &toplevel_paths[index].space);
			if (!xr_api->xr_result(result, "failed to create pose space")) {
				return TRACKING_CONFIDENCE_NONE;
			}
		}

		XrSpaceLocation location;

		location.type = XR_TYPE_SPACE_LOCATION;
		location.next = nullptr;

		XrTime time = xr_api->get_next_frame_time(); // This data will be used for the next frame we render
		XrResult result = xrLocateSpace(toplevel_paths[index].space, xr_api->play_space, time, &location);
		if (!xr_api->xr_result(result, "failed to locate space!")) {
			return TRACKING_CONFIDENCE_NONE;
		}

		return xr_api->transform_from_location(location, p_world_scale, r_transform);

		/*
		bool spaceLocationValid =
				//(location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
				(location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;

		if (spaceLocationValid != toplevel_paths[index].wasLocationvalid) {
			// prevent error spam, just let user know when we've lost tracking and when we've regained it.
			toplevel_paths[index].wasLocationvalid = spaceLocationValid;

			if (!spaceLocationValid) {
				Godot::print_warning(String("OpenXR Space location not valid for hand ") + String::num_int64(index), __FUNCTION__, __FILE__, __LINE__);
				return Transform();
			} else {
				Godot::print("OpenXR regained tracking for hand {0}", index);
			}
		}

		if (spaceLocationValid) {
			return xr_api->transform_from_pose(location.pose, p_world_scale);
		} else {
			return Transform();
		}
		*/
	}
}

void Action::do_haptic_pulse(const XrPath p_path, XrDuration p_duration, float p_frequency, float p_amplitude) {
	if (!xr_api->is_running()) {
		// not running
		return;
	} else if (handle == XR_NULL_HANDLE || p_path == XR_NULL_PATH) {
		// not initialised or setup fully
		return;
	} else if (type != XR_ACTION_TYPE_VIBRATION_OUTPUT) {
		// wrong type
		return;
	} else {
		XrHapticActionInfo action_info;
		XrHapticVibration vibration;

		action_info.type = XR_TYPE_HAPTIC_ACTION_INFO;
		action_info.next = nullptr;
		action_info.action = handle;
		action_info.subactionPath = p_path;

		vibration.type = XR_TYPE_HAPTIC_VIBRATION;
		vibration.next = nullptr;
		vibration.duration = p_duration;
		vibration.frequency = p_frequency;
		vibration.amplitude = p_amplitude;

		XrResult res = xrApplyHapticFeedback(xr_api->get_session(), &action_info, (const XrHapticBaseHeader *)&vibration);
		xr_api->xr_result(res, "Applying haptic pulse");
	}
}
