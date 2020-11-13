/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR pose GDNative object, this exposes specific locations we track

#include <ARVRServer.hpp>

#include "OpenXRPose.h"

using namespace godot;

void OpenXRPose::_register_methods() {
	register_method("_physics_process", &OpenXRPose::_physics_process);

	register_method("get_pose", &OpenXRPose::get_pose);
	register_method("set_pose", &OpenXRPose::set_pose);
	register_property<OpenXRPose, int>(
			"hand",
			&OpenXRPose::set_pose,
			&OpenXRPose::get_pose,
			0,
			GODOT_METHOD_RPC_MODE_DISABLED,
			GODOT_PROPERTY_USAGE_DEFAULT,
			GODOT_PROPERTY_HINT_ENUM,
			"Left hand,Right hand");
}

OpenXRPose::OpenXRPose() {
	pose = Poses::POSE_LEFT_HAND;
	openxr_api = OpenXRApi::openxr_get_api();
}

OpenXRPose::~OpenXRPose() {
	if (openxr_api != NULL) {
		OpenXRApi::openxr_release_api();
	}
}

void OpenXRPose::_init() {
	// nothing to do here
}

void OpenXRPose::_physics_process(float delta) {
	if (openxr_api == NULL) {
		return;
	}

	const float ws = ARVRServer::get_singleton()->get_world_scale();
	switch (pose) {
		case POSE_LEFT_HAND: {
			const HandTracker *hand_tracker = openxr_api->get_hand_tracker(OpenXRApi::HAND_LEFT);
			const XrPosef &pose = hand_tracker->joint_locations[XR_HAND_JOINT_PALM_EXT].pose;
			set_transform(openxr_api->transform_from_pose(pose, ws));
		}; break;
		case POSE_RIGHT_HAND: {
			const HandTracker *hand_tracker = openxr_api->get_hand_tracker(OpenXRApi::HAND_RIGHT);
			const XrPosef &pose = hand_tracker->joint_locations[XR_HAND_JOINT_PALM_EXT].pose;
			set_transform(openxr_api->transform_from_pose(pose, ws));
		}; break;
		default:
			break;
	}
}

int OpenXRPose::get_pose() const {
	return pose;
}

void OpenXRPose::set_pose(int p_pose) {
	if (pose >= 0 && pose < POSE_MAX) {
		pose = (Poses)p_pose;
	}
}
