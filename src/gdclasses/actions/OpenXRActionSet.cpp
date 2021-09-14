/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR ActionSet resource

#include "gdclasses/actions/OpenXRActionSet.h"

#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

void OpenXRActionSet::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_name", "name"), &OpenXRActionSet::set_name);
	ClassDB::bind_method(D_METHOD("get_name"), &OpenXRActionSet::get_name);

	ClassDB::bind_method(D_METHOD("set_localized_name", "name"), &OpenXRActionSet::set_localized_name);
	ClassDB::bind_method(D_METHOD("get_localized_name"), &OpenXRActionSet::get_localized_name);

	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &OpenXRActionSet::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &OpenXRActionSet::get_priority);

	ClassDB::bind_method(D_METHOD("add_action", "action"), &OpenXRActionSet::add_action);
	ClassDB::bind_method(D_METHOD("remove_action", "action"), &OpenXRActionSet::remove_action);
	ClassDB::bind_method(D_METHOD("number_of_actions"), &OpenXRActionSet::number_of_actions);
	ClassDB::bind_method(D_METHOD("get_action", "index"), &OpenXRActionSet::get_action);

	// TODO need to move these into _get_property_list
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "localized_name"), "set_localized_name", "get_localized_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");
}

void OpenXRActionSet::set_name(const String &p_name) {
	name = p_name;
}

String OpenXRActionSet::get_name() const {
	return name;
}

void OpenXRActionSet::set_localized_name(const String &p_name) {
	localized_name = p_name;
}

String OpenXRActionSet::get_localized_name() const {
	return localized_name;
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
}

void OpenXRActionSet::add_action(Ref<OpenXRAction> p_action) {
	// Being overly careful here, might be able to push our new action set directly, but need to investigate if usage count works properly

	// Add an unreferenced entry at the back
	Ref<OpenXRAction> new_ref;
	actions.push_back(new_ref);

	// Then set our reference
	actions.back() = p_action;
}

void OpenXRActionSet::remove_action(Ref<OpenXRAction> p_action) {
	for(int i = 0; i < actions.size(); i++) {
		if (actions[i] == p_action) {
			actions[i].unref();
			actions.erase(actions.begin() + i);
			return;
		}
	}
}

int OpenXRActionSet::number_of_actions() const {
	return actions.size();
}

Ref<OpenXRAction> OpenXRActionSet::get_action(int p_index) const {
	if (p_index >= 0 && p_index < actions.size()) {
		return actions[p_index];
	} else {
		return Ref<OpenXRAction>();
	}
}

/* this needs https://github.com/godotengine/godot-cpp/pull/656
Array OpenXRActionSet::_get_property_list() {
	Array arr;

	{
		Dictionary property;

		property["name"] = "name";
		property["type"] = Variant::STRING;
		property["hint"] = 0;
		property["hint_string"] = "";
		property["usage"] = 0;

		arr.push_back(property);
	}

	{
		Dictionary property;

		property["name"] = "localized_name";
		property["type"] = Variant::STRING;
		property["hint"] = 0;
		property["hint_string"] = "";
		property["usage"] = 0;

		arr.push_back(property);
	}

	{
		Dictionary property;

		property["name"] = "priority";
		property["type"] = Variant::INT;
		property["hint"] = 0;
		property["hint_string"] = "";
		property["usage"] = 0;

		arr.push_back(property);
	}

	// We are returning hidden entries for our actions so they get saved into our resource file
	// These are not for direct use.

	// add an entry for every action we have
	for(int i = 0; i < actions.size(); i++) {
		Dictionary property;

		String name = "actions/";
		name += String::num((double) i, 0);

		property["name"] = name;
		property["type"] = Variant::OBJECT;
		property["hint"] = PROPERTY_HINT_RESOURCE_TYPE;
		property["hint_string"] = "OpenXRAction";
		property["usage"] = PROPERTY_USAGE_NO_EDITOR; // don't show in editor, only load from/save to tscn

		arr.push_back(property);
	}

	return arr;
}

Variant OpenXRActionSet::_get(const String p_name) const {
	Variant ret;

	String str_actions = "actions/";

	if (p_name == "name") {
		return Variant(get_name());
	} else if (p_name == "localized_name") {
		return Variant(get_localized_name());
	} else if (p_name == "priority") {
		return Variant(get_priority());
	} else if (p_name.begins_with(str_actions)) {
		String index = p_name.split("/")[1];
		int id = index.to_int();

		return Variant(get_action_set(id));
	}

	// Must be a property of our superclass, returning an empty (NIL) variant will handle it further
	return ret;
}

bool OpenXRActionSet::_set(const String p_name, Variant p_value) {
	String str_actions = "actions/";

	if (p_name == "name") {
		String value = p_value;
		set_name(value);
		return true;
	} else if (p_name == "localized_name") {
		String value = p_value;
		set_localized_name(value);
		return true;
	} else if (p_name == "priority") {
		int value = p_value;
		set_priority(value);
		return true;
	} else if (p_name.begins_with(str_actions)) {
		Ref<OpenXRAction> action = p_value;

		if (action.is_valid()) {
			String index = p_name.split("/")[1];
			int id = index.to_int();

			if (actions.size() < id - 1) {
				actions.resize(id + 1);
			}

			actions[id] = action;
		}

		return true;
	}

	return false;
}
*/

OpenXRActionSet::OpenXRActionSet() {

}

OpenXRActionSet::~OpenXRActionSet() {

}
