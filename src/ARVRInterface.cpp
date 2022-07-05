////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our OpenXR GDNative module

#include "ARVRInterface.h"
#include "openxr/extensions/xr_ext_hand_tracking_extension_wrapper.h"
#include "openxr/extensions/xr_ext_palm_pose_extension_wrapper.h"
#include "openxr/extensions/xr_ext_performance_settings_extension_wrapper.h"
#include "openxr/extensions/xr_fb_color_space_extension_wrapper.h"
#include "openxr/extensions/xr_fb_display_refresh_rate_extension_wrapper.h"
#include "openxr/extensions/xr_fb_foveation_extension_wrapper.h"
#include "openxr/extensions/xr_fb_passthrough_extension_wrapper.h"
#include "openxr/extensions/xr_fb_swapchain_update_state_extension_wrapper.h"
#include <ARVRInterface.hpp>
#include <MainLoop.hpp>

typedef struct arvr_data_struct {
	OpenXRApi *openxr_api;

	bool has_external_texture_support;
} arvr_data_struct;

godot_string godot_arvr_get_name(const void *p_data) {
	godot_string ret;

	char name[] = "OpenXR";
	godot::api->godot_string_new(&ret);
	godot::api->godot_string_parse_utf8(&ret, name);

	return ret;
}

godot_int godot_arvr_get_capabilities(const void *p_data) {
	godot_int ret;

	// These are capabilities supported by our interface class, not necesarily by the device we're currently using.
	// We'll need to figure out a way to query OpenXR about this.

	ret = godot::ARVRInterface::ARVR_MONO; // We support mono devices (phones, etc.)
	ret += godot::ARVRInterface::ARVR_STEREO; // We support stereo devices (HMDs)
	// ret += godot::ARVRInterface::ARVR_QUAD; // Once we have the ability to do this, we can add it as a feature, Godot 4 most likely
	ret += godot::ARVRInterface::ARVR_AR; // We support AR
	// ret += godot::ARVRInterface::ARVR_VR; // We support VR, strangely missing as a flag, we'll add this in Godot 4
#ifndef ANDROID
	ret += godot::ARVRInterface::ARVR_EXTERNAL; // Rendering to an external device, Godot window is separate.
#endif

	return ret;
}

godot_bool godot_arvr_get_anchor_detection_is_enabled(const void *p_data) {
	godot_bool ret;

	ret = false; // does not apply here

	return ret;
}

void godot_arvr_set_anchor_detection_is_enabled(void *p_data, bool p_enable) {
	// we ignore this, not supported in this interface!
}

godot_bool godot_arvr_is_stereo(const void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	godot_bool ret;

	// TODO we should check our configuration and see if we are setup for stereo (hmd) or mono output (tablet)
	if (arvr_data == nullptr || arvr_data->openxr_api == nullptr) {
		ret = true;
	} else {
		XrViewConfigurationType config_type = arvr_data->openxr_api->get_view_configuration_type();
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
				godot::Godot::print_error(godot::String("Unsupported view configuration type set: ") + godot::String::num_int64(config_type), __FUNCTION__, __FILE__, __LINE__);
				ret = true; // we need to return something even though it really is undefined...
			}; break;
		}
	}

	return ret;
}

godot_bool godot_arvr_is_initialized(const void *p_data) {
	godot_bool ret;
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data == nullptr || arvr_data->openxr_api == nullptr) {
		ret = false;
	} else {
		ret = arvr_data->openxr_api->is_initialised();
	}

	return ret;
}

godot_bool godot_arvr_initialize(void *p_data) {
	godot_bool ret = false;
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	// Doesn't yet exist? create our OpenXR API instance
	if (arvr_data->openxr_api == nullptr) {
		arvr_data->openxr_api = OpenXRApi::openxr_get_api();
	}

	// We (already) have our API instance? cool!
	if (arvr_data->openxr_api != nullptr) {
		// Register our extensions
		arvr_data->openxr_api->register_extension_wrapper<XRFbSwapchainUpdateStateExtensionWrapper>();
		arvr_data->openxr_api->register_extension_wrapper<XRFbFoveationExtensionWrapper>();
		arvr_data->openxr_api->register_extension_wrapper<XRExtPerformanceSettingsExtensionWrapper>();
		arvr_data->openxr_api->register_extension_wrapper<XRFbColorSpaceExtensionWrapper>();
		arvr_data->openxr_api->register_extension_wrapper<XRFbDisplayRefreshRateExtensionWrapper>();
		arvr_data->openxr_api->register_extension_wrapper<XRExtHandTrackingExtensionWrapper>();
		arvr_data->openxr_api->register_extension_wrapper<XRFbPassthroughExtensionWrapper>();
		arvr_data->openxr_api->register_extension_wrapper<XRExtPalmPoseExtensionWrapper>();

		// not initialise
		arvr_data->openxr_api->initialize();

		// Are we good ?
		ret = arvr_data->openxr_api->is_initialised();
	}

	// and return our result
	return ret;
}

