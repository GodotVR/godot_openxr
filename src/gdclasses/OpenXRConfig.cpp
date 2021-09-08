/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR config GDNative object

#include "gdclasses/OpenXRConfig.h"
#include <Dictionary.hpp>
#include <GlobalConstants.hpp>

using namespace godot;

void OpenXRConfig::_register_methods() {
	register_method("keep_3d_linear", &OpenXRConfig::keep_3d_linear);

	register_method("get_form_factor", &OpenXRConfig::get_form_factor);
	register_method("set_form_factor", &OpenXRConfig::set_form_factor);
	register_property<OpenXRConfig, int>("form_factor", &OpenXRConfig::set_form_factor, &OpenXRConfig::get_form_factor, 1, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "Not set,HMD,Hand Held");

	register_method("get_action_sets", &OpenXRConfig::get_action_sets);
	register_method("set_action_sets", &OpenXRConfig::set_action_sets);
	register_property<OpenXRConfig, String>("action_sets", &OpenXRConfig::set_action_sets, &OpenXRConfig::get_action_sets, String(OpenXRApi::default_action_sets_json), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_MULTILINE_TEXT);

	register_method("get_interaction_profiles", &OpenXRConfig::get_interaction_profiles);
	register_method("set_interaction_profiles", &OpenXRConfig::set_interaction_profiles);
	register_property<OpenXRConfig, String>("interaction_profiles", &OpenXRConfig::set_interaction_profiles, &OpenXRConfig::get_interaction_profiles, String(OpenXRApi::default_interaction_profiles_json), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_MULTILINE_TEXT);
}

OpenXRConfig::OpenXRConfig() {
	openxr_api = OpenXRApi::openxr_get_api();
}

OpenXRConfig::~OpenXRConfig() {
	if (openxr_api != NULL) {
		OpenXRApi::openxr_release_api();
	}
}

void OpenXRConfig::_init() {
	// nothing to do here
}

bool OpenXRConfig::keep_3d_linear() const {
	if (openxr_api == NULL) {
		return false;
	} else {
		return openxr_api->get_keep_3d_linear();
	}
}

int OpenXRConfig::get_form_factor() const {
	if (openxr_api == NULL) {
		return 0;
	} else {
		return (int)openxr_api->get_form_factor();
	}
}

void OpenXRConfig::set_form_factor(int p_form_factor) {
	if (openxr_api == NULL) {
		Godot::print("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_form_factor((XrFormFactor)p_form_factor);
	}
}

String OpenXRConfig::get_action_sets() const {
	if (openxr_api == NULL) {
		return String();
	} else {
		return openxr_api->get_action_sets_json();
	}
}

void OpenXRConfig::set_action_sets(const String p_action_sets) {
	if (openxr_api == NULL) {
		Godot::print("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_action_sets_json(p_action_sets);
	}
}

String OpenXRConfig::get_interaction_profiles() const {
	if (openxr_api == NULL) {
		return String();
	} else {
		return openxr_api->get_interaction_profiles_json();
	}
}

void OpenXRConfig::set_interaction_profiles(const String p_interaction_profiles) {
	if (openxr_api == NULL) {
		Godot::print("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_interaction_profiles_json(p_interaction_profiles);
	}
}
