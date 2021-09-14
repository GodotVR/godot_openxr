/////////////////////////////////////////////////////////////////////////////////////
// Our XRInterfaceOpenXR GD extension class

#include "gdclasses/XRInterfaceOpenXR.h"

#include "openxr/extensions/xr_ext_hand_tracking_extension_wrapper.h"
#include "openxr/extensions/xr_ext_performance_settings_extension_wrapper.h"
#include "openxr/extensions/xr_fb_color_space_extension_wrapper.h"
#include "openxr/extensions/xr_fb_display_refresh_rate_extension_wrapper.h"
#include "openxr/extensions/xr_fb_foveation_extension_wrapper.h"
#include "openxr/extensions/xr_fb_swapchain_update_state_extension_wrapper.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/display_server.hpp>

using namespace godot;

// Our constructor, destructor and bind methods

void XRInterfaceOpenXR::_bind_methods() {
	ClassDB::bind_method(D_METHOD("keep_3d_linear"), &XRInterfaceOpenXR::keep_3d_linear);
	ClassDB::bind_method(D_METHOD("get_enabled_extensions"), &XRInterfaceOpenXR::get_enabled_extensions);

	ClassDB::bind_method(D_METHOD("get_view_config_type"), &XRInterfaceOpenXR::get_view_config_type);
	ClassDB::bind_method(D_METHOD("set_view_config_type", "view_configuration"), &XRInterfaceOpenXR::set_view_config_type);

	ClassDB::bind_method(D_METHOD("get_form_factor"), &XRInterfaceOpenXR::get_form_factor);
	ClassDB::bind_method(D_METHOD("set_form_factor", "form_factor"), &XRInterfaceOpenXR::set_form_factor);

	ClassDB::bind_method(D_METHOD("get_color_space"), &XRInterfaceOpenXR::get_color_space);
	ClassDB::bind_method(D_METHOD("set_color_space", "color_space"), &XRInterfaceOpenXR::set_color_space);
	ClassDB::bind_method(D_METHOD("get_available_color_spaces"), &XRInterfaceOpenXR::get_available_color_spaces);

	ClassDB::bind_method(D_METHOD("get_refresh_rate"), &XRInterfaceOpenXR::get_refresh_rate);
	ClassDB::bind_method(D_METHOD("set_refresh_rate", "refresh_rate"), &XRInterfaceOpenXR::set_refresh_rate);
	ClassDB::bind_method(D_METHOD("get_available_refresh_rates"), &XRInterfaceOpenXR::get_available_refresh_rates);

	ClassDB::bind_method(D_METHOD("get_action_sets"), &XRInterfaceOpenXR::get_action_sets);
	ClassDB::bind_method(D_METHOD("set_action_sets", "action_sets"), &XRInterfaceOpenXR::set_action_sets);

	ClassDB::bind_method(D_METHOD("get_interaction_profiles"), &XRInterfaceOpenXR::get_interaction_profiles);
	ClassDB::bind_method(D_METHOD("set_interaction_profiles", "interaction_profiles"), &XRInterfaceOpenXR::set_interaction_profiles);

	ClassDB::bind_method(D_METHOD("get_system_name"), &XRInterfaceOpenXR::get_system_name);

	ClassDB::bind_method(D_METHOD("get_cpu_level"), &XRInterfaceOpenXR::get_cpu_level);
	ClassDB::bind_method(D_METHOD("set_cpu_level"), &XRInterfaceOpenXR::set_cpu_level);

	ClassDB::bind_method(D_METHOD("get_gpu_level"), &XRInterfaceOpenXR::get_gpu_level);
	ClassDB::bind_method(D_METHOD("set_gpu_level"), &XRInterfaceOpenXR::set_gpu_level);

	ClassDB::bind_method(D_METHOD("get_render_target_size_multiplier"), &XRInterfaceOpenXR::get_render_target_size_multiplier);
	ClassDB::bind_method(D_METHOD("set_render_target_size_multiplier"), &XRInterfaceOpenXR::set_render_target_size_multiplier);

	ClassDB::bind_method(D_METHOD("set_foveation_level"), &XRInterfaceOpenXR::set_foveation_level);

	ClassDB::bind_method(D_METHOD("start_passthrough"), &XRInterfaceOpenXR::start_passthrough);
	ClassDB::bind_method(D_METHOD("stop_passthrough"), &XRInterfaceOpenXR::stop_passthrough);

	ADD_GROUP("Configuration", "config_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "config_view_configuration", PROPERTY_HINT_ENUM, "Mono,Stereo"), "set_view_config_type", "get_view_config_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "config_form_factor", PROPERTY_HINT_ENUM, "Not set,HMD,Hand Held"), "set_form_factor", "get_form_factor");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cpu_level", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_cpu_level", "get_cpu_level");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "gpu_level", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_gpu_level", "get_gpu_level");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "render_target_size_multiplier"), "set_render_target_size_multiplier", "get_render_target_size_multiplier");

	ADD_GROUP("Display", "display_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "display_color_space"), "set_color_space", "get_color_space");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "display_refresh_rate"), "set_refresh_rate", "get_refresh_rate");

	ADD_GROUP("ActionSets", "action_sets");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "action_sets_sets"), "set_action_sets", "get_action_sets");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "action_sets_interaction_profiles"), "set_interaction_profiles", "get_interaction_profiles");
}

