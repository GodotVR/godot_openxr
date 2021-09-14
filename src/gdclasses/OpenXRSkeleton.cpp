/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR hand GDNative object implemented through skeleton (bones)

#include <godot_cpp/classes/xr_server.hpp>

#include "OpenXRSkeleton.h"

using namespace godot;

void OpenXRSkeleton::_bind_methods() {
	// these are already defined through our base class
	// ClassDB::bind_method(D_METHOD("_ready"), &OpenXRSkeleton::_ready);
	// ClassDB::bind_method(D_METHOD("_physics_process", "delta"), &OpenXRSkeleton::_physics_process);

	ClassDB::bind_method(D_METHOD("get_hand"), &OpenXRSkeleton::get_hand);
	ClassDB::bind_method(D_METHOD("set_hand", "hand"), &OpenXRSkeleton::set_hand);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "hand", PROPERTY_HINT_ENUM, "Left,Right"), "set_hand", "get_hand");

	ClassDB::bind_method(D_METHOD("get_motion_range"), &OpenXRSkeleton::get_motion_range);
	ClassDB::bind_method(D_METHOD("set_motion_range", "motion_range"), &OpenXRSkeleton::set_motion_range);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "motion_range", PROPERTY_HINT_ENUM, "Unobstructed,Conform to controller"), "set_motion_range", "get_motion_range");
}

OpenXRSkeleton::OpenXRSkeleton() {
	hand = 0;
	motion_range = 0;
	openxr_api = OpenXRApi::openxr_get_api();
	hand_tracking_wrapper = XRExtHandTrackingExtensionWrapper::get_singleton();

	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		bones[i] = -1;
	}
}

OpenXRSkeleton::~OpenXRSkeleton() {
	if (openxr_api != NULL) {
		OpenXRApi::openxr_release_api();
	}

	hand_tracking_wrapper = nullptr;
}

void OpenXRSkeleton::_ready() {
	const char *bone_names[XR_HAND_JOINT_COUNT_EXT] = {
		"Palm",
		"Wrist",
		"Thumb_Metacarpal",
		"Thumb_Proximal",
		"Thumb_Distal",
		"Thumb_Tip",
		"Index_Metacarpal",
		"Index_Proximal",
		"Index_Intermediate",
		"Index_Distal",
		"Index_Tip",
		"Middle_Metacarpal",
		"Middle_Proximal",
		"Middle_Intermediate",
		"Middle_Distal",
		"Middle_Tip",
		"Ring_Metacarpal",
		"Ring_Proximal",
		"Ring_Intermediate",
		"Ring_Distal",
		"Ring_Tip",
		"Little_Metacarpal",
		"Little_Proximal",
		"Little_Intermediate",
		"Little_Distal",
		"Little_Tip",
	};

	// We cast to Node3D which should allow us to use any subclass of that.
	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		char bone_name[250];
		if (hand == 0) {
			sprintf(bone_name, "%s_L", bone_names[i]);
		} else {
			sprintf(bone_name, "%s_R", bone_names[i]);
		}

		bones[i] = find_bone(bone_name);
		if (bones[i] == -1) {
			UtilityFunctions::print("Couldn't obtain bone for {0}", bone_name);
		}
	}

	_set_motion_range();
}

void OpenXRSkeleton::_physics_process(double delta) {
	if (openxr_api == nullptr || hand_tracking_wrapper == nullptr) {
		return;
	} else if (!openxr_api->is_initialised()) {
		return;
	}

	// we cache our transforms so we can quickly calculate local transforms
	Transform3D transforms[XR_HAND_JOINT_COUNT_EXT];
	Transform3D inv_transforms[XR_HAND_JOINT_COUNT_EXT];

	const HandTracker *hand_tracker = hand_tracking_wrapper->get_hand_tracker(hand);
	const float ws = XRServer::get_singleton()->get_world_scale();

	if (hand_tracker->is_initialised && hand_tracker->locations.isActive) {
		// get our transforms
		for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
			transforms[i] = openxr_api->transform_from_space_location(hand_tracker->joint_locations[i], ws);
			inv_transforms[i] = transforms[i].inverse();
		}

		// now update our skeleton
		for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
			if (bones[i] != -1) {
				// FIXME REDO THIS, Godot now uses quaternions to orient bones
				
				int bone = bones[i];
				int parent = get_bone_parent(bone);

				Transform3D t = transforms[i];

				// get local translation, parent should already be processed
				if (parent == -1) {
					// use our palm location here, that is what we are tracking
					t = inv_transforms[XR_HAND_JOINT_PALM_EXT] * t;
				} else {
					int found = false;
					for (int b = 0; b < XR_HAND_JOINT_COUNT_EXT && !found; b++) {
						if (bones[b] == parent) {
							t = inv_transforms[b] * t;
							found = true;
						}
					}
				}

				// get difference with rest
				Transform3D rest = get_bone_rest(bones[i]);
				t = rest.inverse() * t;

				// and set our pose
				// set_bone_pose(bones[i], t);
			}
		}

		// show it
		set_visible(true);
	} else {
		// hide it
		set_visible(false);
	}
}

int OpenXRSkeleton::get_hand() const {
	return hand;
}

void OpenXRSkeleton::set_hand(int p_hand) {
	hand = p_hand == 1 ? 1 : 0;
}

int OpenXRSkeleton::get_motion_range() const {
	return motion_range;
}

void OpenXRSkeleton::set_motion_range(int p_motion_range) {
	motion_range = p_motion_range;
	_set_motion_range();
}

void OpenXRSkeleton::_set_motion_range() {
	if (hand_tracking_wrapper == nullptr) {
		return;
	}

	XrHandJointsMotionRangeEXT xr_motion_range;
	switch (motion_range) {
		case 0:
			xr_motion_range = XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT;
			break;
		case 1:
			xr_motion_range = XR_HAND_JOINTS_MOTION_RANGE_CONFORMING_TO_CONTROLLER_EXT;
			break;
		default:
			xr_motion_range = XR_HAND_JOINTS_MOTION_RANGE_CONFORMING_TO_CONTROLLER_EXT;
			break;
	}

	hand_tracking_wrapper->set_motion_range(hand, xr_motion_range);
}
