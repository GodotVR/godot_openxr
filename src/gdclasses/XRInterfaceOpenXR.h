////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our OpenXR GDNative module

#ifndef OPENXR_INTERFACE_H
#define OPENXR_INTERFACE_H

#include "ARVRInterfaceGDNative.hpp"
#include "CameraMatrix.hpp"
#include "Transform.hpp"

#include "openxr/OpenXRApi.h"

namespace godot {
class XRInterfaceOpenXR : public ARVRInterfaceGDNative {
	GODOT_CLASS(XRInterfaceOpenXR, ARVRInterfaceGDNative)

public:
	static void _register_methods();

	XRInterfaceOpenXR();
	~XRInterfaceOpenXR();

	virtual String get_name() const;
	virtual int get_capabilities() const;

	virtual bool is_initialized() const;
	virtual bool initialize();
	virtual void uninitialize();

	virtual Size2 get_render_targetsize();
	virtual bool is_stereo();
	virtual int64_t get_camera_feed_id();
	virtual bool get_anchor_detection_is_enabled() const;
	virtual void set_anchor_detection_is_enabled(const bool enable);

	// These ones are not exposed through GDNative but we're calling them through our glue code anyway
	Transform get_transform_for_eye(ARVRInterface::Eyes p_eye, const Transform &p_cam_transform);
	CameraMatrix get_projection_for_eye(ARVRInterface::Eyes p_eye, real_t p_aspect, real_t p_z_near, real_t p_z_far);
	void commit_for_eye(ARVRInterface::Eyes p_eye, RID p_render_target, const Rect2 &p_screen_rect);

	void process();
	void notification(int p_what);

	int get_external_texture_for_eye(ARVRInterface::Eyes p_eye);
	int get_external_depth_for_eye(ARVRInterface::Eyes p_eye);

	void _init();

private:
	static XRInterfaceOpenXR *singleton_instance;

	OpenXRApi *openxr_api;

	bool has_external_texture_support;

	void set_default_pos(Transform *p_transform, real_t p_world_scale, ARVRInterface::Eyes p_eye);
};
} // namespace godot
#endif /* !OPENXR_INTERFACE_H */
