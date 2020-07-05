////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our OpenXR GDNative module

#include "ARVRInterface.h"
#include "xrmath.h"

typedef struct arvr_data_struct {
	OpenXRApi *openxr_api;

	bool has_external_texture_support;
} arvr_data_struct;

godot_string godot_arvr_get_name(const void *p_data) {
	godot_string ret;

	char name[] = "OpenXR";
	api->godot_string_new(&ret);
	api->godot_string_parse_utf8(&ret, name);

	return ret;
}

godot_int godot_arvr_get_capabilities(const void *p_data) {
	godot_int ret;
	ret = 2 + 8; // 2 = ARVR_STEREO, 8 = ARVR_EXTERNAL
	return ret;
};

godot_bool godot_arvr_get_anchor_detection_is_enabled(const void *p_data) {
	godot_bool ret;

	ret = false; // does not apply here

	return ret;
};

void godot_arvr_set_anchor_detection_is_enabled(void *p_data, bool p_enable){
	// we ignore this, not supported in this interface!
};

godot_bool godot_arvr_is_stereo(const void *p_data) {
	godot_bool ret;

	ret = true;

	return ret;
};

godot_bool godot_arvr_is_initialized(const void *p_data) {
	godot_bool ret;
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	ret = arvr_data == NULL ? false : arvr_data->openxr_api != NULL;

	return ret;
};

godot_bool godot_arvr_initialize(void *p_data) {
	godot_bool ret;
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data->openxr_api == NULL) {
		arvr_data->openxr_api = OpenXRApi::openxr_get_api();
		if (arvr_data->openxr_api != NULL) {
			// TODO reset state if necessary
		};
	};

	// and return our result
	ret = arvr_data->openxr_api != NULL;
	return ret;
};

void godot_arvr_uninitialize(void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data->openxr_api != NULL) {
		// note, this will already be removed as the primary interface
		// by ARVRInterfaceGDNative

		// detach all our divices
		/*
		for (uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
		  godot_detach_device(arvr_data, i);
		};
		*/

		OpenXRApi::openxr_release_api();
		arvr_data->openxr_api = NULL;
	};
};

godot_vector2 godot_arvr_get_render_targetsize(const void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	godot_vector2 size;

	if (arvr_data->openxr_api != NULL) {
		uint32_t width, height;

		arvr_data->openxr_api->recommended_rendertarget_size(&width, &height);
		// printf("Render Target size %dx%d\n", width, height);

		api->godot_vector2_new(&size, width, height);
	} else {
		api->godot_vector2_new(&size, 500, 500);
	};

	return size;
};

godot_transform godot_arvr_get_transform_for_eye(void *p_data, godot_int p_eye, godot_transform *p_cam_transform) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	godot_transform transform_for_eye;
	godot_transform reference_frame = arvr_api->godot_arvr_get_reference_frame();
	godot_transform ret;
	godot_vector3 offset;
	godot_real world_scale = arvr_api->godot_arvr_get_worldscale();

	if (p_eye == 0) {
		// we want a monoscopic transform.. shouldn't really apply here
		api->godot_transform_new_identity(&transform_for_eye);
	} else if (arvr_data->openxr_api != NULL) {
		// printf("Get view matrix for eye %d\n", p_eye);
		if (p_eye == 1) {
			arvr_data->openxr_api->get_view_matrix(0, world_scale, &transform_for_eye);
		} else if (p_eye == 2) {
			arvr_data->openxr_api->get_view_matrix(1, world_scale, &transform_for_eye);
		} else {
			// TODO this must be implemented as this is used for head positioning, it should return the position center between the eyes
			printf("matrix for eye %d: no\n", p_eye);
		}
	}

	// Now construct our full transform, the order may be in reverse, have
	// to test
	// :)
	ret = *p_cam_transform;
	ret = api->godot_transform_operator_multiply(&ret, &reference_frame);
	ret = api->godot_transform_operator_multiply(&ret, &transform_for_eye);
	return ret;
};

void godot_arvr_fill_projection_for_eye(void *p_data, godot_real *p_projection, godot_int p_eye, godot_real p_aspect, godot_real p_z_near, godot_real p_z_far) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data->openxr_api != NULL) {
		// printf("fill projection for eye %d\n", p_eye);
		if (p_eye == 1) {
			arvr_data->openxr_api->fill_projection_matrix(0, p_z_near, p_z_far, p_projection);
		} else {
			arvr_data->openxr_api->fill_projection_matrix(1, p_z_near, p_z_far, p_projection);
		}
		// ???

		// printf("\n");
	} else {
		// uhm, should do something here really..
	};
};