XRInterfaceOpenXR::XRInterfaceOpenXR() {
	if (xr_server == nullptr) {
		xr_server = XRServer::get_singleton();
		ERR_FAIL_NULL_MSG(xr_server, "Couldn't obtain XRServer singleton");
	}
	if (openxr_api == nullptr) {
		openxr_api = OpenXRApi::openxr_get_api();
		ERR_FAIL_NULL_MSG(openxr_api, "Couldn't obtain OpenXR API singleton");
	}
	color_space_wrapper = XRFbColorSpaceExtensionWrapper::get_singleton();
	display_refresh_rate_wrapper = XRFbDisplayRefreshRateExtensionWrapper::get_singleton();
	foveation_wrapper = XRFbFoveationExtensionWrapper::get_singleton();
	performance_settings_wrapper = XRExtPerformanceSettingsExtensionWrapper::get_singleton();
	passthrough_wrapper = XRFbPassthroughExtensionWrapper::get_singleton();
}

XRInterfaceOpenXR::~XRInterfaceOpenXR() {
	if (openxr_api) {
		if (openxr_api->is_initialised()) {
			_uninitialize();
		}

		// and release
		OpenXRApi::openxr_release_api();
		openxr_api = nullptr;
	}

	xr_server = nullptr;
	color_space_wrapper = nullptr;
	display_refresh_rate_wrapper = nullptr;
	foveation_wrapper = nullptr;
	performance_settings_wrapper = nullptr;
}

// Property setters and getters

bool XRInterfaceOpenXR::keep_3d_linear() const {
	if (openxr_api == NULL) {
		return false;
	} else {
		return openxr_api->get_keep_3d_linear();
	}
}

int XRInterfaceOpenXR::get_view_config_type() const {
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
				UtilityFunctions::printerr(String("Unsupported configuration type set: ") + String::num(config_type));
				return 1;
			}; break;
		}
	}
}

void XRInterfaceOpenXR::set_view_config_type(const int p_config_type) {
	// TODO may need to add something here that this is read only after initialisation
	if (openxr_api == NULL) {
		UtilityFunctions::print("OpenXR object wasn't constructed.");
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
				UtilityFunctions::printerr(String("Unsupported configuration type set: ") + String::num(p_config_type));
			}; break;
		}
	}
}

int XRInterfaceOpenXR::get_form_factor() const {
	if (openxr_api == NULL) {
		return 0;
	} else {
		return (int)openxr_api->get_form_factor();
	}
}

