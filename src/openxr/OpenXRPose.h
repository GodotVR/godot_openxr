/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR pose GDNative object, this exposes specific locations we track

#ifndef OPENXR_POSE_H
#define OPENXR_POSE_H

#include "OpenXRApi.h"
#include <Spatial.hpp>

namespace godot {
class OpenXRPose : public Spatial {
	GODOT_CLASS(OpenXRPose, Spatial)

	// For now hardcoded enums but this may need to become string based, we'll see.
	enum Poses {
		POSE_LEFT_HAND,
		POSE_RIGHT_HAND,
		POSE_MAX
	};

private:
	OpenXRApi *openxr_api;
	Poses pose;

public:
	static void _register_methods();

	void _init();
	void _physics_process(float delta);

	OpenXRPose();
	~OpenXRPose();

	int get_pose() const;
	void set_pose(int p_pose);
};
} // namespace godot

#endif // !OPENXR_POSE_H
