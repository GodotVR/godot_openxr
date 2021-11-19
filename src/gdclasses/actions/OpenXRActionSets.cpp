/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR ActionSets resource

#include "gdclasses/actions/OpenXRActionSets.h"

#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

void OpenXRActionSets::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_action_set", "action_set"), &OpenXRActionSets::add_action_set);
	ClassDB::bind_method(D_METHOD("remove_action_set", "action_set"), &OpenXRActionSets::remove_action_set);
	ClassDB::bind_method(D_METHOD("get_action_sets"), &OpenXRActionSets::get_action_sets);
	ClassDB::bind_method(D_METHOD("set_action_sets", "sets"), &OpenXRActionSets::set_action_sets);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "action_sets", PROPERTY_HINT_RESOURCE_TYPE, "OpenXRActionSet", PROPERTY_USAGE_NO_EDITOR), "set_action_sets", "get_action_sets");

	ClassDB::bind_method(D_METHOD("add_interaction_profile", "interaction_profile"), &OpenXRActionSets::add_interaction_profile);
	ClassDB::bind_method(D_METHOD("remove_interaction_profile", "interaction_profile"), &OpenXRActionSets::remove_interaction_profile);
	ClassDB::bind_method(D_METHOD("get_interaction_profiles"), &OpenXRActionSets::get_interaction_profiles);
	ClassDB::bind_method(D_METHOD("set_interaction_profiles", "profiles"), &OpenXRActionSets::set_interaction_profiles);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "interaction_profiles", PROPERTY_HINT_RESOURCE_TYPE, "OpenXRInteractionProfile", PROPERTY_USAGE_NO_EDITOR), "set_interaction_profiles", "get_interaction_profiles");

	ADD_SIGNAL(MethodInfo("action_sets_changed"));
	ADD_SIGNAL(MethodInfo("interaction_profiles_changed"));
}

void OpenXRActionSets::clear_action_sets() {
	// Being overly careful here, might be able to just clear the vector but making sure we properly unreference our objects so they get freed up

	while (action_sets.size() > 0) {
		action_sets.back().unref();
		action_sets.pop_back();
	}

	emit_signal("action_sets_changed");
	notify_property_list_changed();
}

void OpenXRActionSets::add_action_set(Ref<OpenXRActionSet> p_action_set) {
	// Being overly careful here, might be able to push our new action set directly, but need to investigate if usage count works properly

	// Add an unreferenced entry at the back
	Ref<OpenXRActionSet> new_ref;
	action_sets.push_back(new_ref);

	// Then set our reference
	action_sets.back() = p_action_set;

	emit_signal("action_sets_changed");
	notify_property_list_changed();
}

void OpenXRActionSets::remove_action_set(Ref<OpenXRActionSet> p_action_set) {
	for(int i = 0; i < action_sets.size(); i++) {
		if (action_sets[i] == p_action_set) {
			action_sets[i].unref();
			action_sets.erase(action_sets.begin() + i);

			emit_signal("action_sets_changed");
			notify_property_list_changed();
			return;
		}
	}
}

Array OpenXRActionSets::get_action_sets() const {
	Array arr;

	for (auto action_set : action_sets) {
		Variant var = action_set;
		arr.push_back(var);
	}	

	return arr;
}

void OpenXRActionSets::set_action_sets(Array p_action_sets) {
	// In theory this setter should only be used when our resource is loaded.
	// We should add a sort at the end.

	while (action_sets.size() > 0) {
		action_sets.back().unref();
		action_sets.pop_back();
	}

	for (int i = 0; i < p_action_sets.size(); i++) {
		Ref<OpenXRActionSet> action_set;

		action_set = p_action_sets[i];
		action_sets.push_back(action_set);
	}

	emit_signal("action_sets_changed");
	notify_property_list_changed();
}

void OpenXRActionSets::clear_interaction_profiles() {
	// Being overly careful here, might be able to just clear the vector but making sure we properly unreference our objects so they get freed up

	while (interaction_profiles.size() > 0) {
		interaction_profiles.back().unref();
		interaction_profiles.pop_back();
	}

	emit_signal("interaction_profiles_changed");
	notify_property_list_changed();
}

void OpenXRActionSets::add_interaction_profile(Ref<OpenXRInteractionProfile> p_interaction_profile) {
	// Being overly careful here, might be able to push our new action set directly, but need to investigate if usage count works properly

	// Add an unreferenced entry at the back
	Ref<OpenXRInteractionProfile> new_ref;
	interaction_profiles.push_back(new_ref);

	// Then set our reference
	interaction_profiles.back() = p_interaction_profile;

	emit_signal("interaction_profiles_changed");
	notify_property_list_changed();
}

void OpenXRActionSets::remove_interaction_profile(Ref<OpenXRInteractionProfile> p_interaction_profile) {
	for(int i = 0; i < interaction_profiles.size(); i++) {
		if (interaction_profiles[i] == p_interaction_profile) {
			interaction_profiles[i].unref();
			interaction_profiles.erase(interaction_profiles.begin() + i);

			emit_signal("interaction_profiles_changed");
			notify_property_list_changed();
			return;
		}
	}
}

Array OpenXRActionSets::get_interaction_profiles() const {
	Array arr;

	for (auto interaction_profile : interaction_profiles) {
		Variant var = interaction_profile;
		arr.push_back(var);
	}	

	return arr;
}

void OpenXRActionSets::set_interaction_profiles(Array p_interaction_profiles) {
	// In theory this setter should only be used when our resource is loaded.
	// We should add a sort at the end.

	while (interaction_profiles.size() > 0) {
		interaction_profiles.back().unref();
		interaction_profiles.pop_back();
	}

	for (int i = 0; i < p_interaction_profiles.size(); i++) {
		Ref<OpenXRInteractionProfile> interaction_profile;

		interaction_profile = p_interaction_profiles[i];
		interaction_profiles.push_back(interaction_profile);
	}

	emit_signal("interaction_profiles_changed");
	notify_property_list_changed();
}

OpenXRActionSets::OpenXRActionSets() {
	// nothing to do here yet...
}

OpenXRActionSets::~OpenXRActionSets() {
	clear_action_sets();
}