void godot_arvr_uninitialize(void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data->openxr_api != nullptr) {
		// cleanup
		arvr_data->openxr_api->uninitialize();

		// and release
		OpenXRApi::openxr_release_api();
		arvr_data->openxr_api = nullptr;
	}
}

godot_vector2 godot_arvr_get_render_targetsize(const void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	godot_vector2 size;

	if (arvr_data->openxr_api != nullptr) {
		uint32_t width, height;

		arvr_data->openxr_api->recommended_rendertarget_size(&width, &height);

		godot::api->godot_vector2_new(&size, width, height);
	} else {
		godot::api->godot_vector2_new(&size, 500, 500);
	}

	return size;
}

void set_default_pos(godot_transform *p_transform, godot_real p_world_scale, godot_int p_eye) {
	godot::Transform *t = (godot::Transform *)p_transform;
	godot::api->godot_transform_new_identity(p_transform);

	// if we're not tracking, don't put our head on the floor...
	t->origin.y = 1.5 * p_world_scale;

	// overkill but..
	if (p_eye == 1) {
		t->origin.x = 0.03 * p_world_scale;
	} else if (p_eye == 2) {
		t->origin.x = -0.03 * p_world_scale;
	}
}

godot_transform godot_arvr_get_transform_for_eye(void *p_data, godot_int p_eye, godot_transform *p_cam_transform) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	godot_transform transform_for_eye;
	godot_transform reference_frame = godot::arvr_api->godot_arvr_get_reference_frame();
	godot_transform ret;
	godot_real world_scale = godot::arvr_api->godot_arvr_get_worldscale();

	if (arvr_data->openxr_api != nullptr) {
		if (p_eye == 0) {
			// this is used for head positioning, it should return the position center between the eyes
			if (!arvr_data->openxr_api->get_head_center(world_scale, &transform_for_eye)) {
				set_default_pos(&transform_for_eye, world_scale, p_eye);
			}
		} else {
			if (p_eye == 1) {
				if (!arvr_data->openxr_api->get_view_transform(0, world_scale, &transform_for_eye)) {
					set_default_pos(&transform_for_eye, world_scale, p_eye);
				}
			} else if (p_eye == 2) {
				if (!arvr_data->openxr_api->get_view_transform(1, world_scale, &transform_for_eye)) {
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

	// Now construct our full transform, the order may be in reverse, have
	// to test
	// :)
	ret = *p_cam_transform;
	ret = godot::api->godot_transform_operator_multiply(&ret, &reference_frame);
	ret = godot::api->godot_transform_operator_multiply(&ret, &transform_for_eye);
	return ret;
}

void godot_arvr_fill_projection_for_eye(void *p_data, godot_real *p_projection, godot_int p_eye, godot_real p_aspect, godot_real p_z_near, godot_real p_z_far) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data->openxr_api != nullptr) {
		if (p_eye == 1) {
			arvr_data->openxr_api->fill_projection_matrix(0, p_z_near, p_z_far, p_projection);
		} else {
			arvr_data->openxr_api->fill_projection_matrix(1, p_z_near, p_z_far, p_projection);
		}
	} else {
		// uhm, should do something here really..
	}
}

void godot_arvr_commit_for_eye(void *p_data, godot_int p_eye, godot_rid *p_render_target, godot_rect2 *p_screen_rect) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	// This function is responsible for outputting the final render buffer
	// for each eye. p_screen_rect will only have a value when we're
	// outputting to the main viewport.

	// For an interface that must output to the main viewport (such as with
	// mobile VR) we should give an error when p_screen_rect is not set For
	// an interface that outputs to an external device we should render a
	// copy of one of the eyes to the main viewport if p_screen_rect is set,
	// and only output to the external device if not.

	godot::Rect2 screen_rect = *(godot::Rect2 *)p_screen_rect;

#ifndef ANDROID
	// TODO check with OpenXR of the compositor outputs to a separate screen (HMD attached to computer) or to our main device (stand alone VR)
	// We don't want this copy if we're already outputting to the main device. Assuming this is the case on Android for now.

	if (p_eye == 1 && !screen_rect.has_no_area()) {
		// blit as mono, attempt to keep our aspect ratio and center our
		// render buffer
		godot_vector2 rs = godot_arvr_get_render_targetsize(p_data);
		godot::Vector2 *render_size = (godot::Vector2 *)&rs;

		float new_height = screen_rect.size.x * (render_size->y / render_size->x);
		if (new_height > screen_rect.size.y) {
			screen_rect.position.y = (0.5 * screen_rect.size.y) - (0.5 * new_height);
			screen_rect.size.y = new_height;
		} else {
			float new_width = screen_rect.size.y * (render_size->x / render_size->y);

			screen_rect.position.x = (0.5 * screen_rect.size.x) - (0.5 * new_width);
			screen_rect.size.x = new_width;
		}

		// From Godot 3.4 onwards this should now correctly apply a linear->sRGB conversion if our render buffer remains in linear color space.
		godot::arvr_api->godot_arvr_blit(0, p_render_target, (godot_rect2 *)&screen_rect);
	}
#endif

	if (arvr_data->openxr_api != nullptr) {
		uint32_t texid = godot::arvr_api->godot_arvr_get_texid(p_render_target);
		arvr_data->openxr_api->render_openxr(p_eye - 1, texid, arvr_data->has_external_texture_support);
	}
}