void godot_arvr_commit_for_eye(void *p_data, godot_int p_eye, godot_rid *p_render_target, godot_rect2 *p_screen_rect) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
	// printf("Commit eye %d\n", p_eye);

	// This function is responsible for outputting the final render buffer
	// for each eye. p_screen_rect will only have a value when we're
	// outputting to the main viewport.

	// For an interface that must output to the main viewport (such as with
	// mobile VR) we should give an error when p_screen_rect is not set For
	// an interface that outputs to an external device we should render a
	// copy of one of the eyes to the main viewport if p_screen_rect is set,
	// and only output to the external device if not.

	godot_rect2 screen_rect = *p_screen_rect;

	if (p_eye == 1 && !api->godot_rect2_has_no_area(&screen_rect)) {
		// blit as mono, attempt to keep our aspect ratio and center our
		// render buffer
		godot_vector2 render_size = godot_arvr_get_render_targetsize(p_data);
		// printf("Rendersize = %fx%f\n", render_size.x, render_size.y);

		float new_height = screen_rect.size.x * (render_size.y / render_size.x);
		if (new_height > screen_rect.size.y) {
			screen_rect.position.y = (0.5 * screen_rect.size.y) - (0.5 * new_height);
			screen_rect.size.y = new_height;
		} else {
			float new_width = screen_rect.size.y * (render_size.x / render_size.y);

			screen_rect.position.x = (0.5 * screen_rect.size.x) - (0.5 * new_width);
			screen_rect.size.x = new_width;
		};

		// printf("Blit: %0.2f, %0.2f - %0.2f,
		// %0.2f\n",screen_rect.position.x, screen_rect.position.y,
		// screen_rect.size.x, screen_rect.size.y);

		arvr_api->godot_arvr_blit(0, p_render_target, &screen_rect);
	};

	if (arvr_data->openxr_api != NULL) {
		uint32_t texid = arvr_api->godot_arvr_get_texid(p_render_target);
		arvr_data->openxr_api->render_openxr(p_eye - 1, texid, arvr_data->has_external_texture_support);
	};
};

void godot_arvr_process(void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	// this method gets called before every frame is rendered, here is where
	// you should update tracking data, update controllers, etc.
	if (arvr_data->openxr_api != NULL) {
		arvr_data->openxr_api->process_openxr();
	}
};

void *godot_arvr_constructor(godot_object *p_instance) {
	godot_string ret;

	arvr_data_struct *arvr_data = (arvr_data_struct *)api->godot_alloc(sizeof(arvr_data_struct));
	arvr_data->openxr_api = NULL;

	return arvr_data;
}

void godot_arvr_destructor(void *p_data) {
	if (p_data != NULL) {
		arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;
		if (arvr_data->openxr_api != NULL) {
			// this should have already been called... But just in
			// case...
			godot_arvr_uninitialize(p_data);
		}

		api->godot_free(p_data);
	};
}

int godot_arvr_get_external_texture_for_eye(void *p_data, int p_eye) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	// this only gets called from Godot 3.2 and newer, allows us to use
	// OpenXR swapchain directly.

	if (arvr_data->openxr_api != NULL) {
		return arvr_data->openxr_api->get_external_texture_for_eye(p_eye - 1, &arvr_data->has_external_texture_support);
	} else {
		return 0;
	}
}

void godot_arvr_notification(void *p_data, int p_what) {
	// nothing to do here for now but we should implement this.
}

const godot_arvr_interface_gdnative interface_struct = {
	GODOTVR_API_MAJOR, GODOTVR_API_MINOR, godot_arvr_constructor,
	godot_arvr_destructor, godot_arvr_get_name, godot_arvr_get_capabilities,
	godot_arvr_get_anchor_detection_is_enabled,
	godot_arvr_set_anchor_detection_is_enabled, godot_arvr_is_stereo,
	godot_arvr_is_initialized, godot_arvr_initialize, godot_arvr_uninitialize,
	godot_arvr_get_render_targetsize, godot_arvr_get_transform_for_eye,
	godot_arvr_fill_projection_for_eye, godot_arvr_commit_for_eye,
	godot_arvr_process,
	// only available in Godot 3.2+
	godot_arvr_get_external_texture_for_eye, godot_arvr_notification
};
