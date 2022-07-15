/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR config GDNative object

#ifndef OPENXR_CONFIG_H
#define OPENXR_CONFIG_H

#include "openxr/OpenXRApi.h"
#include "openxr/extensions/xr_ext_hand_tracking_extension_wrapper.h"
#include "openxr/extensions/xr_ext_performance_settings_extension_wrapper.h"
#include "openxr/extensions/xr_fb_color_space_extension_wrapper.h"
#include "openxr/extensions/xr_fb_display_refresh_rate_extension_wrapper.h"
#include "openxr/extensions/xr_fb_foveation_extension_wrapper.h"
#include "openxr/extensions/xr_fb_passthrough_extension_wrapper.h"
#include <Node.hpp>

namespace godot {
class OpenXRConfig : public Node {
	GODOT_CLASS(OpenXRConfig, Node)

private:
	OpenXRApi *openxr_api;
	XRFbColorSpaceExtensionWrapper *color_space_wrapper = nullptr;
	XRFbDisplayRefreshRateExtensionWrapper *display_refresh_rate_wrapper = nullptr;
	XRFbFoveationExtensionWrapper *foveation_wrapper = nullptr;
	XRExtPerformanceSettingsExtensionWrapper *performance_settings_wrapper = nullptr;
	XRFbPassthroughExtensionWrapper *passthrough_wrapper = nullptr;
	XRExtHandTrackingExtensionWrapper *hand_tracking_wrapper = nullptr;

public:
	// For Godot we can't have gaps in our enums so we define our own where needed.
	// Sadly GDNative doesn't have a way to expose these enums.

	enum ViewConfigurationType {
		VIEW_CONFIGURATION_MONO,
		VIEW_CONFIGURATION_STEREO,
		// We don't support these (yet)
		// VIEW_CONFIGURATION_QUAD,
		// VIEW_CONFIGURATION_FPO,
	};

	enum PlaySpaceType {
		PLAY_SPACE_VIEW,
		PLAY_SPACE_LOCAL,
		PLAY_SPACE_STAGE,
		// We don't support these (yet)
		// PLAY_SPACE_UNBOUNDED,
		// PLAY_SPACE_COMBINED_EYE,
	};

	static void _register_methods();

	void _init();

	OpenXRConfig();
	~OpenXRConfig();

	bool keep_3d_linear() const;

	int get_view_config_type() const;
	void set_view_config_type(const int p_config_type);

	int get_form_factor() const;
	void set_form_factor(const int p_form_factor);

	int get_color_space() const;
	void set_color_space(const int p_color_space);
	godot::Dictionary get_available_color_spaces();

	int get_play_space_type() const;
	void set_play_space_type(const int p_play_space_type);

	double get_refresh_rate() const;
	void set_refresh_rate(const double p_refresh_rate);
	godot::Array get_available_refresh_rates() const;

	godot::Array get_enabled_extensions() const;

	int get_tracking_confidence(const int p_godot_controller) const;

	String get_action_sets() const;
	void set_action_sets(const String p_action_sets);

	String get_interaction_profiles() const;
	void set_interaction_profiles(const String p_interaction_profiles);

	String get_system_name() const;

	int get_cpu_level() const;
	void set_cpu_level(int level);

	int get_gpu_level() const;
	void set_gpu_level(int level);

	double get_render_target_size_multiplier() const;
	void set_render_target_size_multiplier(double multiplier);

	void set_foveation_level(int level, bool is_dynamic);

	bool is_passthrough_supported();
	bool is_passthrough_enabled();
	bool start_passthrough();
	void stop_passthrough();

	godot::Array get_play_space();
};
} // namespace godot

#endif // !OPENXR_CONFIG_H
