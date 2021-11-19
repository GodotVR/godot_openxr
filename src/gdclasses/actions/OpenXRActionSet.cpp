/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR ActionSet resource

#include "gdclasses/actions/OpenXRActionSet.h"

#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

void OpenXRActionSet::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_name", "name"), &OpenXRActionSet::set_name);
	ClassDB::bind_method(D_METHOD("get_name"), &OpenXRActionSet::get_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");

	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &OpenXRActionSet::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &OpenXRActionSet::get_priority);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");

	ClassDB::bind_method(D_METHOD("add_action", "action"), &OpenXRActionSet::add_action);
	ClassDB::bind_method(D_METHOD("remove_action", "action"), &OpenXRActionSet::remove_action);
	ClassDB::bind_method(D_METHOD("get_actions"), &OpenXRActionSet::get_actions);
	ClassDB::bind_method(D_METHOD("set_actions", "sets"), &OpenXRActionSet::set_actions);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "actions", PROPERTY_HINT_RESOURCE_TYPE, "OpenXRAction", PROPERTY_USAGE_NO_EDITOR), "set_actions", "get_actions");

	ADD_SIGNAL(MethodInfo("actions_changed"));
}

void OpenXRActionSet::set_name(const String &p_name) {
	name = p_name;
}

String OpenXRActionSet::get_name() const {
	return name;
}

void OpenXRActionSet::set_priority(int p_priority) {
	priority = p_priority;
}

int OpenXRActionSet::get_priority() const {
	return priority;
}

void OpenXRActionSet::clear_actions() {
	// Being overly careful here, might be able to just clear the vector but making sure we properly unreference our objects so they get freed up

	while (actions.size() > 0) {
		actions.back().unref();
		actions.pop_back();
	}

	emit_signal("actions_changed");
	notify_property_list_changed();
}

void OpenXRActionSet::add_action(Ref<OpenXRAction> p_action) {
	// Being overly careful here, might be able to push our new action set directly, but need to investigate if usage count works properly

	// Add an unreferenced entry at the back
	Ref<OpenXRAction> new_ref;
	actions.push_back(new_ref);

	// Then set our reference
	actions.back() = p_action;

	emit_signal("actions_changed");
	notify_property_list_changed();
}

void OpenXRActionSet::remove_action(Ref<OpenXRAction> p_action) {
	for(int i = 0; i < actions.size(); i++) {
		if (actions[i] == p_action) {
			actions[i].unref();
			actions.erase(actions.begin() + i);

			emit_signal("actions_changed");
			notify_property_list_changed();
			return;
		}
	}
}

Array OpenXRActionSet::get_actions() const {
	Array arr;

	for (auto action : actions) {
		Variant var = action;
		arr.push_back(var);
	}	

	return arr;
}

void OpenXRActionSet::set_actions(Array p_actions) {
	// In theory this setter should only be used when our resource is loaded.
	// We should add a sort at the end.

	while (actions.size() > 0) {
		actions.back().unref();
		actions.pop_back();
	}

	for (int i = 0; i < p_actions.size(); i++) {
		Ref<OpenXRAction> action;

		action = p_actions[i];
		actions.push_back(action);
	}

	emit_signal("actions_changed");
	notify_property_list_changed();
}


OpenXRActionSet::OpenXRActionSet() {

}

OpenXRActionSet::~OpenXRActionSet() {

}
