/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR pose GDNative object, this exposes specific locations we track

#ifndef OPENXR_POSE_H
#define OPENXR_POSE_H

#include "openxr/OpenXRApi.h"
#include <Spatial.hpp>

namespace godot {
class OpenXRPose : public Spatial {
	GODOT_CLASS(OpenXRPose, Spatial)

private:
	OpenXRApi *openxr_api;
	bool invisible_if_inactive = true;
	String action;
	String path;

	// cache action and path
	bool fail_cache = false;
	Action *_action;
	XrPath _path;
	bool check_action_and_path();

public:
	static void _register_methods();

	void _init();
	void _physics_process(float delta);

	OpenXRPose();
	~OpenXRPose();

	bool is_active();

	bool get_invisible_if_inactive() const;
	void set_invisible_if_inactive(bool hide);

	String get_action() const;
	void set_action(const String p_action);

	String get_path() const;
	void set_path(const String p_path);
};
} // namespace godot

#endif // !OPENXR_POSE_H
