/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR config GDNative object

#include <ARVRServer.hpp>
#include <Dictionary.hpp>
#include <GlobalConstants.hpp>

#include "gdclasses/OpenXRConfig.h"

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

	register_method("get_play_space_type", &OpenXRConfig::get_play_space_type);
	register_method("set_play_space_type", &OpenXRConfig::set_play_space_type);
	register_property<OpenXRConfig, int>("play_space_type", &OpenXRConfig::set_play_space_type, &OpenXRConfig::get_play_space_type, 2, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "View,Local,Stage"); // we don't support XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT and XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO at this time.

	register_method("get_refresh_rate", &OpenXRConfig::get_refresh_rate);
	register_method("set_refresh_rate", &OpenXRConfig::set_refresh_rate);
	register_property<OpenXRConfig, double>("refresh_rate", &OpenXRConfig::set_refresh_rate, &OpenXRConfig::get_refresh_rate, 1, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_NOEDITOR);
	register_method("get_available_refresh_rates", &OpenXRConfig::get_available_refresh_rates);

	register_method("get_enabled_extensions", &OpenXRConfig::get_enabled_extensions);

	register_method("get_tracking_confidence", &OpenXRConfig::get_tracking_confidence);

	register_method("get_action_sets", &OpenXRConfig::get_action_sets);
	register_method("set_action_sets", &OpenXRConfig::set_action_sets);
	register_property<OpenXRConfig, String>("action_sets", &OpenXRConfig::set_action_sets, &OpenXRConfig::get_action_sets, String(OpenXRApi::default_action_sets_json), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_MULTILINE_TEXT);

	register_method("get_interaction_profiles", &OpenXRConfig::get_interaction_profiles);
	register_method("set_interaction_profiles", &OpenXRConfig::set_interaction_profiles);
	register_property<OpenXRConfig, String>("interaction_profiles", &OpenXRConfig::set_interaction_profiles, &OpenXRConfig::get_interaction_profiles, String(OpenXRApi::default_interaction_profiles_json), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_MULTILINE_TEXT);

	register_method("get_system_name", &OpenXRConfig::get_system_name);

	register_method("get_cpu_level", &OpenXRConfig::get_cpu_level);
	register_method("set_cpu_level", &OpenXRConfig::set_cpu_level);
	register_property<OpenXRConfig, int>("cpu_level", &OpenXRConfig::set_cpu_level, &OpenXRConfig::get_cpu_level, XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_NOEDITOR);

	register_method("get_gpu_level", &OpenXRConfig::get_gpu_level);
	register_method("set_gpu_level", &OpenXRConfig::set_gpu_level);
	register_property<OpenXRConfig, int>("gpu_level", &OpenXRConfig::set_gpu_level, &OpenXRConfig::get_gpu_level, XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_NOEDITOR);

	register_method("get_render_target_size_multiplier", &OpenXRConfig::get_render_target_size_multiplier);
	register_method("set_render_target_size_multiplier", &OpenXRConfig::set_render_target_size_multiplier);
	register_property<OpenXRConfig, double>("render_target_size_multiplier", &OpenXRConfig::set_render_target_size_multiplier, &OpenXRConfig::get_render_target_size_multiplier, 1, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_NOEDITOR);

	register_method("set_foveation_level", &OpenXRConfig::set_foveation_level);

	register_method("is_passthrough_supported", &OpenXRConfig::is_passthrough_supported);
	register_method("is_passthrough_enabled", &OpenXRConfig::is_passthrough_enabled);
	register_method("start_passthrough", &OpenXRConfig::start_passthrough);
	register_method("stop_passthrough", &OpenXRConfig::stop_passthrough);

	register_method("get_play_space", &OpenXRConfig::get_play_space);
}

OpenXRConfig::OpenXRConfig() {
	openxr_api = OpenXRApi::openxr_get_api();
	color_space_wrapper = XRFbColorSpaceExtensionWrapper::get_singleton();
	display_refresh_rate_wrapper = XRFbDisplayRefreshRateExtensionWrapper::get_singleton();
	foveation_wrapper = XRFbFoveationExtensionWrapper::get_singleton();
	performance_settings_wrapper = XRExtPerformanceSettingsExtensionWrapper::get_singleton();
	passthrough_wrapper = XRFbPassthroughExtensionWrapper::get_singleton();
	hand_tracking_wrapper = XRExtHandTrackingExtensionWrapper::get_singleton();
}

