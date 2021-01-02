/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR hand GDNative object implemented through meshes

#include <ARVRServer.hpp>

#include "OpenXRHand.h"

using namespace godot;

void OpenXRHand::_register_methods() {
	register_method("_ready", &OpenXRHand::_ready);
	register_method("_physics_process", &OpenXRHand::_physics_process);

	register_method("get_hand", &OpenXRHand::get_hand);
	register_method("set_hand", &OpenXRHand::set_hand);
	register_property<OpenXRHand, int>("hand", &OpenXRHand::set_hand, &OpenXRHand::get_hand, 0, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "Left,Right");
}

OpenXRHand::OpenXRHand() {
	hand = OpenXRApi::HAND_LEFT;
	openxr_api = OpenXRApi::openxr_get_api();

	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		joints[i] = NULL;
	}
}

OpenXRHand::~OpenXRHand() {
	if (openxr_api != NULL) {
		OpenXRApi::openxr_release_api();
	}
}

void OpenXRHand::_init() {
	// nothing to do here
}

void OpenXRHand::_ready() {
	const char *node_names[XR_HAND_JOINT_COUNT_EXT] = {
		".",
		"Wrist",
		"Wrist/ThumbMetacarpal",
		"Wrist/ThumbMetacarpal/ThumbProximal",
		"Wrist/ThumbMetacarpal/ThumbProximal/ThumbDistal",
		"Wrist/ThumbMetacarpal/ThumbProximal/ThumbDistal/ThumbTip",
		"Wrist/IndexMetacarpal",
		"Wrist/IndexMetacarpal/IndexProximal",
		"Wrist/IndexMetacarpal/IndexProximal/IndexIntermediate",
		"Wrist/IndexMetacarpal/IndexProximal/IndexIntermediate/IndexDistal",
		"Wrist/IndexMetacarpal/IndexProximal/IndexIntermediate/IndexDistal/IndexTip",
		"Wrist/MiddleMetacarpal",
		"Wrist/MiddleMetacarpal/MiddleProximal",
		"Wrist/MiddleMetacarpal/MiddleProximal/MiddleIntermediate",
		"Wrist/MiddleMetacarpal/MiddleProximal/MiddleIntermediate/MiddleDistal",
		"Wrist/MiddleMetacarpal/MiddleProximal/MiddleIntermediate/MiddleDistal/MiddleTip",
		"Wrist/RingMetacarpal",
		"Wrist/RingMetacarpal/RingProximal",
		"Wrist/RingMetacarpal/RingProximal/RingIntermediate",
		"Wrist/RingMetacarpal/RingProximal/RingIntermediate/RingDistal",
		"Wrist/RingMetacarpal/RingProximal/RingIntermediate/RingDistal/RingTip",
		"Wrist/LittleMetacarpal",
		"Wrist/LittleMetacarpal/LittleProximal",
		"Wrist/LittleMetacarpal/LittleProximal/LittleIntermediate",
		"Wrist/LittleMetacarpal/LittleProximal/LittleIntermediate/LittleDistal",
		"Wrist/LittleMetacarpal/LittleProximal/LittleIntermediate/LittleDistal/LittleTip",
	};

	// We cast to spatials which should allow us to use any subclass of that.
	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		joints[i] = Object::cast_to<Spatial>(get_node(node_names[i]));
		if (joints[i] == NULL) {
			printf("Couldn't obtain joint for %s\n", node_names[i]);
		}
	}
}

void OpenXRHand::_physics_process(float delta) {
	if (openxr_api == NULL) {
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

	const HandTracker *hand_tracker = openxr_api->get_hand_tracker(hand);
	const float ws = ARVRServer::get_singleton()->get_world_scale();

	if (hand_tracker->is_initialised && hand_tracker->locations.isActive) {
		for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
			if (joints[i] != NULL) {
				const XrPosef &pose = hand_tracker->joint_locations[i].pose;

				Transform t = openxr_api->transform_from_pose(pose, ws);
				if (parents[i] != -1) {
					int parent = parents[i];
					if (joints[parent] != NULL) {
						Transform inv_p = joints[parent]->get_global_transform().inverse();
						t = inv_p * t;
					}
				}

				// and set our local transform, note that our for our palm we use our global tracking position which relates it to the XR origin point
				joints[i]->set_transform(t);
			}
		}

		// show it
		set_visible(true);
	} else {
		// hide it
		set_visible(false);
	}
}

int OpenXRHand::get_hand() const {
	if (hand == OpenXRApi::HAND_LEFT) {
		return 0;
	} else if (hand == OpenXRApi::HAND_RIGHT) {
		return 1;
	} else {
		return 0;
	}
}

void OpenXRHand::set_hand(int p_hand) {
	if (p_hand == 1) {
		hand = OpenXRApi::HAND_RIGHT;
	} else {
		hand = OpenXRApi::HAND_LEFT;
	}
}
