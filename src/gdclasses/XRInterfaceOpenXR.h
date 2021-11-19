/////////////////////////////////////////////////////////////////////////////////////
// Our XRInterfaceOpenXR GD extension class

// BLOCKED by https://github.com/godotengine/godot/pull/52532, can't load openxr loader dependency
// BLOCKED by https://github.com/godotengine/godot/pull/52210, can't implement controller logic

#ifndef XR_INTERFACE_OPENXR_H
#define XR_INTERFACE_OPENXR_H

#include "openxr/OpenXRApi.h"
#include "openxr/extensions/xr_ext_performance_settings_extension_wrapper.h"
#include "openxr/extensions/xr_fb_color_space_extension_wrapper.h"
#include "openxr/extensions/xr_fb_display_refresh_rate_extension_wrapper.h"
#include "openxr/extensions/xr_fb_foveation_extension_wrapper.h"
#include "openxr/extensions/xr_fb_passthrough_extension_wrapper.h"

#include <godot_cpp/classes/xr_interface_extension.hpp>

#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/classes/xr_server.hpp>

namespace godot {
class XRInterfaceOpenXR : public XRInterfaceExtension {
	GDCLASS(XRInterfaceOpenXR, XRInterfaceExtension);

private:
	OpenXRApi *openxr_api = nullptr;
	XRServer *xr_server = nullptr;

	XRFbColorSpaceExtensionWrapper *color_space_wrapper = nullptr;
	XRFbDisplayRefreshRateExtensionWrapper *display_refresh_rate_wrapper = nullptr;
	XRFbFoveationExtensionWrapper *foveation_wrapper = nullptr;
	XRExtPerformanceSettingsExtensionWrapper *performance_settings_wrapper = nullptr;
	XRFbPassthroughExtensionWrapper *passthrough_wrapper = nullptr;

	void _set_default_pos(Transform3D &p_transform, double p_world_scale, uint64_t p_eye);
protected:
	static void _bind_methods();

public:
	XRInterfaceOpenXR();
	~XRInterfaceOpenXR();

	// Constants

	// Property setters and getters
	bool keep_3d_linear() const;

	int get_view_config_type() const;
	void set_view_config_type(const int p_config_type);

	int get_form_factor() const;
	void set_form_factor(const int p_form_factor);

	int get_color_space() const;
	void set_color_space(const int p_color_space);
	godot::Dictionary get_available_color_spaces();

	double get_refresh_rate() const;
	void set_refresh_rate(const double p_refresh_rate);
	godot::Array get_available_refresh_rates() const;

	godot::Array get_enabled_extensions() const;

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

	bool start_passthrough();
	void stop_passthrough();

	// Interface functions
	virtual StringName _get_name() const override;
	virtual int64_t _get_capabilities() const override;

	virtual bool _is_initialized() const override;
	virtual bool _initialize() override;
	virtual void _uninitialize() override;

	virtual int64_t _get_tracking_status() const override;

	virtual Vector2 _get_render_target_size() override;
	virtual int64_t _get_view_count() override;
	virtual Transform3D _get_camera_transform() override;
	virtual Transform3D _get_transform_for_view(int64_t p_view, const Transform3D &p_cam_transform) override;
	virtual PackedFloat64Array _get_projection_for_view(int64_t p_view, double p_aspect, double p_z_near, double p_z_far) override;

	virtual void _commit_views(const RID &p_render_target, const Rect2 &p_screen_rect) override;

	virtual void _process() override;
	virtual void _notification(int64_t what) override;

	virtual bool _get_anchor_detection_is_enabled() const override;
	virtual void _set_anchor_detection_is_enabled(bool enabled) override;
	virtual int64_t _get_camera_feed_id() const override;

	virtual RID _get_external_color_texture();
	virtual RID _get_external_depth_texture();

	/* Future feature
	virtual RID _get_external_vrs_texture(uint64_t p_view);
	*/
};
} // namespace godot

#endif // !XR_INTERFACE_OPENXR_H