OpenXRConfig::~OpenXRConfig() {
	if (openxr_api != nullptr) {
		OpenXRApi::openxr_release_api();
	}
	color_space_wrapper = nullptr;
	display_refresh_rate_wrapper = nullptr;
	foveation_wrapper = nullptr;
	performance_settings_wrapper = nullptr;
	passthrough_wrapper = nullptr;
	hand_tracking_wrapper = nullptr;
}

void OpenXRConfig::_init() {
	// nothing to do here
}

bool OpenXRConfig::keep_3d_linear() const {
	if (openxr_api == nullptr) {
		return false;
	} else {
		return openxr_api->get_keep_3d_linear();
	}
}

int OpenXRConfig::get_view_config_type() const {
	if (openxr_api == nullptr) {
		return 1;
	} else {
		XrViewConfigurationType config_type = openxr_api->get_view_configuration_type();
		switch (config_type) {
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO: {
				return VIEW_CONFIGURATION_MONO;
			}; break;
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO: {
				return VIEW_CONFIGURATION_STEREO;
			}; break;
			/* These are not (yet) supported, adding them in here for future reference
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO: {
				return VIEW_CONFIGURATION_QUAD;
			}; break;
			case XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT: {
				return VIEW_CONFIGURATION_FPO;
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
	if (openxr_api == nullptr) {
		Godot::print("OpenXR object wasn't constructed.");
	} else {
		ViewConfigurationType config_type = ViewConfigurationType(p_config_type);
		switch (config_type) {
			case VIEW_CONFIGURATION_MONO: {
				openxr_api->set_view_configuration_type(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO);
			}; break;
			case VIEW_CONFIGURATION_STEREO: {
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
	if (openxr_api == nullptr) {
		return 0;
	} else {
		return (int)openxr_api->get_form_factor();
	}
}

void OpenXRConfig::set_form_factor(const int p_form_factor) {
	if (openxr_api == nullptr) {
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

int OpenXRConfig::get_play_space_type() const {
	if (openxr_api == nullptr) {
		return XR_REFERENCE_SPACE_TYPE_STAGE;
	} else {
		switch (openxr_api->get_play_space_type()) {
			case XR_REFERENCE_SPACE_TYPE_VIEW:
				return PLAY_SPACE_VIEW;
			case XR_REFERENCE_SPACE_TYPE_LOCAL:
				return PLAY_SPACE_LOCAL;
			case XR_REFERENCE_SPACE_TYPE_STAGE:
				return PLAY_SPACE_STAGE;
				//case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT:
				//	return PLAY_SPACE_UNBOUNDED;
				//case XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO:
				//	return PLAY_SPACE_COMBINED_EYE;
			default:
				return 2;
		}
	}
}

void OpenXRConfig::set_play_space_type(const int p_play_space_type) {
	if (openxr_api == nullptr) {
		Godot::print("OpenXR object wasn't constructed.");
	} else {
		PlaySpaceType play_space_type = PlaySpaceType(p_play_space_type);
		switch (play_space_type) {
			case PLAY_SPACE_VIEW: {
				openxr_api->set_play_space_type(XR_REFERENCE_SPACE_TYPE_VIEW);
			} break;
			case PLAY_SPACE_LOCAL: {
				openxr_api->set_play_space_type(XR_REFERENCE_SPACE_TYPE_LOCAL);
			} break;
			case PLAY_SPACE_STAGE: {
				openxr_api->set_play_space_type(XR_REFERENCE_SPACE_TYPE_STAGE);
			} break;
				//case PLAY_SPACE_UNBOUNDED: {
				//	openxr_api->set_play_space_type(XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT);
				//} break;
				//case PLAY_SPACE_COMBINED_EYE: {
				//	openxr_api->set_play_space_type(XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO);
				//} break;
			default: {
				openxr_api->set_play_space_type(XR_REFERENCE_SPACE_TYPE_STAGE);
			} break;
		}
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
	if (openxr_api == nullptr) {
		godot::Array arr;
		return arr;
	} else {
		return openxr_api->get_enabled_extensions();
	}
}

int OpenXRConfig::get_tracking_confidence(const int p_godot_controller) const {
	int confidence = 0;
	if (openxr_api && openxr_api->is_input_map_controller(p_godot_controller)) {
		confidence = int(openxr_api->get_controller_tracking_confidence(p_godot_controller));
	} else if (hand_tracking_wrapper && hand_tracking_wrapper->is_hand_tracker_controller(p_godot_controller)) {
		confidence = int(hand_tracking_wrapper->get_hand_tracker_tracking_confidence(p_godot_controller));
	}
	return confidence;
}

String OpenXRConfig::get_action_sets() const {
	if (openxr_api == nullptr) {
		return String();
	} else {
		return openxr_api->get_action_sets_json();
	}
}

void OpenXRConfig::set_action_sets(const String p_action_sets) {
	if (openxr_api == nullptr) {
		Godot::print("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_action_sets_json(p_action_sets);
	}
}

String OpenXRConfig::get_interaction_profiles() const {
	if (openxr_api == nullptr) {
		return String();
	} else {
		return openxr_api->get_interaction_profiles_json();
	}
}

void OpenXRConfig::set_interaction_profiles(const String p_interaction_profiles) {
	if (openxr_api == nullptr) {
		Godot::print("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_interaction_profiles_json(p_interaction_profiles);
	}
}

String OpenXRConfig::get_system_name() const {
	if (openxr_api == nullptr) {
		return String();
	} else {
		return openxr_api->get_system_name();
	}
}

int OpenXRConfig::get_cpu_level() const {
	if (performance_settings_wrapper == nullptr) {
		return XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
	} else {
		return performance_settings_wrapper->get_cpu_level();
	}
}

void OpenXRConfig::set_cpu_level(int level) {
	if (performance_settings_wrapper != nullptr) {
		performance_settings_wrapper->set_cpu_level(static_cast<XrPerfSettingsLevelEXT>(level));
	}
}

int OpenXRConfig::get_gpu_level() const {
	if (performance_settings_wrapper == nullptr) {
		return XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
	} else {
		return performance_settings_wrapper->get_gpu_level();
	}
}

void OpenXRConfig::set_gpu_level(int level) {
	if (performance_settings_wrapper != nullptr) {
		performance_settings_wrapper->set_gpu_level(static_cast<XrPerfSettingsLevelEXT>(level));
	}
}

double OpenXRConfig::get_render_target_size_multiplier() const {
	if (openxr_api == nullptr) {
		return 1.0;
	} else {
		return openxr_api->get_render_target_size_multiplier();
	}
}

void OpenXRConfig::set_render_target_size_multiplier(double multiplier) {
	if (openxr_api != nullptr) {
		openxr_api->set_render_target_size_multiplier(multiplier);
	}
}

void OpenXRConfig::set_foveation_level(int level, bool is_dynamic) {
	if (foveation_wrapper != nullptr) {
		XrFoveationDynamicFB foveation_dynamic = is_dynamic ? XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB : XR_FOVEATION_DYNAMIC_DISABLED_FB;
		foveation_wrapper->set_foveation_level(static_cast<XrFoveationLevelFB>(level), foveation_dynamic);
	}
}

bool OpenXRConfig::is_passthrough_supported() {
	return passthrough_wrapper != nullptr && passthrough_wrapper->is_passthrough_supported();
}

bool OpenXRConfig::is_passthrough_enabled() {
	return passthrough_wrapper != nullptr && passthrough_wrapper->is_passthrough_enabled();
}

bool OpenXRConfig::start_passthrough() {
	return passthrough_wrapper != nullptr && passthrough_wrapper->start_passthrough();
}

void OpenXRConfig::stop_passthrough() {
	if (passthrough_wrapper) {
		passthrough_wrapper->stop_passthrough();
	}
}

godot::Array OpenXRConfig::get_play_space() {
	ARVRServer *server = ARVRServer::get_singleton();
	Array arr;
	Vector3 sides[4] = {
		Vector3(-0.5f, 0.0f, -0.5f),
		Vector3(0.5f, 0.0f, -0.5f),
		Vector3(0.5f, 0.0f, 0.5f),
		Vector3(-0.5f, 0.0f, 0.5f),
	};

	if (openxr_api && server) {
		Size2 extends = openxr_api->get_play_space_bounds();
		if (extends.width != 0.0 && extends.height != 0.0) {
			Transform reference_frame = server->get_reference_frame();

			for (int i = 0; i < 4; i++) {
				Vector3 coord = sides[i];

				// scale it up
				coord.x *= extends.width;
				coord.z *= extends.height;

				// now apply our reference
				Vector3 out = reference_frame.xform(coord);
				arr.push_back(out);
			}
		} else {
			Godot::print("No extends available.");
		}
	}

	return arr;
}
