////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our OpenXR GDNative module

#include <ARVRServer.hpp> // declare at the top of we'll have compile issues

#include "XRInterfaceOpenXR.h"

#include "godot_openxr.h"
#include "openxr/extensions/xr_ext_hand_tracking_extension_wrapper.h"
#include "openxr/extensions/xr_fb_color_space_extension_wrapper.h"
#include "openxr/extensions/xr_fb_display_refresh_rate_extension_wrapper.h"
#include <ARVRInterface.hpp>
#include <MainLoop.hpp>

using namespace godot;

void XRInterfaceOpenXR::_register_methods() {
	register_method("_init", &XRInterfaceOpenXR::_init);

	// register_method("_resume", &XRInterfaceOpenXR::_resume);
	// register_method("_pause", &XRInterfaceOpenXR::_pause);
}

void XRInterfaceOpenXR::_init() {
	arvr_1_2_api->godot_arvr_set_interface(this->_owner, &arvr_interface_struct);
}

String XRInterfaceOpenXR::get_name() const {
	return "OpenXR";
}

int XRInterfaceOpenXR::get_capabilities() const {
	int ret;

	// These are capabilities supported by our interface class, not necesarily by the device we're currently using.
	// We'll need to figure out a way to query OpenXR about this.

	ret = ARVRInterface::ARVR_MONO; // We support mono devices (phones, etc.)
	ret += ARVRInterface::ARVR_STEREO; // We support stereo devices (HMDs)
	// ret += ARVRInterface::ARVR_QUAD; // Once we have the ability to do this, we can add it as a feature, Godot 4 most likely
	ret += ARVRInterface::ARVR_AR; // We support AR
	// ret += ARVRInterface::ARVR_VR; // We support VR, strangely missing as a flag, we'll add this in Godot 4
#ifndef ANDROID
	ret += ARVRInterface::ARVR_EXTERNAL; // Rendering to an external device, Godot window is separate.
#endif

	return ret;
}

int64_t XRInterfaceOpenXR::get_camera_feed_id() {
	return 0;
}

bool XRInterfaceOpenXR::get_anchor_detection_is_enabled() const {
	// does not apply here
	return false;
}

void XRInterfaceOpenXR::set_anchor_detection_is_enabled(bool p_enable) {
	// we ignore this, not supported in this interface (yet)!
}

bool XRInterfaceOpenXR::is_stereo() {
	bool ret = true;

	// TODO we should check our configuration and see if we are setup for stereo (hmd) or mono output (tablet)
	if (openxr_api == nullptr) {
		ret = true;
	} else {
		XrViewConfigurationType config_type = openxr_api->get_view_configuration_type();
		switch (config_type) {
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO: {
				// In Godot 4 we'll return 1
				ret = false;
			}; break;
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO: {
				// In Godot 4 we'll return 2
				ret = true;
			}; break;
			/* These are not (yet) supported, adding them in here for future reference
			case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO: {
				// In Godot 4 we'll return 4 once we add support for Quad rendering
				return ???;
			}; break;
			case XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT: {
				// In Godot 4 we'll return 1, we'll need to support multiple viewports for this with alternative camera tracking.
				return ???;
			}; break;
			*/
			default: {
				Godot::print_error(String("Unsupported view configuration type set: ") + String::num_int64(config_type), __FUNCTION__, __FILE__, __LINE__);
				ret = true; // we need to return something even though it really is undefined...
			}; break;
		}
	}

	return ret;
}

bool XRInterfaceOpenXR::is_initialized() const {
	if (openxr_api == NULL) {
		return false;
	} else {
		return openxr_api->is_initialised();
	}
}

bool XRInterfaceOpenXR::initialize() {
	// Doesn't yet exist? create our OpenXR API instance
	if (openxr_api == NULL) {
		openxr_api = OpenXRApi::openxr_get_api();
	};

	// We (already) have our API instance? cool!
	if (openxr_api != NULL) {
		// Register our extensions
		openxr_api->register_extension_wrapper<XRFbColorSpaceExtensionWrapper>();
		openxr_api->register_extension_wrapper<XRFbDisplayRefreshRateExtensionWrapper>();
		openxr_api->register_extension_wrapper<XRExtHandTrackingExtensionWrapper>();

		// not initialise
		openxr_api->initialize();

		// Are we good ?
		return openxr_api->is_initialised();
	}

	// and return our result
	return false;
}

