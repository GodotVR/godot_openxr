////////////////////////////////////////////////////////////////////////////////////////////////
// Class for OpenXR Action Sets

#include "openxr/OpenXRApi.h"

using namespace godot;

ActionSet::ActionSet(OpenXRApi *p_api, const godot::String &p_name, const godot::String &p_localised_name, uint32_t p_priority) {
	xr_api = p_api;
	name = p_name;

	// create our action set...
	XrActionSetCreateInfo actionSetInfo = {
		.type = XR_TYPE_ACTION_SET_CREATE_INFO,
		.next = nullptr,
		.priority = p_priority
	};
	strcpy(actionSetInfo.actionSetName, name.utf8().get_data());
	strcpy(actionSetInfo.localizedActionSetName, p_localised_name.utf8().get_data());

	XrResult result = xrCreateActionSet(xr_api->instance, &actionSetInfo, &handle);
	if (!xr_api->xr_result(result, "failed to create actionset {0}", name)) {
		return;
	}
}

ActionSet::~ActionSet() {
	// loop through actions in actions set..
	while (!actions.empty()) {
		Action *action = actions.back();
		delete action;
		actions.pop_back();
	}

	if (handle != XR_NULL_HANDLE) {
		xrDestroyActionSet(handle);
	}
}

bool ActionSet::is_active() const {
	return active;
}

void ActionSet::set_active(bool p_is_active) {
	active = p_is_active;
}

godot::String ActionSet::get_name() const {
	return name;
}

Action *ActionSet::add_action(XrActionType p_type, const godot::String &p_name, const godot::String &p_localised_name, int p_toplevel_path_count, const XrPath *p_toplevel_paths) {
	if (is_attached) {
		// can't add an action after we've attached our set to our session
		return nullptr;
	}

	// check if we already have this action
	for (uint64_t i = 0; i < actions.size(); i++) {
		if (actions[i]->get_name() == p_name) {
			return actions[i];
		}
	}

	Action *new_action = new Action(xr_api, handle, p_type, p_name, p_localised_name, p_toplevel_path_count, p_toplevel_paths);
	actions.push_back(new_action);

	return new_action;
}

Action *ActionSet::get_action(const godot::String &p_name) {
	// find this action
	for (uint64_t i = 0; i < actions.size(); i++) {
		if (actions[i]->get_name() == p_name) {
			return actions[i];
		}
	}

	return nullptr;
}

XrActionSet ActionSet::get_action_set() {
	return handle;
}

bool ActionSet::attach() {
	if (is_attached) {
		// already attached to our session
		return true;
	}
	if (handle == XR_NULL_HANDLE) {
		Godot::print_error("Can't attach action set if it has not been created.", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
	if (xr_api->session == XR_NULL_HANDLE) {
		Godot::print_error("Can't attach action set if there is no session.", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	// So according to the docs, once we attach our action set to our session it becomes read only..
	// https://www.khronos.org/registry/OpenXR/specs/1.0/man/html/xrAttachSessionActionSets.html
	XrSessionActionSetsAttachInfo attachInfo = {
		.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
		.next = nullptr,
		.countActionSets = 1,
		.actionSets = &handle
	};

	XrResult result = xrAttachSessionActionSets(xr_api->session, &attachInfo);
	if (!xr_api->xr_result(result, "failed to attach action set")) {
		return false;
	}

	is_attached = true;
	return true;
}

void ActionSet::reset_spaces() {
	for (uint64_t i = 0; i < actions.size(); i++) {
		actions[i]->reset_spaces();
	}
}
