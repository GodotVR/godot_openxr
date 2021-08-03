////////////////////////////////////////////////////////////////////////////////////////////////
// Class for OpenXR Action sets
// Note, included through OpenXRApi.h
//
// ActionSets combine a collection of actions that can be triggered or interacted with
// if the actionset is active.
// We can then use interaction profiles to bind these actions to inputs on various controllers.

#ifndef OPENXR_ACTIONSET_H
#define OPENXR_ACTIONSET_H

class ActionSet {
private:
	bool active = true;
	bool is_attached = false;
	OpenXRApi *xr_api;
	godot::String name;

	// vector with actions
	std::vector<Action *> actions;

	// OpenXR
	XrActionSet handle = XR_NULL_HANDLE;

public:
	ActionSet(OpenXRApi *p_api, const godot::String &p_name, const godot::String &p_localised_name, uint32_t p_priority);
	~ActionSet();

	bool is_active() const;
	void set_active(bool p_is_active);

	godot::String get_name() const;

	Action *add_action(XrActionType p_type, const godot::String &p_name, const godot::String &p_localised_name, int p_toplevel_path_count, const XrPath *p_toplevel_paths);
	Action *get_action(const godot::String &p_name);

	XrActionSet get_action_set();
	bool attach();
	void reset_spaces();
};

#endif /* !OPENXR_ACTIONSET_H */