void XRInterfaceOpenXR::uninitialize() {
	if (openxr_api != NULL) {
		// cleanup
		openxr_api->uninitialize();

		// and release
		OpenXRApi::openxr_release_api();
		openxr_api = NULL;
	}
}

Size2 XRInterfaceOpenXR::get_render_targetsize() {
	Size2 size;

	if (openxr_api != NULL) {
		uint32_t width, height;

		openxr_api->recommended_rendertarget_size(&width, &height);
		// printf("Render Target size %dx%d\n", width, height);

		size.x = (real_t)width;
		size.y = (real_t)height;
	} else {
		size.x = 500.0;
		size.y = 500.0;
	}

	return size;
}

void XRInterfaceOpenXR::set_default_pos(Transform *p_transform, real_t p_world_scale, ARVRInterface::Eyes p_eye) {
	*p_transform = Transform::IDENTITY;

	// if we're not tracking, don't put our head on the floor...
	p_transform->origin.y = 1.5f * p_world_scale;

	// overkill but..
	if (p_eye == ARVRInterface::EYE_LEFT) {
		p_transform->origin.x = 0.03f * p_world_scale;
	} else if (p_eye == ARVRInterface::EYE_RIGHT) {
		p_transform->origin.x = -0.03f * p_world_scale;
	}
}

Transform XRInterfaceOpenXR::get_transform_for_eye(ARVRInterface::Eyes p_eye, const Transform &p_cam_transform) {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	Transform transform_for_eye;
	Transform reference_frame = arvr_server->get_reference_frame();
	real_t world_scale = arvr_server->get_world_scale();

	if (openxr_api != NULL) {
		if (p_eye == 0) {
			// this is used for head positioning, it should return the position center between the eyes
			if (!openxr_api->get_head_center(world_scale, (godot_transform *)&transform_for_eye)) {
				set_default_pos(&transform_for_eye, world_scale, p_eye);
			}
		} else {
			// printf("Get view matrix for eye %d\n", p_eye);
			if (p_eye == ARVRInterface::EYE_LEFT) {
				if (!openxr_api->get_view_transform(0, world_scale, (godot_transform *)&transform_for_eye)) {
					set_default_pos(&transform_for_eye, world_scale, p_eye);
				}
			} else if (p_eye == ARVRInterface::EYE_RIGHT) {
				if (!openxr_api->get_view_transform(1, world_scale, (godot_transform *)&transform_for_eye)) {
					set_default_pos(&transform_for_eye, world_scale, p_eye);
				}
			} else {
				// TODO does this ever happen?
				set_default_pos(&transform_for_eye, world_scale, p_eye);
			}
		}
	} else {
		set_default_pos(&transform_for_eye, world_scale, p_eye);
	}

	// Now construct our full transform
	return p_cam_transform * reference_frame * transform_for_eye;
}

CameraMatrix XRInterfaceOpenXR::get_projection_for_eye(ARVRInterface::Eyes p_eye, real_t p_aspect, real_t p_z_near, real_t p_z_far) {
	CameraMatrix cm;

	if (openxr_api != NULL) {
		// printf("fill projection for eye %d\n", p_eye);
		if (p_eye == ARVRInterface::EYE_LEFT) {
			openxr_api->fill_projection_matrix(0, p_z_near, p_z_far, (real_t *)cm.matrix);
		} else {
			openxr_api->fill_projection_matrix(1, p_z_near, p_z_far, (real_t *)cm.matrix);
		}
		// ???

		// printf("\n");
	} else {
		// uhm, should do something here really..
	}

	return cm;
}