void godot_arvr_process(void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	// this method gets called before every frame is rendered, here is where
	// you should update tracking data, update controllers, etc.
	if (arvr_data->openxr_api != nullptr) {
		arvr_data->openxr_api->process_openxr();
	}
}

void *godot_arvr_constructor(godot_object *p_instance) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)godot::api->godot_alloc(sizeof(arvr_data_struct));
	arvr_data->openxr_api = nullptr;

	return arvr_data;
}

void godot_arvr_destructor(void *p_data) {
	if (p_data != nullptr) {
		arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
		if (arvr_data->openxr_api != nullptr) {
			// this should have already been called... But just in
			// case...
			godot_arvr_uninitialize(p_data);
		}

		godot::api->godot_free(p_data);
	}
}

int godot_arvr_get_external_texture_for_eye(void *p_data, int p_eye) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	// this only gets called from Godot 3.2 and newer, allows us to use
	// OpenXR swapchain directly.

	if (arvr_data->openxr_api != nullptr) {
		return arvr_data->openxr_api->get_external_texture_for_eye(p_eye - 1, &arvr_data->has_external_texture_support);
	} else {
		return 0;
	}
}

void godot_arvr_notification(void *p_data, int p_what) {
	// nothing to do here for now but we should implement this.
	auto *arvr_data = (arvr_data_struct *)p_data;
	if (arvr_data->openxr_api == nullptr) {
		return;
	}

	switch (p_what) {
		case godot::MainLoop::NOTIFICATION_APP_RESUMED:
			arvr_data->openxr_api->on_resume();
			break;
		case godot::MainLoop::NOTIFICATION_APP_PAUSED:
			arvr_data->openxr_api->on_pause();
			break;

		default:
			break;
	}
}

int godot_arvr_get_camera_feed_id(void *) {
	// stub
	return 0;
}

int godot_arvr_get_external_depth_for_eye(void *p_data, int p_eye) {
	// stub
	return 0;
}

const godot_arvr_interface_gdnative interface_struct = {
	{ GODOTVR_API_MAJOR, GODOTVR_API_MINOR },
	godot_arvr_constructor,
	godot_arvr_destructor,
	godot_arvr_get_name,
	godot_arvr_get_capabilities,
	godot_arvr_get_anchor_detection_is_enabled,
	godot_arvr_set_anchor_detection_is_enabled,
	godot_arvr_is_stereo,
	godot_arvr_is_initialized,
	godot_arvr_initialize,
	godot_arvr_uninitialize,
	godot_arvr_get_render_targetsize,
	godot_arvr_get_transform_for_eye,
	godot_arvr_fill_projection_for_eye,
	godot_arvr_commit_for_eye,
	godot_arvr_process,
	// only available in Godot 3.2+
	godot_arvr_get_external_texture_for_eye,
	godot_arvr_notification,
	godot_arvr_get_camera_feed_id,
	// only available in Godot 3.3+
	godot_arvr_get_external_depth_for_eye
};
