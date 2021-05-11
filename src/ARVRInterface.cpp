////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our OpenXR GDNative module

#include "ARVRInterface.h"

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

	// TODO we should check our configuration and see if we are setup for stereo (hmd) or mono output (tablet)

	ret = true;

	return ret;
};

godot_bool godot_arvr_is_initialized(const void *p_data) {
	godot_bool ret;
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data == NULL) {
		ret = false;
	} else if (arvr_data->openxr_api == NULL) {
		ret = false;
	} else {
		ret = arvr_data->openxr_api->is_initialised();
	}

	return ret;
};

godot_bool godot_arvr_initialize(void *p_data) {
	godot_bool ret = false;
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	// Doesn't yet exist? create our OpenXR API instance
	if (arvr_data->openxr_api == NULL) {
		arvr_data->openxr_api = OpenXRApi::openxr_get_api();
	};

	// We (already) have our API instance? cool!
	if (arvr_data->openxr_api != NULL) {
		// not initialise
		arvr_data->openxr_api->initialize();

		// Are we good ?
		ret = arvr_data->openxr_api->is_initialised();
	}

	// and return our result
	return ret;
};

void godot_arvr_uninitialize(void *p_data) {
	arvr_data_struct *arvr_data = (arvr_data_struct *)p_data;

	if (arvr_data->openxr_api != NULL) {
		// cleanup
		arvr_data->openxr_api->uninitialize();

		// and release
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

		godot::api->godot_vector2_new(&size, width, height);
	} else {
		godot::api->godot_vector2_new(&size, 500, 500);
	};

	return size;
};

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

	if (arvr_data->openxr_api != NULL) {
		if (p_eye == 0) {
			// this is used for head positioning, it should return the position center between the eyes
			if (!arvr_data->openxr_api->get_head_center(world_scale, &transform_for_eye)) {
				set_default_pos(&transform_for_eye, world_scale, p_eye);
			}
		} else {
			// printf("Get view matrix for eye %d\n", p_eye);
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

	godot::Rect2 screen_rect = *(godot::Rect2 *)p_screen_rect;

	if (p_eye == 1 && !screen_rect.has_no_area()) {
		// blit as mono, attempt to keep our aspect ratio and center our
		// render buffer
		godot_vector2 rs = godot_arvr_get_render_targetsize(p_data);
		godot::Vector2 *render_size = (godot::Vector2 *)&rs;
		// printf("Rendersize = %fx%f\n", render_size.x, render_size.y);

		float new_height = screen_rect.size.x * (render_size->y / render_size->x);
		if (new_height > screen_rect.size.y) {
			screen_rect.position.y = (0.5 * screen_rect.size.y) - (0.5 * new_height);
			screen_rect.size.y = new_height;
		} else {
			float new_width = screen_rect.size.y * (render_size->x / render_size->y);

			screen_rect.position.x = (0.5 * screen_rect.size.x) - (0.5 * new_width);
			screen_rect.size.x = new_width;
		};

		// printf("Blit: %0.2f, %0.2f - %0.2f,
		// %0.2f\n",screen_rect.position.x, screen_rect.position.y,
		// screen_rect.size.x, screen_rect.size.y);

		// !BAS! We don't have support for this but if keep_3d_linear is true we should tell the blit to do an sRGB conversion or our preview will be too dark.

		godot::arvr_api->godot_arvr_blit(0, p_render_target, (godot_rect2 *)&screen_rect);
	};

	if (arvr_data->openxr_api != NULL) {
		uint32_t texid = godot::arvr_api->godot_arvr_get_texid(p_render_target);
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

	arvr_data_struct *arvr_data = (arvr_data_struct *)godot::api->godot_alloc(sizeof(arvr_data_struct));
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

		godot::api->godot_free(p_data);
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

int godot_arvr_get_camera_feed_id(void *) {
	// stub
	return 0;
}

int godot_arvr_get_external_depth_for_eye(void *p_data, int p_eye) {
	// stub
	return 0;
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
	godot_arvr_get_external_texture_for_eye, godot_arvr_notification, godot_arvr_get_camera_feed_id,
	// only available in Godot 3.3+
	godot_arvr_get_external_depth_for_eye
};
