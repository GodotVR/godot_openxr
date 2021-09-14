/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR ActionSets resource

#include "gdclasses/actions/OpenXRActionSets.h"

#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

void OpenXRActionSets::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_action_set", "action_set"), &OpenXRActionSets::add_action_set);
	ClassDB::bind_method(D_METHOD("remove_action_set", "action_set"), &OpenXRActionSets::remove_action_set);
	ClassDB::bind_method(D_METHOD("number_of_action_sets"), &OpenXRActionSets::number_of_action_sets);
	ClassDB::bind_method(D_METHOD("get_action_set", "index"), &OpenXRActionSets::get_action_set);

	ClassDB::bind_method(D_METHOD("add_interaction_profile", "interaction_profile"), &OpenXRActionSets::add_interaction_profile);
	ClassDB::bind_method(D_METHOD("remove_interaction_profile", "interaction_profile"), &OpenXRActionSets::remove_interaction_profile);
	ClassDB::bind_method(D_METHOD("number_of_interaction_profiles"), &OpenXRActionSets::number_of_interaction_profiles);
	ClassDB::bind_method(D_METHOD("get_interaction_profile", "index"), &OpenXRActionSets::get_interaction_profile);
}

void OpenXRActionSets::clear_action_sets() {
	// Being overly careful here, might be able to just clear the vector but making sure we properly unreference our objects so they get freed up

	while (action_sets.size() > 0) {
		action_sets.back().unref();
		action_sets.pop_back();
	}
}

void OpenXRActionSets::add_action_set(Ref<OpenXRActionSet> p_action_set) {
	// Being overly careful here, might be able to push our new action set directly, but need to investigate if usage count works properly

	// Add an unreferenced entry at the back
	Ref<OpenXRActionSet> new_ref;
	action_sets.push_back(new_ref);

	// Then set our reference
	action_sets.back() = p_action_set;
}

void OpenXRActionSets::remove_action_set(Ref<OpenXRActionSet> p_action_set) {
	for(int i = 0; i < action_sets.size(); i++) {
		if (action_sets[i] == p_action_set) {
			action_sets[i].unref();
			action_sets.erase(action_sets.begin() + i);
			return;
		}
	}
}

int OpenXRActionSets::number_of_action_sets() const {
	return action_sets.size();
}

Ref<OpenXRActionSet> OpenXRActionSets::get_action_set(int p_index) const {
	if (p_index >= 0 && p_index < action_sets.size()) {
		return action_sets[p_index];
	} else {
		return Ref<OpenXRActionSet>();
	}
}

void OpenXRActionSets::clear_interaction_profiles() {
	// Being overly careful here, might be able to just clear the vector but making sure we properly unreference our objects so they get freed up

	while (interaction_profiles.size() > 0) {
		interaction_profiles.back().unref();
		interaction_profiles.pop_back();
	}
}

void OpenXRActionSets::add_interaction_profile(Ref<OpenXRInteractionProfile> p_interaction_profile) {
	// Being overly careful here, might be able to push our new action set directly, but need to investigate if usage count works properly

	// Add an unreferenced entry at the back
	Ref<OpenXRInteractionProfile> new_ref;
	interaction_profiles.push_back(new_ref);

	// Then set our reference
	interaction_profiles.back() = p_interaction_profile;
}

void OpenXRActionSets::remove_interaction_profile(Ref<OpenXRInteractionProfile> p_interaction_profile) {
	for(int i = 0; i < interaction_profiles.size(); i++) {
		if (interaction_profiles[i] == p_interaction_profile) {
			interaction_profiles[i].unref();
			interaction_profiles.erase(interaction_profiles.begin() + i);
			return;
		}
	}
}

int OpenXRActionSets::number_of_interaction_profiles() const {
	return interaction_profiles.size();
}

Ref<OpenXRInteractionProfile> OpenXRActionSets::get_interaction_profile(int p_index) const {
	if (p_index >= 0 && p_index < interaction_profiles.size()) {
		return interaction_profiles[p_index];
	} else {
		return Ref<OpenXRInteractionProfile>();
	}
}

/* this needs https://github.com/godotengine/godot-cpp/pull/656
Array OpenXRActionSets::_get_property_list() {
	// We are returning hidden entries for our action sets and interaction profiles so they get saved into our resource file
	// These are not for direct use.

	Array arr;

	// TODO might want to do something here to remove unreferenced entries in action_sets and interaction_profiles?

	// add an entry for every actionset we have
	for(int i = 0; i < action_sets.size(); i++) {
		Dictionary property;

		String name = "action_sets/";
		name += String::num((double) i, 0);

		property["name"] = name;
		property["type"] = Variant::OBJECT;
		property["hint"] = PROPERTY_HINT_RESOURCE_TYPE;
		property["hint_string"] = "OpenXRActionSet";
		property["usage"] = PROPERTY_USAGE_NO_EDITOR; // don't show in editor, only load from/save to tscn

		arr.push_back(property);
	}

	// add an entry for every interaction profile we have
	for(int i = 0; i < interaction_profiles.size(); i++) {
		Dictionary property;

		String name = "interaction_profile/";
		name += String::num((double) i, 0);

		property["name"] = name;
		property["type"] = Variant::OBJECT;
		property["hint"] = PROPERTY_HINT_RESOURCE_TYPE;
		property["hint_string"] = "OpenXRInteractionProfile";
		property["usage"] = PROPERTY_USAGE_NO_EDITOR; // don't show in editor, only load from/save to tscn

		arr.push_back(property);
	}

	return arr;
}

Variant OpenXRActionSets::_get(const String p_name) const {
	Variant ret;

	String str_action_sets = "action_sets/";
	String str_interaction_profiles = "interaction_profiles/";

	if (p_name.begins_with(str_action_sets)) {
		String index = p_name.split("/")[1];
		int id = index.to_int();

		return Variant(get_action_set(id));
	} else if (p_name.begins_with(str_interaction_profiles)) {
		String index = p_name.split("/")[1];
		int id = index.to_int();

		return Variant(get_interaction_profile(id));
	}

	// Must be a property of our superclass, returning an empty (NIL) variant will handle it further
	return ret;
}

bool OpenXRActionSets::_set(const String p_name, Variant p_value) {
	// Note that this is basically only called when we're loading an action sets resource from disk

	String str_action_sets = "action_sets/";
	String str_interaction_profiles = "interaction_profiles/";

	if (p_name.begins_with(str_action_sets)) {
		Ref<OpenXRActionSet> action_set = p_value;

		if (action_set.is_valid()) {
			String index = p_name.split("/")[1];
			int id = index.to_int();

			if (action_sets.size() < id - 1) {
				action_sets.resize(id + 1);
			}

			action_sets[id] = action_set;
		}

		return true;
	} else if (p_name.begins_with(str_interaction_profiles)) {
		Ref<OpenXRInteractionProfile> interaction_profile = p_value;

		if (interaction_profiles.is_valid()) {
			String index = p_name.split("/")[1];
			int id = index.to_int();

			if (interaction_profiles.size() < id - 1) {
				interaction_profiles.resize(id + 1);
			}

			interaction_profiles[id] = interaction_profile;
		}

		return true;
	}


	// Must be a property of our superclass, returning false will handle it further
	return false;
}
*/

OpenXRActionSets::OpenXRActionSets() {
	// nothing to do here yet...
}

OpenXRActionSets::~OpenXRActionSets() {
	clear_action_sets();
}
