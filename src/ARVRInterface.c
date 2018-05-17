////////////////////////////////////////////////////////////////////////////////////////////////
// Our main ARVRInterface code for our OpenHMD GDNative module

#include "ARVRInterface.h"

void *godot_arvr_constructor(godot_object *p_instance) {
	return get_openhmd_data();
}

void godot_arvr_destructor(void *p_data) {
	if (p_data == NULL || openhmd_data != p_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else {
		release_openhmd_data();
	}
}

godot_string godot_arvr_get_name(const void *p_data) {
	godot_string ret;

	char name[] = "OpenHMD";
	api->godot_string_new(&ret);
	api->godot_string_parse_utf8(&ret, name);

	return ret;
}

godot_int godot_arvr_get_capabilities(const void *p_data) {
	godot_int ret;

	// need to add 4 (ARVR_EXTERNAL) once we support direct output to the right monitor
	ret = 2; // 2 = ARVR_STEREO

	return ret;
};

godot_bool godot_arvr_get_anchor_detection_is_enabled(const void *p_data) {
	godot_bool ret;

	ret = false; // does not apply here

	return ret;
};

void godot_arvr_set_anchor_detection_is_enabled(void *p_data, bool p_enable) {
	// we ignore this, not supported in this interface!
};

godot_bool godot_arvr_is_stereo(const void *p_data) {
	godot_bool ret;

	ret = true;

	return ret;
};

godot_bool godot_arvr_is_initialized(const void *p_data) {
	godot_bool ret = false;

	if (p_data == NULL || p_data != openhmd_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else {
		ret = openhmd_data->ohmd_ctx != NULL;
	}

	return ret;
};
 
godot_bool godot_arvr_initialize(void *p_data) {
	godot_bool ret = false;

	if (p_data == NULL || p_data != openhmd_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else if (openhmd_data->ohmd_ctx == NULL) {
		// initialize openhmd
		openhmd_data->ohmd_ctx = ohmd_ctx_create();

		// should we build this once and just remember it? or keep building it like this?
		openhmd_data->ohmd_settings = ohmd_device_settings_create(openhmd_data->ohmd_ctx);

		// automatic polling of the HMD should always be enabled, will make sure that even when the application can not keep up, 
		// the tracking will stay correct and not only update per rendered frame.

		int auto_update = 1;
		ohmd_device_settings_seti(openhmd_data->ohmd_settings, OHMD_IDS_AUTOMATIC_UPDATE, &auto_update);

		// create our lens distortion shader
		openhmd_shader_init();

		// populate our initial list of available devices
		openhmd_scan_for_devices();

		// initialize our first device?
		if (openhmd_data->do_auto_init_device_zero) {
			openhmd_init_hmd_device(0);
		};

		ret = openhmd_data->ohmd_ctx != NULL;
	};

	// and return our result
	return ret;
};

void godot_arvr_uninitialize(void *p_data) {
	if (p_data == NULL || p_data != openhmd_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else if (openhmd_data->ohmd_ctx != NULL) {
		for (int i = 0; i < OPENHMD_MAX_CONTROLLERS; i++) {
			// closes device if there is a device to close
			openhmd_close_controller_device(i);
		}

		openhmd_close_tracking_device();
		openhmd_close_hmd_device();

		openhmd_shader_cleanup();

		if (openhmd_data->ohmd_settings != NULL) {
			ohmd_device_settings_destroy(openhmd_data->ohmd_settings);
			openhmd_data->ohmd_settings = NULL;
		};

		ohmd_ctx_destroy(openhmd_data->ohmd_ctx);
		openhmd_data->ohmd_ctx = NULL;
	};
};

godot_vector2 godot_arvr_get_render_targetsize(const void *p_data) {
	godot_vector2 size;

	if (p_data == NULL || p_data != openhmd_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else if (openhmd_data->ohmd_ctx != NULL) {
			if (openhmd_data->hmd_device != NULL) {
				// use floats so we can multiply
				float width = openhmd_data->width;
				float height = openhmd_data->height;

				width = floor(width * openhmd_data->oversample);
				height = floor(height * openhmd_data->oversample);

				api->godot_vector2_new(&size, (int) width, (int) height);
			} else {
				/* just return something so we can show something instead of crashing */
				api->godot_vector2_new(&size, 600, 900);
			};
	};

	return size;
};

// Converts a openhmd matrix to a transform, note, don't use this for a projection matrix!
void openhmd_transform_from_matrix(godot_transform *p_dest, ohmd_device *p_device, ohmd_float_value p_type, float p_world_scale) {
	if (p_device != NULL) {
		godot_basis basis;
		godot_vector3 origin;
		float *basis_ptr = (float *) &basis; // Godot can switch between real_t being double or float.. which one is used...
		float m[4][4];

		ohmd_device_getf(p_device, p_type, (float *)m);

		int k = 0;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				basis_ptr[k++] = m[i][j];
			};
		};

		api->godot_vector3_new(&origin, m[0][3] * p_world_scale, m[1][3] * p_world_scale, m[2][3] * p_world_scale);
		api->godot_transform_new(p_dest, &basis, &origin);
	} else {
		api->godot_transform_new_identity(p_dest);		
	};
};

void openhmd_transform_from_rot_pos(godot_transform *p_dest, ohmd_device *p_device, float p_world_scale) {
	if (p_device != NULL) {
		godot_quat q;
		godot_basis basis;
		godot_vector3 origin;
		float ohmd_q[4];
		float ohmd_v[4];

		// convert orientation quad to position, should add helper function for this :)
		ohmd_device_getf(p_device, OHMD_ROTATION_QUAT, ohmd_q);
		api->godot_quat_new(&q, ohmd_q[0], ohmd_q[1], ohmd_q[2], ohmd_q[3]);
		api->godot_basis_new_with_euler_quat(&basis, &q);

		ohmd_device_getf(p_device, OHMD_POSITION_VECTOR, ohmd_v);

		api->godot_vector3_new(&origin, ohmd_v[0] * p_world_scale, ohmd_v[1] * p_world_scale, ohmd_v[2] * p_world_scale);
		api->godot_transform_new(p_dest, &basis, &origin);
	} else {
		api->godot_transform_new_identity(p_dest);		
	};
};

godot_transform godot_arvr_get_transform_for_eye(void *p_data, godot_int p_eye, godot_transform *p_cam_transform) {
	godot_transform transform_for_eye;
	godot_transform reference_frame = arvr_api->godot_arvr_get_reference_frame();
	godot_transform ret;
	godot_vector3 offset;
	godot_real world_scale = arvr_api->godot_arvr_get_worldscale();

	if (p_data == NULL || p_data != openhmd_data) {
		// this should never ever ever ever happen, just being paranoid....
		api->godot_transform_new_identity(&ret);
	} else if (openhmd_data->ohmd_ctx != NULL) {
		if (openhmd_data->tracking_device != NULL) {
			godot_transform hmd_transform;
			
			// Our tracker will only have location and position data, OHMD_LEFT_EYE_GL_MODELVIEW_MATRIX and OHMD_RIGHT_EYE_GL_MODELVIEW_MATRIX would return the same thing
			openhmd_transform_from_rot_pos(&hmd_transform, openhmd_data->tracking_device, world_scale);

			// now we manually add our IPD from our HMD
			api->godot_transform_new_identity(&transform_for_eye);
			if (openhmd_data->hmd_device != NULL) {
				float ipd;
				ohmd_device_getf(openhmd_data->hmd_device, OHMD_EYE_IPD, &ipd);

				if (p_eye == 1) {
					godot_vector3 v_ipd;
					api->godot_vector3_new(&v_ipd, (ipd / 2.0) * world_scale, 0.0, 0.0);
					api->godot_transform_set_origin(&transform_for_eye, &v_ipd);
				} else if (p_eye == 2) {
					godot_vector3 v_ipd;
					api->godot_vector3_new(&v_ipd, (-ipd / 2.0) * world_scale, 0.0, 0.0);
					api->godot_transform_set_origin(&transform_for_eye, &v_ipd);
				};
			};

			transform_for_eye = api->godot_transform_operator_multiply(&transform_for_eye, &hmd_transform);
		} else if (openhmd_data->hmd_device != NULL) {
			// Get our view matrices from OpenHMD
			if (p_eye == 1) {
				openhmd_transform_from_matrix(&transform_for_eye, openhmd_data->hmd_device, OHMD_LEFT_EYE_GL_MODELVIEW_MATRIX, world_scale);
			} else if (p_eye == 2) {
				openhmd_transform_from_matrix(&transform_for_eye, openhmd_data->hmd_device, OHMD_RIGHT_EYE_GL_MODELVIEW_MATRIX, world_scale);
			} else {
				// 'mono' will be requested purely for scene positioning feedback, no longer used by renderer
				openhmd_transform_from_rot_pos(&transform_for_eye, openhmd_data->hmd_device, world_scale);
			};
		} else {
			// calling before initialised
			api->godot_transform_new_identity(&transform_for_eye);
		};

		// Now construct our full transform, the order may be in reverse, have to test :)
		ret = *p_cam_transform;
		ret = api->godot_transform_operator_multiply(&ret, &reference_frame);
		ret = api->godot_transform_operator_multiply(&ret, &transform_for_eye);
	} else {
		// calling before initialised
		api->godot_transform_new_identity(&ret);
	};

	return ret;
};

void godot_arvr_fill_projection_for_eye(void *p_data, godot_real *p_projection, godot_int p_eye, godot_real p_aspect, godot_real p_z_near, godot_real p_z_far) {
	if (p_data == NULL || p_data != openhmd_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else if (openhmd_data->ohmd_ctx == NULL || openhmd_data->hmd_device == NULL) {
		// calling before initialised
	} else {
		float m[4][4];
		float z_near = p_z_near;
		float z_far = p_z_far;
		ohmd_device_setf(openhmd_data->hmd_device, OHMD_PROJECTION_ZNEAR, &z_near);
		ohmd_device_setf(openhmd_data->hmd_device, OHMD_PROJECTION_ZFAR, &z_far);
		ohmd_device_getf(openhmd_data->hmd_device, p_eye == 1 ? OHMD_LEFT_EYE_GL_PROJECTION_MATRIX : OHMD_RIGHT_EYE_GL_PROJECTION_MATRIX, (float *)m);

		int k = 0;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				p_projection[k++] = m[i][j];
			};
		};
	};
};

void godot_arvr_commit_for_eye(void *p_data, godot_int p_eye, godot_rid *p_render_target, godot_rect2 *p_screen_rect) {
	// This function is responsible for outputting the final render buffer for each eye. 
	// p_screen_rect will only have a value when we're outputting to the main viewport.

	// For an interface that must output to the main viewport (such as with mobile VR) we should give an error when p_screen_rect is not set
	// For an interface that outputs to an external device we should render a copy of one of the eyes to the main viewport if p_screen_rect is set, and only output to the external device if not.

	if (p_data == NULL || p_data != openhmd_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else if (openhmd_data->ohmd_ctx == NULL || openhmd_data->hmd_device == NULL) {
		// calling before initialised
		arvr_api->godot_arvr_blit(p_eye, p_render_target, p_screen_rect);
	} else {
		/* Once we render in a secondairy window that is linked to our HMD, we can output for our spectator if we're using the main viewport
		if (p_eye == 1 && !api->godot_rect2_has_no_area(p_screen_rect)) {
			// blit as mono
			arvr_api->godot_arvr_blit(0, p_render_target, p_screen_rect);
		};
		*/

//		arvr_api->godot_arvr_blit(p_eye, p_render_target, p_screen_rect);


		///@TODO we should set our output to the window we opened for our HMD. For now, just output to our main window
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		godot_vector2 size = api->godot_rect2_get_size(p_screen_rect);
		glViewport(0, 0, api->godot_vector2_get_x(&size), api->godot_vector2_get_y(&size));
		uint32_t texid = arvr_api->godot_arvr_get_texid(p_render_target);
		openhmd_shader_render_eye(texid, p_eye == 1 ? 0 : 1);
	};
};

void godot_arvr_process(void *p_data) {
	// this method gets called before every frame is rendered, here is where you should update tracking data, update controllers, etc.

	if (p_data == NULL || p_data != openhmd_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else {
		ohmd_ctx_update(openhmd_data->ohmd_ctx);
		for (int i = 0; i < OPENHMD_MAX_CONTROLLERS; i++) {
			if (openhmd_data->controller_tracker_mapping[i].device != NULL) {
				godot_transform controller_transform;

				openhmd_transform_from_rot_pos(&controller_transform, openhmd_data->controller_tracker_mapping[i].device, 1.0);
				arvr_api->godot_arvr_set_controller_transform(openhmd_data->controller_tracker_mapping[i].tracker, &controller_transform, true, true);


				// Set controller buttons
				int control_count = 0;
				ohmd_device_geti(openhmd_data->controller_tracker_mapping[i].device, OHMD_CONTROL_COUNT, &control_count);

				float control_state[256];
				ohmd_device_getf(openhmd_data->controller_tracker_mapping[i].device, OHMD_CONTROLS_STATE, control_state);

				int control_function[64];
				ohmd_device_geti(openhmd_data->controller_tracker_mapping[i].device, OHMD_CONTROLS_HINTS, control_function);

				for(int j = 0; j < control_count; j++)
				{
					int button = 0;
					if (control_function[j] == OHMD_TRIGGER_CLICK)
						button = 15;
					else if (control_function[j] == OHMD_MENU)
						button = 1;
					else
						button = j;
					if (control_state[j] > 0)
						arvr_api->godot_arvr_set_controller_button(openhmd_data->controller_tracker_mapping[i].tracker, button, true);
					else
						arvr_api->godot_arvr_set_controller_button(openhmd_data->controller_tracker_mapping[i].tracker, button, false);
				}
			};
		};		
	};
};

const godot_arvr_interface_gdnative interface_struct = {
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
	godot_arvr_process
};