void XRInterfaceOpenXR::set_form_factor(const int p_form_factor) {
	if (openxr_api == NULL) {
		UtilityFunctions::print("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_form_factor((XrFormFactor)p_form_factor);
	}
}

int XRInterfaceOpenXR::get_color_space() const {
	if (color_space_wrapper == nullptr) {
		return 0;
	} else {
		return (int)color_space_wrapper->get_color_space();
	}
}

void XRInterfaceOpenXR::set_color_space(const int p_color_space) {
	if (color_space_wrapper != nullptr) {
		color_space_wrapper->set_color_space((uint32_t)p_color_space);
	}
}

godot::Dictionary XRInterfaceOpenXR::get_available_color_spaces() {
	if (color_space_wrapper != nullptr) {
		return color_space_wrapper->get_available_color_spaces();
	} else {
		return godot::Dictionary();
	}
}

double XRInterfaceOpenXR::get_refresh_rate() const {
	if (display_refresh_rate_wrapper == nullptr) {
		return 0;
	} else {
		return display_refresh_rate_wrapper->get_refresh_rate();
	}
}

void XRInterfaceOpenXR::set_refresh_rate(const double p_refresh_rate) {
	if (display_refresh_rate_wrapper != nullptr) {
		display_refresh_rate_wrapper->set_refresh_rate(p_refresh_rate);
	}
}

godot::Array XRInterfaceOpenXR::get_available_refresh_rates() const {
	if (display_refresh_rate_wrapper == nullptr) {
		godot::Array arr;
		return arr;
	} else {
		return display_refresh_rate_wrapper->get_available_refresh_rates();
	}
}

godot::Array XRInterfaceOpenXR::get_enabled_extensions() const {
	if (openxr_api == NULL) {
		godot::Array arr;
		return arr;
	} else {
		return openxr_api->get_enabled_extensions();
	}
}

String XRInterfaceOpenXR::get_action_sets() const {
	if (openxr_api == NULL) {
		return String();
	} else {
		return openxr_api->get_action_sets_json();
	}
}

void XRInterfaceOpenXR::set_action_sets(const String p_action_sets) {
	if (openxr_api == NULL) {
		UtilityFunctions::print("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_action_sets_json(p_action_sets);
	}
}

String XRInterfaceOpenXR::get_interaction_profiles() const {
	if (openxr_api == NULL) {
		return String();
	} else {
		return openxr_api->get_interaction_profiles_json();
	}
}

void XRInterfaceOpenXR::set_interaction_profiles(const String p_interaction_profiles) {
	if (openxr_api == NULL) {
		UtilityFunctions::print("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_interaction_profiles_json(p_interaction_profiles);
	}
}


String XRInterfaceOpenXR::get_system_name() const {
	if (openxr_api == nullptr) {
		return String();
	} else {
		return openxr_api->get_system_name();
	}
}

int XRInterfaceOpenXR::get_cpu_level() const {
	if (performance_settings_wrapper == nullptr) {
		return XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
	} else {
		return performance_settings_wrapper->get_cpu_level();
	}
}

void XRInterfaceOpenXR::set_cpu_level(int level) {
	if (performance_settings_wrapper != nullptr) {
		performance_settings_wrapper->set_cpu_level(static_cast<XrPerfSettingsLevelEXT>(level));
	}
}

int XRInterfaceOpenXR::get_gpu_level() const {
	if (performance_settings_wrapper == nullptr) {
		return XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
	} else {
		return performance_settings_wrapper->get_gpu_level();
	}
}

void XRInterfaceOpenXR::set_gpu_level(int level) {
	if (performance_settings_wrapper != nullptr) {
		performance_settings_wrapper->set_gpu_level(static_cast<XrPerfSettingsLevelEXT>(level));
	}
}

double XRInterfaceOpenXR::get_render_target_size_multiplier() const {
	if (openxr_api == nullptr) {
		return 1.0;
	} else {
		return openxr_api->get_render_target_size_multiplier();
	}
}

void XRInterfaceOpenXR::set_render_target_size_multiplier(double multiplier) {
	if (openxr_api != nullptr) {
		openxr_api->set_render_target_size_multiplier(multiplier);
	}
}

void XRInterfaceOpenXR::set_foveation_level(int level, bool is_dynamic) {
	if (foveation_wrapper != nullptr) {
		XrFoveationDynamicFB foveation_dynamic = is_dynamic ? XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB : XR_FOVEATION_DYNAMIC_DISABLED_FB;
		foveation_wrapper->set_foveation_level(static_cast<XrFoveationLevelFB>(level), foveation_dynamic);
	}
}

bool XRInterfaceOpenXR::start_passthrough() {
	return passthrough_wrapper != nullptr && passthrough_wrapper->start_passthrough();
}

void XRInterfaceOpenXR::stop_passthrough() {
	if (passthrough_wrapper) {
		passthrough_wrapper->stop_passthrough();
	}
}

// Interface functions

StringName XRInterfaceOpenXR::_get_name() const {
	// BLOCKED BY https://github.com/godotengine/godot-cpp/issues/611
	StringName name("OpenXR");
	return name;
}

int64_t XRInterfaceOpenXR::_get_capabilities() const {
	int64_t ret = 0;

	// We should detect what hardware we're currently on to decide whether we can use mono/stereo or quad
	ret += XR_MONO;
	ret += XR_STEREO;
	// ret += XR_QUAD; //  Once we have the ability to do this, we can add it as a feature
	ret += XR_AR;
	// ret += XR_VR; // We support VR, strangely missing as a flag, we'll add this in Godot 4
#ifndef ANDROID
	ret += XR_EXTERNAL; // Rendering to an external device, Godot window is separate.
#endif

	return ret;
}

bool XRInterfaceOpenXR::_is_initialized() const {
	if (openxr_api == nullptr) {
		return false;
	} else {
		return openxr_api->is_initialised();
	}
}

bool XRInterfaceOpenXR::_initialize() {
	// Obtain our XR server pointer
	if (xr_server == nullptr) {
		ERR_FAIL_NULL_V_MSG(xr_server, false, "Couldn't obtain XRServer singleton");
	}

	// Create our OpenXR API instance
	if (openxr_api == nullptr) {
		ERR_FAIL_NULL_V_MSG(openxr_api, false, "Couldn't obtain OpenXR API singleton");
	}

	if (!openxr_api->is_initialised()) {
		// Register our extensions
		openxr_api->register_extension_wrapper<XRFbSwapchainUpdateStateExtensionWrapper>();
		openxr_api->register_extension_wrapper<XRFbFoveationExtensionWrapper>();
		openxr_api->register_extension_wrapper<XRExtPerformanceSettingsExtensionWrapper>();
		openxr_api->register_extension_wrapper<XRFbColorSpaceExtensionWrapper>();
		openxr_api->register_extension_wrapper<XRFbDisplayRefreshRateExtensionWrapper>();
		openxr_api->register_extension_wrapper<XRExtHandTrackingExtensionWrapper>();
		openxr_api->register_extension_wrapper<XRFbPassthroughExtensionWrapper>();

		// now initialise
		openxr_api->initialize();
	}

	// Are we good ?
	return openxr_api->is_initialised();
}

void XRInterfaceOpenXR::_uninitialize() {
	if (openxr_api && openxr_api->is_initialised()) {
		// cleanup
		openxr_api->uninitialize();
	}
}

int64_t XRInterfaceOpenXR::_get_tracking_status() const {
	// FIXME implement this!
	return XRInterface::XR_UNKNOWN_TRACKING;
}

Vector2 XRInterfaceOpenXR::_get_render_target_size() {
	Vector2 size;

	if (openxr_api != nullptr) {
		uint32_t width, height;

		openxr_api->recommended_rendertarget_size(&width, &height);
		// printf("Render Target size %dx%d\n", width, height);

		size.x = width;
		size.y = height;
	}

	return size;
}

int64_t XRInterfaceOpenXR::_get_view_count() {
	int64_t ret = 2;

	if (openxr_api != nullptr) {
		XrViewConfigurationType config_type = openxr_api->get_view_configuration_type();
		switch (config_type) {
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO: {
				ret = 1;
			}; break;
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO: {
				ret = 2;
			}; break;
			/* These are not (yet) supported, adding them in here for future reference
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO: {
				// Once we add support for Quad rendering
				ret = 4;
			}; break;
			case XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT: {
				// Once we support multiple viewports for this with alternative camera tracking.
				ret = 1;
			}; break;
			*/
			default: {
				UtilityFunctions::printerr(String("Unsupported view configuration type set: ") + String::num(config_type));
				return 2;
			}; break;
		}
	}

	return ret;
}

void XRInterfaceOpenXR::_set_default_pos(Transform3D &p_transform, double p_world_scale, uint64_t p_eye) {
	p_transform = Transform3D();

	// if we're not tracking, don't put our head on the floor...
	p_transform.origin.y = 1.5 * p_world_scale;

	// overkill but..
	if (p_eye == 1) {
		p_transform.origin.x = 0.03 * p_world_scale;
	} else if (p_eye == 2) {
		p_transform.origin.x = -0.03 * p_world_scale;
	}
}

Transform3D XRInterfaceOpenXR::_get_camera_transform() {
	Transform3D hmd_transform;
	double world_scale = xr_server->get_world_scale();

	if (openxr_api == nullptr) {
		_set_default_pos(hmd_transform, world_scale, 0);
	} else if (!openxr_api->get_head_center(world_scale, hmd_transform)) {
		_set_default_pos(hmd_transform, world_scale, 0);
	}

	return xr_server->get_reference_frame() * hmd_transform;
}

Transform3D XRInterfaceOpenXR::_get_transform_for_view(int64_t p_view, const Transform3D &p_cam_transform) {
	Transform3D transform_for_eye;
	double world_scale = xr_server->get_world_scale();

	if (openxr_api == nullptr) {
		_set_default_pos(transform_for_eye, world_scale, p_view + 1);
	} else if (!openxr_api->get_view_transform(p_view, world_scale, transform_for_eye)) {
		_set_default_pos(transform_for_eye, world_scale, 0);
	}

	return p_cam_transform * xr_server->get_reference_frame() * transform_for_eye;
}

PackedFloat64Array XRInterfaceOpenXR::_get_projection_for_view(int64_t p_view, double p_aspect, double p_z_near, double p_z_far) {
	PackedFloat64Array arr;
	arr.resize(16); // 4x4 matrix

	if (openxr_api != nullptr) {
		double *projection = &arr[0]; // will this work?
		openxr_api->fill_projection_matrix(p_view, p_z_near, p_z_far, projection);
	}

	return arr;
}

void XRInterfaceOpenXR::_commit_views(const RID &p_render_target, const Rect2 &p_screen_rect) {
	if (openxr_api == nullptr) {
		return;
	}

#ifndef ANDROID
	// TODO check with OpenXR of the compositor outputs to a separate screen (HMD attached to computer) or to our main device (stand alone VR)
	// We don't want this copy if we're already outputting to the main device. Assuming this is the case on Android for now.

	uint32_t w,h;
	float width, height;
	openxr_api->recommended_rendertarget_size(&w, &h);
	width = (float) w;
	height = (float) h;

	if (!p_screen_rect.has_no_area()) {
		// just blit left eye out to screen
		Rect2 src_rect;
		Rect2 dst_rect = p_screen_rect;
		if (dst_rect.size.x > 0.0 && dst_rect.size.y > 0.0) {
			float src_height = width * (dst_rect.size.y / dst_rect.size.x); // height of our screen mapped to source space
			if (src_height < height) {
				src_height /= height;
				src_rect.position = Vector2(0.0, 0.5 * (1.0 - src_height));
				src_rect.size = Vector2(1.0, src_height);
			} else {
				float src_width = height * (dst_rect.get_size().x / dst_rect.get_size().y); // width of our screen mapped to source space
				src_width /= width;
				src_rect.position = Vector2(0.5 * (1.0 - src_width), 0.0);
				src_rect.size = Vector2(src_width, 1.0);
			}

			add_blit(p_render_target, src_rect, dst_rect, true, 0, false, Vector2(), 0.0, 0.0, 0.0, 0.0);
		}
	}
#endif

	openxr_api->render_openxr(p_render_target);
}

void XRInterfaceOpenXR::_process() {
	// this method gets called before every frame is rendered, here is where
	// you should update tracking data, update controllers, etc.
	if (openxr_api != nullptr) {
		openxr_api->process_openxr();
	}
}

void XRInterfaceOpenXR::_notification(int64_t what) {
	if (openxr_api == nullptr) {
		return;
	}

	/* FIXME reimplement this, these notifications were only added in Godot 3
	switch (what) {
		case MainLoop::NOTIFICATION_APP_RESUMED:
			openxr_api->on_resume();
			break;
		case MainLoop::NOTIFICATION_APP_PAUSED:
			openxr_api->on_pause();
			break;

		default:
			break;
	}
	*/
}

bool XRInterfaceOpenXR::_get_anchor_detection_is_enabled() const {
	return false;
}

void XRInterfaceOpenXR::_set_anchor_detection_is_enabled(bool enabled) {
}

int64_t XRInterfaceOpenXR::_get_camera_feed_id() const {
	return 0;
}

/* BLOCKED by https://github.com/godotengine/godot/pull/51179
RID XRInterfaceOpenXR::_get_external_color_texture() {
	if (openxr_api != nullptr) {
		return openxr_api->get_external_color_texture();
	}

	return RID();
}

RID XRInterfaceOpenXR::_get_external_depth_texture() {
	// TODO we should support this
	return RID();
}
*/
