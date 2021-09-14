/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR Interaction Profile resource

#include "gdclasses/actions/OpenXRInteractionProfile.h"

#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

void OpenXRInteractionProfile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_path", "path"), &OpenXRInteractionProfile::set_path);
	ClassDB::bind_method(D_METHOD("get_path"), &OpenXRInteractionProfile::get_path);

	ClassDB::bind_method(D_METHOD("add_binding", "binding"), &OpenXRInteractionProfile::add_binding);
	ClassDB::bind_method(D_METHOD("remove_binding", "binding"), &OpenXRInteractionProfile::remove_binding);
	ClassDB::bind_method(D_METHOD("number_of_bindings"), &OpenXRInteractionProfile::number_of_bindings);
	ClassDB::bind_method(D_METHOD("get_binding", "index"), &OpenXRInteractionProfile::get_binding);

	// TODO need to move these into _get_property_list
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "path"), "set_path", "get_path");
}

void OpenXRInteractionProfile::set_path(const String &p_path) {
	path = p_path;
}

String OpenXRInteractionProfile::get_path() const {
	return path;
}

void OpenXRInteractionProfile::clear_bindings() {
	// Being overly careful here, might be able to just clear the vector but making sure we properly unreference our objects so they get freed up

	while (bindings.size() > 0) {
		bindings.back().unref();
		bindings.pop_back();
	}
}

void OpenXRInteractionProfile::add_binding(Ref<OpenXRBinding> p_binding) {
	// Being overly careful here, might be able to push our new action set directly, but need to investigate if usage count works properly

	// Add an unreferenced entry at the back
	Ref<OpenXRBinding> new_ref;
	bindings.push_back(new_ref);

	// Then set our reference
	bindings.back() = p_binding;
}

void OpenXRInteractionProfile::remove_binding(Ref<OpenXRBinding> p_binding) {
	for(int i = 0; i < bindings.size(); i++) {
		if (bindings[i] == p_binding) {
			bindings[i].unref();
			bindings.erase(bindings.begin() + i);
			return;
		}
	}
}

int OpenXRInteractionProfile::number_of_bindings() const {
	return bindings.size();
}

Ref<OpenXRBinding> OpenXRInteractionProfile::get_binding(int p_index) const {
	if (p_index >= 0 && p_index < bindings.size()) {
		return bindings[p_index];
	} else {
		return Ref<OpenXRBinding>();
	}
}

/* this needs https://github.com/godotengine/godot-cpp/pull/656
Array OpenXRInteractionProfile::_get_property_list() {
	Array arr;

	{
		Dictionary property;

		property["name"] = "path";
		property["type"] = Variant::STRING;
		property["hint"] = 0;
		property["hint_string"] = "";
		property["usage"] = 0;

		arr.push_back(property);
	}

	// We are returning hidden entries for our bindings so they get saved into our resource file
	// These are not for direct use.

	// add an entry for every binding we have
	for(int i = 0; i < bindings.size(); i++) {
		Dictionary property;

		String name = "bindings/";
		name += String::num((double) i, 0);

		property["name"] = name;
		property["type"] = Variant::OBJECT;
		property["hint"] = PROPERTY_HINT_RESOURCE_TYPE;
		property["hint_string"] = "OpenXRBinding";
		property["usage"] = PROPERTY_USAGE_NO_EDITOR; // don't show in editor, only load from/save to tscn

		arr.push_back(property);
	}

	return arr;
}

Variant OpenXRInteractionProfile::_get(const String p_name) const {
	Variant ret;

	String str_bindings = "bindings/";

	if (p_name == "path") {
		return Variant(get_path());
	} else if (p_name.begins_with(str_bindings)) {
		String index = p_name.split("/")[1];
		int id = index.to_int();

		return Variant(get_binding(id));
	}

	// Must be a property of our superclass, returning an empty (NIL) variant will handle it further
	return ret;
}

bool OpenXRInteractionProfile::_set(const String p_name, Variant p_value) {
	String str_bindings = "bindings/";

	if (p_name == "path") {
		String value = p_value;
		set_path(value);
		return true;
	} else if (p_name.begins_with(str_bindings)) {
		Ref<OpenXRBinding> binding = p_value;

		if (binding.is_valid()) {
			String index = p_name.split("/")[1];
			int id = index.to_int();

			if (bindings.size() < id - 1) {
				bindings.resize(id + 1);
			}

			bindings[id] = binding;
		}

		return true;
	}

	return false;
}
*/

OpenXRInteractionProfile::OpenXRInteractionProfile() {

}

OpenXRInteractionProfile::~OpenXRInteractionProfile() {

}
