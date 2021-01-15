/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR hand GDNative object implemented through skeleton (bones)

#ifndef OPENXR_SKELETON_H
#define OPENXR_SKELETON_H

#include "OpenXRApi.h"
#include <Ref.hpp>
#include <Skeleton.hpp>

namespace godot {
class OpenXRSkeleton : public Skeleton {
	GODOT_CLASS(OpenXRSkeleton, Skeleton)

private:
	OpenXRApi *openxr_api;
	int hand;

	int64_t bones[XR_HAND_JOINT_COUNT_EXT];

public:
	static void _register_methods();

	void _init();
	void _ready();
	void _physics_process(float delta);

	OpenXRSkeleton();
	~OpenXRSkeleton();

	int get_hand() const;
	void set_hand(int p_hand);
};
} // namespace godot

#endif // !OPENXR_SKELETON_H
