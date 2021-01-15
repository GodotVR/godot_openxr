/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR hand GDNative object implemented through skeleton (bones)

#include <ARVRServer.hpp>

#include "OpenXRSkeleton.h"

using namespace godot;

void OpenXRSkeleton::_register_methods() {
	register_method("_ready", &OpenXRSkeleton::_ready);
	register_method("_physics_process", &OpenXRSkeleton::_physics_process);

	register_method("get_hand", &OpenXRSkeleton::get_hand);
	register_method("set_hand", &OpenXRSkeleton::set_hand);
	register_property<OpenXRSkeleton, int>("hand", &OpenXRSkeleton::set_hand, &OpenXRSkeleton::get_hand, 0, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "Left,Right");
}

OpenXRSkeleton::OpenXRSkeleton() {
	hand = 0;
	openxr_api = OpenXRApi::openxr_get_api();

	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		bones[i] = -1;
	}
}

OpenXRSkeleton::~OpenXRSkeleton() {
	if (openxr_api != NULL) {
		OpenXRApi::openxr_release_api();
	}
}

void OpenXRSkeleton::_init() {
	// nothing to do here
}

void OpenXRSkeleton::_ready() {
	const char *bone_names[XR_HAND_JOINT_COUNT_EXT] = {
		".",
		"Wrist",
		"ThumbMetacarpal",
		"ThumbProximal",
		"ThumbDistal",
		"ThumbTip",
		"IndexMetacarpal",
		"IndexProximal",
		"IndexIntermediate",
		"IndexDistal",
		"IndexTip",
		"MiddleMetacarpal",
		"MiddleProximal",
		"MiddleIntermediate",
		"MiddleDistal",
		"MiddleTip",
		"RingMetacarpal",
		"RingProximal",
		"RingIntermediate",
		"RingDistal",
		"RingTip",
		"LittleMetacarpal",
		"LittleProximal",
		"LittleIntermediate",
		"LittleDistal",
		"LittleTip",
	};

	// We cast to spatials which should allow us to use any subclass of that.
	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		bones[i] = find_bone(bone_names[i]);
		if (bones[i] == -1) {
			printf("Couldn't obtain bone for %s\n", bone_names[i]);
		}
	}
}

void OpenXRSkeleton::_physics_process(float delta) {
	if (openxr_api == NULL) {
		return;
	} else if (!openxr_api->is_initialised()) {
		return;
	}

	const int parents[XR_HAND_JOINT_COUNT_EXT]{
		-1, // XR_HAND_JOINT_PALM_EXT = 0,
		0, // XR_HAND_JOINT_WRIST_EXT = 1,
		1, // XR_HAND_JOINT_THUMB_METACARPAL_EXT = 2,
		2, // XR_HAND_JOINT_THUMB_PROXIMAL_EXT = 3,
		3, // XR_HAND_JOINT_THUMB_DISTAL_EXT = 4,
		4, // XR_HAND_JOINT_THUMB_TIP_EXT = 5,
		1, // XR_HAND_JOINT_INDEX_METACARPAL_EXT = 6,
		6, // XR_HAND_JOINT_INDEX_PROXIMAL_EXT = 7,
		7, // XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT = 8,
		8, // XR_HAND_JOINT_INDEX_DISTAL_EXT = 9,
		9, // XR_HAND_JOINT_INDEX_TIP_EXT = 10,
		1, // XR_HAND_JOINT_MIDDLE_METACARPAL_EXT = 11,
		11, // XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT = 12,
		12, // XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT = 13,
		13, // XR_HAND_JOINT_MIDDLE_DISTAL_EXT = 14,
		14, // XR_HAND_JOINT_MIDDLE_TIP_EXT = 15,
		1, // XR_HAND_JOINT_RING_METACARPAL_EXT = 16,
		16, // XR_HAND_JOINT_RING_PROXIMAL_EXT = 17,
		17, // XR_HAND_JOINT_RING_INTERMEDIATE_EXT = 18,
		18, // XR_HAND_JOINT_RING_DISTAL_EXT = 19,
		19, // XR_HAND_JOINT_RING_TIP_EXT = 20,
		1, // XR_HAND_JOINT_LITTLE_METACARPAL_EXT = 21,
		21, // XR_HAND_JOINT_LITTLE_PROXIMAL_EXT = 22,
		22, // XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT = 23,
		23, // XR_HAND_JOINT_LITTLE_DISTAL_EXT = 24,
		24, // XR_HAND_JOINT_LITTLE_TIP_EXT = 25,
	};

	// we cache the inverse of our transforms so we can quickly calculate local transforms
	Transform inv_transforms[XR_HAND_JOINT_COUNT_EXT];

	const HandTracker *hand_tracker = openxr_api->get_hand_tracker(hand);
	const float ws = ARVRServer::get_singleton()->get_world_scale();

	if (hand_tracker->is_initialised && hand_tracker->locations.isActive) {
		for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
			if (i == 0) {
				// just get our transform, we place our mesh with OpenXRPose
				const XrPosef &pose = hand_tracker->joint_locations[i].pose;
				Transform t = openxr_api->transform_from_pose(pose, ws);

				inv_transforms[i] = t.inverse();
			} else if (bones[i] == -1) {
				// missing bone...
				inv_transforms[i] = Transform();
			} else if (parents[i] != -1) {
				const XrPosef &pose = hand_tracker->joint_locations[i].pose;

				Transform t = openxr_api->transform_from_pose(pose, ws);
				inv_transforms[i] = t.inverse();

				// get local translation
				if (parents[i] != -1) {
					// parent should already be processed
					int parent = parents[i];
					t = inv_transforms[parent] * t;
				}

				// get difference with rest
				Transform rest = get_bone_rest(bones[i]);
				t = rest.inverse() * t;

				// and set our pose
				set_bone_pose(bones[i], t);
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
