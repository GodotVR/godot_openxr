////////////////////////////////////////////////////////////////////////////////////////////////
// Class for OpenXR Actions
// Note, included through OpenXRApi.h
//
// Our action object encapsulates all data and logic related to interaction with actions
// in OpenXR.

#ifndef OPENXR_ACTION_H
#define OPENXR_ACTION_H

class Action {
private:
	OpenXRApi *xr_api;
	XrActionType type = XR_ACTION_TYPE_BOOLEAN_INPUT;
	godot::String name;

	struct path_with_space {
		XrPath toplevel_path;
		XrSpace space;
		bool wasLocationvalid;
	};

	std::vector<path_with_space> toplevel_paths;

	// OpenXR
	XrAction handle = XR_NULL_HANDLE;

public:
	Action(OpenXRApi *p_api, XrActionSet p_action_set, XrActionType p_type, const godot::String &p_name, const godot::String &p_localised_name, int p_toplevel_path_count, const XrPath *p_toplevel_paths);
	~Action();

	void reset_spaces();

	XrActionType get_type() const;
	godot::String get_name() const;

	XrAction get_action() const;
	bool get_as_bool(const XrPath p_path);
	float get_as_float(const XrPath p_path);
	godot::Vector2 get_as_vector(const XrPath p_path);
	bool is_pose_active(const XrPath p_path);
	TrackingConfidence get_as_pose(const XrPath p_path, float p_world_scale, godot::Transform &r_transform);
	void do_haptic_pulse(const XrPath p_path, XrDuration p_duration, float p_frequency, float p_amplitude);
};

#endif /* !OPENXR_ACTION_H */