void XRInterfaceOpenXR::commit_for_eye(ARVRInterface::Eyes p_eye, RID p_render_target, const Rect2 &p_screen_rect) {
	// printf("Commit eye %d\n", p_eye);

	// This function is responsible for outputting the final render buffer
	// for each eye. p_screen_rect will only have a value when we're
	// outputting to the main viewport.

	// For an interface that must output to the main viewport (such as with
	// mobile VR) we should give an error when p_screen_rect is not set For
	// an interface that outputs to an external device we should render a
	// copy of one of the eyes to the main viewport if p_screen_rect is set,
	// and only output to the external device if not.

#ifndef ANDROID
	// TODO check with OpenXR of the compositor outputs to a separate screen (HMD attached to computer) or to our main device (stand alone VR)
	// We don't want this copy if we're already outputting to the main device. Assuming this is the case on Android for now.

	if (p_eye == ARVRInterface::EYE_LEFT && !p_screen_rect.has_no_area()) {
		// blit as mono, attempt to keep our aspect ratio and center our
		// render buffer
		Rect2 screen_rect = p_screen_rect;
		Size2 render_size = get_render_targetsize();
		// printf("Rendersize = %fx%f\n", render_size.x, render_size.y);

		float new_height = screen_rect.size.x * (render_size.y / render_size.x);
		if (new_height > screen_rect.size.y) {
			screen_rect.position.y = (0.5f * screen_rect.size.y) - (0.5f * new_height);
			screen_rect.size.y = new_height;
		} else {
			float new_width = screen_rect.size.y * (render_size.x / render_size.y);

			screen_rect.position.x = (0.5f * screen_rect.size.x) - (0.5f * new_width);
			screen_rect.size.x = new_width;
		}

		// printf("Blit: %0.2f, %0.2f - %0.2f,
		// %0.2f\n",screen_rect.position.x, screen_rect.position.y,
		// screen_rect.size.x, screen_rect.size.y);

		// !BAS! We don't have support for this but if keep_3d_linear is true we should tell the blit to do an sRGB conversion or our preview will be too dark.

		arvr_api->godot_arvr_blit(0, (godot_rid *)&p_render_target, (godot_rect2 *)&screen_rect);
	}
#endif

	if (openxr_api != NULL) {
		uint32_t texid = arvr_api->godot_arvr_get_texid((godot_rid *)&p_render_target);
		openxr_api->render_openxr(p_eye == ARVRInterface::EYE_LEFT ? 0 : 1, texid, has_external_texture_support);
	}
}

void XRInterfaceOpenXR::process() {
	// this method gets called before every frame is rendered, here is where
	// you should update tracking data, update controllers, etc.
	if (openxr_api != NULL) {
		openxr_api->process_openxr();
	}
}

XRInterfaceOpenXR::XRInterfaceOpenXR() {
	openxr_api = NULL;
	has_external_texture_support = false;
}

XRInterfaceOpenXR::~XRInterfaceOpenXR() {
	if (openxr_api != NULL) {
		// this should have already been called... But just in
		// case...
		uninitialize();
	}
}

int XRInterfaceOpenXR::get_external_texture_for_eye(ARVRInterface::Eyes p_eye) {
	// this only gets called from Godot 3.2 and newer, allows us to use
	// OpenXR swapchain directly.

	if (openxr_api != NULL) {
		return openxr_api->get_external_texture_for_eye(p_eye == ARVRInterface::EYE_LEFT ? 0 : 1, &has_external_texture_support);
	} else {
		return 0;
	}
}

void XRInterfaceOpenXR::notification(int p_what) {
	// nothing to do here for now but we should implement this.
	if (openxr_api == nullptr) {
		return;
	}

	switch (p_what) {
		case MainLoop::NOTIFICATION_APP_RESUMED:
			openxr_api->on_resume();
			break;
		case MainLoop::NOTIFICATION_APP_PAUSED:
			openxr_api->on_pause();
			break;

		default:
			break;
	}
}

int XRInterfaceOpenXR::get_external_depth_for_eye(ARVRInterface::Eyes p_eye) {
	// stub
	return 0;
}
