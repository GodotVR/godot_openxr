/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR pose GDNative object, this exposes specific locations we track

#include <ARVRServer.hpp>

#include "gdclasses/OpenXRPose.h"

using namespace godot;

void OpenXRPose::_register_methods() {
	register_method("_physics_process", &OpenXRPose::_physics_process);

	register_method("get_invisible_if_inactive", &OpenXRPose::get_invisible_if_inactive);
	register_method("set_invisible_if_inactive", &OpenXRPose::set_invisible_if_inactive);
	register_property<OpenXRPose, bool>(
			"invisible_if_inactive",
			&OpenXRPose::set_invisible_if_inactive,
			&OpenXRPose::get_invisible_if_inactive,
			true);

	// For now these are hard coded based on our actions
	// As our actions JSON is parsed after initialisation we can't really present the dropdown (yet)
	// For now this will do
	// Note that SkeletonBase is a special value for our hand skeleton support.
	register_method("get_action", &OpenXRPose::get_action);
	register_method("set_action", &OpenXRPose::set_action);
	register_property<OpenXRPose, String>(
			"action",
			&OpenXRPose::set_action,
			&OpenXRPose::get_action,
			String("SkeletonBase"),
			GODOT_METHOD_RPC_MODE_DISABLED,
			GODOT_PROPERTY_USAGE_DEFAULT,
			GODOT_PROPERTY_HINT_ENUM,
			"SkeletonBase,godot/aim_pose,godot/grip_pose,godot/palm_pose");

	// For now this is hard coded, these are fixed entries based on the OpenXR spec
	register_method("get_path", &OpenXRPose::get_path);
	register_method("set_path", &OpenXRPose::set_path);
	register_property<OpenXRPose, String>(
			"path",
			&OpenXRPose::set_path,
			&OpenXRPose::get_path,
			String("/user/hand/left"),
			GODOT_METHOD_RPC_MODE_DISABLED,
			GODOT_PROPERTY_USAGE_DEFAULT,
			GODOT_PROPERTY_HINT_ENUM,
			"/user/hand/left,/user/hand/right,/user/treadmill");

	register_method("is_active", &OpenXRPose::is_active);
	register_method("get_tracking_confidence", &OpenXRPose::get_tracking_confidence);
}

OpenXRPose::OpenXRPose() {
	invisible_if_inactive = true;
	action = String("SkeletonBase");
	_action = nullptr;
	path = String("/user/hand/left");
	_path = XR_NULL_PATH;
	openxr_api = OpenXRApi::openxr_get_api();
	hand_tracking_wrapper = XRExtHandTrackingExtensionWrapper::get_singleton();
}

OpenXRPose::~OpenXRPose() {
	if (openxr_api != nullptr) {
		OpenXRApi::openxr_release_api();
	}

	hand_tracking_wrapper = nullptr;
}

void OpenXRPose::_init() {
	// nothing to do here
}

bool OpenXRPose::check_action_and_path() {
	// not yet ready?
	if (!openxr_api->has_action_sets()) {
		return false;
	}

	// don't keep trying this over and over and over again if we fail
	if (fail_cache) {
		return false;
	}

	if (_action == nullptr) {
		Array split = action.split("/");
		if (split.size() != 2) {
			Godot::print("Incorrect action string {0}", action);
			fail_cache = true;
			return false;
		}

		ActionSet *aset = openxr_api->get_action_set(split[0]);
		if (aset == nullptr) {
			Godot::print("Couldn't find action set {0}", split[0]);
			fail_cache = true;
			return false;
		}

		_action = aset->get_action(split[1]);
		if (_action == nullptr) {
			Godot::print("Couldn't find action {0}", split[1]);
			fail_cache = true;
			return false;
		}
	}

	if (_path == XR_NULL_PATH) {
		XrResult res = xrStringToPath(openxr_api->get_instance(), path.utf8().get_data(), &_path);
		if (!openxr_api->xr_result(res, "Couldn't obtain path {0}", path)) {
			fail_cache = true;
			return false;
		}
	}

	return true;
}

void OpenXRPose::_physics_process(float delta) {
	if (openxr_api == nullptr || hand_tracking_wrapper == nullptr) {
		return;
	} else if (!openxr_api->is_initialised()) {
		return;
	}

	if (invisible_if_inactive) {
		set_visible(is_active());
	}

	ARVRServer *server = ARVRServer::get_singleton();
	const float ws = server->get_world_scale();
	Transform reference_frame = server->get_reference_frame();

	if (action == "SkeletonBase") {
		if (path == "/user/hand/left") {
			const HandTracker *hand_tracker = hand_tracking_wrapper->get_hand_tracker(0);
			Transform t;
			confidence = openxr_api->transform_from_location(hand_tracker->joint_locations[XR_HAND_JOINT_PALM_EXT], ws, t);
			set_transform(reference_frame * t);
		} else if (path == "/user/hand/right") {
			const HandTracker *hand_tracker = hand_tracking_wrapper->get_hand_tracker(1);
			Transform t;
			confidence = openxr_api->transform_from_location(hand_tracker->joint_locations[XR_HAND_JOINT_PALM_EXT], ws, t);
			set_transform(reference_frame * t);
		}
	} else if (check_action_and_path()) {
		Transform t;
		confidence = _action->get_as_pose(_path, ws, t);
		set_transform(reference_frame * t);
	}
}

bool OpenXRPose::is_active() {
	if (openxr_api == nullptr || hand_tracking_wrapper == nullptr) {
		return false;
	} else if (!openxr_api->is_initialised()) {
		return false;
	}

	if (action == "SkeletonBase") {
		if (path == "/user/hand/left") {
			const HandTracker *hand_tracker = hand_tracking_wrapper->get_hand_tracker(0);

			return (hand_tracker->is_initialised && hand_tracker->locations.isActive);
		} else if (path == "/user/hand/right") {
			const HandTracker *hand_tracker = hand_tracking_wrapper->get_hand_tracker(1);

			return (hand_tracker->is_initialised && hand_tracker->locations.isActive);
		}
	} else if (check_action_and_path()) {
		return _action->is_pose_active(_path);
	}

	return false;
}

bool OpenXRPose::get_invisible_if_inactive() const {
	return invisible_if_inactive;
}

void OpenXRPose::set_invisible_if_inactive(bool hide) {
	invisible_if_inactive = hide;
}

String OpenXRPose::get_action() const {
	return action;
}

void OpenXRPose::set_action(const String p_action) {
	action = p_action;
	_action = nullptr;
	fail_cache = false;
}

String OpenXRPose::get_path() const {
	return path;
}

void OpenXRPose::set_path(const String p_path) {
	path = p_path;
	_path = XR_NULL_PATH;
	fail_cache = false;
}

int OpenXRPose::get_tracking_confidence() const {
	return int(confidence);
}
