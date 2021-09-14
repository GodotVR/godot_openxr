/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR config GDNative object

#include "gdclasses/OpenXRConfig.h"
#include <Dictionary.hpp>
#include <GlobalConstants.hpp>

using namespace godot;

void OpenXRConfig::_register_methods() {
	// In Godot 4 we'll move these into XRInterfaceOpenXR and add groups to our properties

	register_method("keep_3d_linear", &OpenXRConfig::keep_3d_linear);

	register_method("get_view_config_type", &OpenXRConfig::get_view_config_type);
	register_method("set_view_config_type", &OpenXRConfig::set_view_config_type);
	register_property<OpenXRConfig, int>("view_configuration", &OpenXRConfig::set_view_config_type, &OpenXRConfig::get_view_config_type, 1, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "Mono,Stereo");

	register_method("get_form_factor", &OpenXRConfig::get_form_factor);
	register_method("set_form_factor", &OpenXRConfig::set_form_factor);
	register_property<OpenXRConfig, int>("form_factor", &OpenXRConfig::set_form_factor, &OpenXRConfig::get_form_factor, 1, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "Not set,HMD,Hand Held");

	register_method("get_color_space", &OpenXRConfig::get_color_space);
	register_method("set_color_space", &OpenXRConfig::set_color_space);
	register_property<OpenXRConfig, int>("color_space", &OpenXRConfig::set_color_space, &OpenXRConfig::get_color_space, 1, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_NOEDITOR);
	register_method("get_available_color_spaces", &OpenXRConfig::get_available_color_spaces);

	register_method("get_refresh_rate", &OpenXRConfig::get_refresh_rate);
	register_method("set_refresh_rate", &OpenXRConfig::set_refresh_rate);
	register_property<OpenXRConfig, double>("refresh_rate", &OpenXRConfig::set_refresh_rate, &OpenXRConfig::get_refresh_rate, 1, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_NOEDITOR);
	register_method("get_available_refresh_rates", &OpenXRConfig::get_available_refresh_rates);

	register_method("get_enabled_extensions", &OpenXRConfig::get_enabled_extensions);

	register_method("get_action_sets", &OpenXRConfig::get_action_sets);
	register_method("set_action_sets", &OpenXRConfig::set_action_sets);
	register_property<OpenXRConfig, String>("action_sets", &OpenXRConfig::set_action_sets, &OpenXRConfig::get_action_sets, String(OpenXRApi::default_action_sets_json), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_MULTILINE_TEXT);

	register_method("get_interaction_profiles", &OpenXRConfig::get_interaction_profiles);
	register_method("set_interaction_profiles", &OpenXRConfig::set_interaction_profiles);
	register_property<OpenXRConfig, String>("interaction_profiles", &OpenXRConfig::set_interaction_profiles, &OpenXRConfig::get_interaction_profiles, String(OpenXRApi::default_interaction_profiles_json), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_MULTILINE_TEXT);
}

OpenXRConfig::OpenXRConfig() {
	openxr_api = OpenXRApi::openxr_get_api();
	color_space_wrapper = XRFbColorSpaceExtensionWrapper::get_singleton();
	display_refresh_rate_wrapper = XRFbDisplayRefreshRateExtensionWrapper::get_singleton();
}

OpenXRConfig::~OpenXRConfig() {
	if (openxr_api != NULL) {
		OpenXRApi::openxr_release_api();
	}
	color_space_wrapper = nullptr;
	display_refresh_rate_wrapper = nullptr;
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

int OpenXRConfig::get_view_config_type() const {
	if (openxr_api == NULL) {
		return 1;
	} else {
		XrViewConfigurationType config_type = openxr_api->get_view_configuration_type();
		switch (config_type) {
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO: {
				return 0;
			}; break;
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO: {
				return 1;
			}; break;
			/* These are not (yet) supported, adding them in here for future reference
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO: {
				return 2;
			}; break;
			case XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT: {
				return 3;
			}; break;
			*/
			default: {
				// we don't support the others
				Godot::print_error(String("Unsupported configuration type set: ") + String::num_int64(config_type), __FUNCTION__, __FILE__, __LINE__);
				return 1;
			}; break;
		}
	}
}

void OpenXRConfig::set_view_config_type(const int p_config_type) {
	// TODO may need to add something here that this is read only after initialisation
	if (openxr_api == NULL) {
		Godot::print("OpenXR object wasn't constructed.");
	} else {
		switch (p_config_type) {
			case 0: {
				openxr_api->set_view_configuration_type(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO);
			}; break;
			case 1: {
				openxr_api->set_view_configuration_type(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO);
			}; break;
			default: {
				// Shouldn't be set, just give error.
				Godot::print_error(String("Unsupported configuration type set: ") + String::num_int64(p_config_type), __FUNCTION__, __FILE__, __LINE__);
			}; break;
		}
	}
}

int OpenXRConfig::get_form_factor() const {
	if (openxr_api == NULL) {
		return 0;
	} else {
		return (int)openxr_api->get_form_factor();
	}
}

void OpenXRConfig::set_form_factor(const int p_form_factor) {
	if (openxr_api == NULL) {
		Godot::print("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_form_factor((XrFormFactor)p_form_factor);
	}
}

int OpenXRConfig::get_color_space() const {
	if (color_space_wrapper == nullptr) {
		return 0;
	} else {
		return (int)color_space_wrapper->get_color_space();
	}
}

void OpenXRConfig::set_color_space(const int p_color_space) {
	if (color_space_wrapper != nullptr) {
		color_space_wrapper->set_color_space((uint32_t)p_color_space);
	}
}

godot::Dictionary OpenXRConfig::get_available_color_spaces() {
	if (color_space_wrapper != nullptr) {
		return color_space_wrapper->get_available_color_spaces();
	} else {
		return godot::Dictionary();
	}
}

double OpenXRConfig::get_refresh_rate() const {
	if (display_refresh_rate_wrapper == nullptr) {
		return 0;
	} else {
		return display_refresh_rate_wrapper->get_refresh_rate();
	}
}

void OpenXRConfig::set_refresh_rate(const double p_refresh_rate) {
	if (display_refresh_rate_wrapper != nullptr) {
		display_refresh_rate_wrapper->set_refresh_rate(p_refresh_rate);
	}
}

godot::Array OpenXRConfig::get_available_refresh_rates() const {
	if (display_refresh_rate_wrapper == nullptr) {
		godot::Array arr;
		return arr;
	} else {
		return display_refresh_rate_wrapper->get_available_refresh_rates();
	}
}

godot::Array OpenXRConfig::get_enabled_extensions() const {
	if (openxr_api == NULL) {
		godot::Array arr;
		return arr;
	} else {
		return openxr_api->get_enabled_extensions();
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
