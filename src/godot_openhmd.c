////////////////////////////////////////////////////////////////////////////
// OpenHMD ARVR module for the Godot game engine
//
// Written by Bastiaan "Mux213" Olij, with loads of help from Thomas "Karroffel" Herzog

#include <gdnative_api_struct.gen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openhmd.h>
#include "openhmd_shader.h"

const godot_gdnative_core_api_struct *api = NULL;
const godot_gdnative_ext_arvr_api_struct *arvr_api = NULL;
const godot_gdnative_ext_nativescript_api_struct *nativescript_api = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////
// OpenHMD data container

#define OPENHMD_MAX_CONTROLLERS 16

typedef struct openhmd_controller_tracker {
	ohmd_device *device;
	godot_int tracker;
} openhmd_controller_tracker;

typedef struct openhmd_data_struct {
	int use_count; // should always be 1!
	bool do_auto_init_device_zero;
	int num_devices;
	int width, height;
	int oversample_scale;
	ohmd_context *ohmd_ctx; /* OpenHMD context we're using */
	ohmd_device_settings *ohmd_settings; /* Settings we're using */
	ohmd_device *hmd_device; /* HMD device we're rendering to */
	ohmd_device *tracking_device; /* if not NULL, alternative device we're using to track the position and orientation of our HMD */
	openhmd_controller_tracker controller_tracker_mapping[16];
} openhmd_data_struct;

// We should only have one instance of our ARVR Interface registered with our server, we'll keep a single reference to our data structure
// Note that we should have our interface, even when not needed (yet), loaded automatically at startup, so basically this structure will
// be accessible from the get go.
openhmd_data_struct *openhmd_data = NULL;

openhmd_data_struct *get_openhmd_data() {
	if (openhmd_data == NULL) {
		openhmd_data = api->godot_alloc(sizeof(openhmd_data_struct));
		openhmd_data->use_count = 1;
		openhmd_data->do_auto_init_device_zero = true;
		openhmd_data->num_devices = 0;
		openhmd_data->width = 0;
		openhmd_data->height = 0;
		openhmd_data->oversample_scale = 2;
		openhmd_data->ohmd_ctx = NULL;
		openhmd_data->ohmd_settings = NULL;
		openhmd_data->hmd_device = NULL;
		openhmd_data->tracking_device = NULL;

		for (int i = 0; i < OPENHMD_MAX_CONTROLLERS; i++) {
			openhmd_data->controller_tracker_mapping[i].device = 0;
			openhmd_data->controller_tracker_mapping[i].tracker = 0;
		}
	} else {
		openhmd_data->use_count++;
	}

	return openhmd_data;
};

void release_openhmd_data() {
	if (openhmd_data != NULL) {
		if (openhmd_data->use_count > 1) {
			openhmd_data->use_count--;
		} else if (openhmd_data->use_count == 1) {
			api->godot_free(openhmd_data);
			openhmd_data = NULL;
		}
	}
};

void openhmd_scan_for_devices() {
	if (openhmd_data == NULL) {
		// Not yet initialised!
	} else if (openhmd_data->ohmd_ctx == NULL) {
		// Not yet initialised!
	} else {
		// Calling ohmd_ctx_probe will initialize our list of active devices.
		// Until it is called again our indices should not change.
		openhmd_data->num_devices = ohmd_ctx_probe(openhmd_data->ohmd_ctx);
		if (openhmd_data->num_devices < 0) {
			printf("OpenHMD: failed to get device count - %s\n", ohmd_ctx_get_error(openhmd_data->ohmd_ctx));
		} else {
			// just output them for now while we're debugging, this needs to become accessible from GDScript
			for (int i = 0; i < openhmd_data->num_devices; i++) {
				printf("OpenHMD: found %i %s - %s\n", i, ohmd_list_gets(openhmd_data->ohmd_ctx, i, OHMD_VENDOR), ohmd_list_gets(openhmd_data->ohmd_ctx, i, OHMD_PRODUCT));
			};
		};
	};
};

void openhmd_close_hmd_device() {
	if (openhmd_data == NULL) {
		// Not yet initialised!
	} else if (openhmd_data->ohmd_ctx == NULL) {
		// Not yet initialised!
	} else if (openhmd_data->hmd_device != NULL) {
		printf("Closing HMD OpenHMD device\n");

		ohmd_close_device(openhmd_data->hmd_device);
		openhmd_data->hmd_device = NULL;
	};
};

bool openhmd_init_hmd_device(int p_device) {
	if (openhmd_data == NULL) {
		// Not yet initialised!
		return false;
	} else if (openhmd_data->ohmd_ctx == NULL) {
		// Not yet initialised!
		return false;
	} else {
		if (openhmd_data->hmd_device != NULL) {
			openhmd_close_hmd_device();
		}

		if (openhmd_data->num_devices <= p_device) {
			printf("OpenHMD: Device ID out of bounds\n");
			return false;
		};

		printf("Initialising device no %i as the HMD device\n", p_device);

		// create our device instance
		openhmd_data->hmd_device = ohmd_list_open_device_s(openhmd_data->ohmd_ctx, p_device, openhmd_data->ohmd_settings);
		if (openhmd_data->hmd_device == NULL) {
			printf("OpenHMD: failed to open device - %s\n", ohmd_ctx_get_error(openhmd_data->ohmd_ctx));
			return false;
		} else {
			// get resolution
			ohmd_device_geti(openhmd_data->hmd_device, OHMD_SCREEN_HORIZONTAL_RESOLUTION, &openhmd_data->width);
			openhmd_data->width /= 2; /* need half this */
			ohmd_device_geti(openhmd_data->hmd_device, OHMD_SCREEN_VERTICAL_RESOLUTION, &openhmd_data->height);

			// now copy some of these into our shader..
			openhmd_shader_set_device_parameters(openhmd_data->hmd_device);

			// need to check if we can actually use this device!

			// need to open up a window so we can render to the monitor this device relates too

			printf("OpenHMD: initialized hmd %s - %s\n", ohmd_list_gets(openhmd_data->ohmd_ctx, p_device, OHMD_VENDOR), ohmd_list_gets(openhmd_data->ohmd_ctx, p_device, OHMD_PRODUCT));
		};

		return true;
	};
};

void openhmd_close_tracking_device() {
	if (openhmd_data == NULL) {
		// Not yet initialised!
	} else if (openhmd_data->ohmd_ctx == NULL) {
		// Not yet initialised!
	} else if (openhmd_data->tracking_device != NULL) {
		printf("Closing tracking OpenHMD device\n");

		ohmd_close_device(openhmd_data->tracking_device);
		openhmd_data->tracking_device = NULL;
	};
};

bool openhmd_init_tracking_device(int p_device) {
	if (openhmd_data == NULL) {
		// Not yet initialised!
		return false;
	} else if (openhmd_data->ohmd_ctx == NULL) {
		// Not yet initialised!
		return false;
	} else {
		if (openhmd_data->tracking_device != NULL) {
			openhmd_close_tracking_device();
		}

		if (openhmd_data->num_devices <= p_device) {
			printf("OpenHMD: Device ID out of bounds\n");
			return false;
		};

		printf("Initialising device no %i as the tracking device\n", p_device);

		// create our device instance
		openhmd_data->tracking_device = ohmd_list_open_device_s(openhmd_data->ohmd_ctx, p_device, openhmd_data->ohmd_settings);
		if (openhmd_data->tracking_device == NULL) {
			printf("OpenHMD: failed to open device - %s\n", ohmd_ctx_get_error(openhmd_data->ohmd_ctx));
			return false;
		} else {
			printf("OpenHMD: initialized tracking %s - %s\n", ohmd_list_gets(openhmd_data->ohmd_ctx, p_device, OHMD_VENDOR), ohmd_list_gets(openhmd_data->ohmd_ctx, p_device, OHMD_PRODUCT));
		};

		return true;
	};
};

void openhmd_close_controller_device(int p_index) {
	if (openhmd_data == NULL) {
		// Not yet initialised!
	} else if (openhmd_data->ohmd_ctx == NULL) {
		// Not yet initialised!
	} else if (openhmd_data->controller_tracker_mapping[p_index].device != NULL) {
		printf("Closing controller OpenHMD device\n");

		ohmd_close_device(openhmd_data->controller_tracker_mapping[p_index].device);
		openhmd_data->controller_tracker_mapping[p_index].device = NULL;

		arvr_api->godot_arvr_remove_controller(openhmd_data->controller_tracker_mapping[p_index].tracker);
		openhmd_data->controller_tracker_mapping[p_index].tracker = 0;
	};
};

bool openhmd_init_controller_device(int p_device) {
	if (openhmd_data == NULL) {
		// Not yet initialised!
		return false;
	} else if (openhmd_data->ohmd_ctx == NULL) {
		// Not yet initialised!
		return false;
	} else {
		// find an empty slot...
		int i = 0;
		while (i < OPENHMD_MAX_CONTROLLERS && openhmd_data->controller_tracker_mapping[i].device != NULL) {
			i++;
		};
		if (i == OPENHMD_MAX_CONTROLLERS) {
			return false;
		};

		openhmd_data->controller_tracker_mapping[i].device = ohmd_list_open_device_s(openhmd_data->ohmd_ctx, p_device, openhmd_data->ohmd_settings);
		if (openhmd_data->controller_tracker_mapping[i].device != NULL) {
			char device_name[256];
			godot_int hand = 0;

			sprintf(device_name,"%s_%i",ohmd_list_gets(openhmd_data->ohmd_ctx, p_device, OHMD_PRODUCT),i);
			openhmd_data->controller_tracker_mapping[i].tracker = arvr_api->godot_arvr_add_controller(device_name, hand, true, true);

			printf("OpenHMD: initialized controller %s - %s\n", ohmd_list_gets(openhmd_data->ohmd_ctx, p_device, OHMD_VENDOR), ohmd_list_gets(openhmd_data->ohmd_ctx, p_device, OHMD_PRODUCT));
		};

		return true;
	};
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Our OpenHMD configuration interface

GDCALLINGCONV void *openhmd_config_constructor(godot_object *p_instance, void *p_method_data) {
	return get_openhmd_data();
}

GDCALLINGCONV void openhmd_config_destructor(godot_object *p_instance, void *p_method_data, void *p_user_data) {
	if (p_user_data == NULL || openhmd_data != p_user_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else {
		release_openhmd_data();
	}
}

GDCALLINGCONV godot_variant openhmd_config_scan_for_devices(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
	godot_variant	ret;

	if (p_user_data == NULL || openhmd_data != p_user_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else {
		openhmd_scan_for_devices();
	};

	// just return true
	api->godot_variant_new_bool(&ret, true);
	return ret;
};

GDCALLINGCONV godot_variant openhmd_config_init_hmd_device(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
	godot_variant	ret;

	if (p_user_data == NULL || openhmd_data != p_user_data) {
		// this should never ever ever ever happen, just being paranoid....
		api->godot_variant_new_bool(&ret, false);
	} else if (p_num_args == 0) {
		// huh?
		api->godot_variant_new_bool(&ret, false);
	} else if (openhmd_init_hmd_device(api->godot_variant_as_uint(p_args[0]))) {
		api->godot_variant_new_bool(&ret, true);
	} else {
		api->godot_variant_new_bool(&ret, false);
	};

	return ret;
};

GDCALLINGCONV godot_variant openhmd_config_close_hmd_device(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
	godot_variant	ret;

	if (p_user_data == NULL || openhmd_data != p_user_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else {
		openhmd_close_hmd_device();
	};

	// just return true
	api->godot_variant_new_bool(&ret, true);
	return ret;
};

GDCALLINGCONV godot_variant openhmd_config_init_tracking_device(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
	godot_variant	ret;

	if (p_user_data == NULL || openhmd_data != p_user_data) {
		// this should never ever ever ever happen, just being paranoid....
		api->godot_variant_new_bool(&ret, false);
	} else if (p_num_args == 0) {
		// huh?
		api->godot_variant_new_bool(&ret, false);
	} else if (openhmd_init_tracking_device(api->godot_variant_as_uint(p_args[0]))) {
		api->godot_variant_new_bool(&ret, true);
	} else {
		api->godot_variant_new_bool(&ret, false);
	};

	return ret;
};

GDCALLINGCONV godot_variant openhmd_config_close_tracking_device(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
	godot_variant	ret;

	if (p_user_data == NULL || openhmd_data != p_user_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else {
		openhmd_close_tracking_device();
	};

	// just return true
	api->godot_variant_new_bool(&ret, true);
	return ret;
};

GDCALLINGCONV godot_variant openhmd_config_init_controller_device(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
	godot_variant	ret;

	if (p_user_data == NULL || openhmd_data != p_user_data) {
		// this should never ever ever ever happen, just being paranoid....
		api->godot_variant_new_bool(&ret, false);
	} else if (p_num_args == 0) {
		// huh?
		api->godot_variant_new_bool(&ret, false);
	} else if (openhmd_init_controller_device(api->godot_variant_as_uint(p_args[0]))) {
		api->godot_variant_new_bool(&ret, true);
	} else {
		api->godot_variant_new_bool(&ret, false);
	};

	return ret;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Our GDNative OpenHMD module

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

		// we turn our automatic updates off, we're calling this from our process call which is called from our main render thread
		// which guarantees this gets called atleast once every frame and as close to our rendering as possible.
		int auto_update = 0;
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

godot_vector2 godot_arvr_get_recommended_render_targetsize(const void *p_data) {
	godot_vector2 size;

	if (p_data == NULL || p_data != openhmd_data) {
		// this should never ever ever ever happen, just being paranoid....
	} else if (openhmd_data->ohmd_ctx != NULL) {
			if (openhmd_data->hmd_device != NULL) {
				api->godot_vector2_new(&size, openhmd_data->width * openhmd_data->oversample_scale, openhmd_data->height * openhmd_data->oversample_scale);
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
			};
		};		
	};
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Library initialisation

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
	godot_arvr_get_recommended_render_targetsize,
	godot_arvr_get_transform_for_eye,
	godot_arvr_fill_projection_for_eye,
	godot_arvr_commit_for_eye,
	godot_arvr_process
};

void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options *p_options) {
	// get our main API struct
	api = p_options->api_struct;

	// now find our arvr extension
	for (int i = 0; i < api->num_extensions; i++) {
		// todo: add version checks
		switch (api->extensions[i]->type) {
			case GDNATIVE_EXT_ARVR: {
				arvr_api = (godot_gdnative_ext_arvr_api_struct *)api->extensions[i];
			}; break;
			case GDNATIVE_EXT_NATIVESCRIPT: {
				nativescript_api = (godot_gdnative_ext_nativescript_api_struct *)api->extensions[i];
			}; break;
			default: break;
		};
	};

	if (!gladLoadGL()) {
		printf("Error initializing GLAD\n");
	}
}

void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *p_options) {
	api = NULL;
}

void GDN_EXPORT godot_gdnative_singleton() {
	arvr_api->godot_arvr_register_interface(&interface_struct);
}

void GDN_EXPORT godot_nativescript_init(void *p_handle) {
	{
		godot_instance_create_func create = { NULL, NULL, NULL };
		create.create_func = &openhmd_config_constructor;

		godot_instance_destroy_func destroy = { NULL, NULL, NULL };
		destroy.destroy_func = &openhmd_config_destructor;

		nativescript_api->godot_nativescript_register_class(p_handle, "OpenHMDConfig", "Reference", create, destroy);
	}

	{
		godot_instance_method get_data = { NULL, NULL, NULL };
		get_data.method = &openhmd_config_scan_for_devices;

		godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

		nativescript_api->godot_nativescript_register_method(p_handle, "OpenHMDConfig", "scan_for_devices", attributes, get_data);
	}

	{
		godot_instance_method get_data = { NULL, NULL, NULL };
		get_data.method = &openhmd_config_init_hmd_device;

		godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

		nativescript_api->godot_nativescript_register_method(p_handle, "OpenHMDConfig", "init_hmd_device", attributes, get_data);
	}

	{
		godot_instance_method get_data = { NULL, NULL, NULL };
		get_data.method = &openhmd_config_close_hmd_device;

		godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

		nativescript_api->godot_nativescript_register_method(p_handle, "OpenHMDConfig", "close_hmd_device", attributes, get_data);
	}

	{
		godot_instance_method get_data = { NULL, NULL, NULL };
		get_data.method = &openhmd_config_init_tracking_device;

		godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

		nativescript_api->godot_nativescript_register_method(p_handle, "OpenHMDConfig", "init_tracking_device", attributes, get_data);
	}

	{
		godot_instance_method get_data = { NULL, NULL, NULL };
		get_data.method = &openhmd_config_close_tracking_device;

		godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

		nativescript_api->godot_nativescript_register_method(p_handle, "OpenHMDConfig", "close_tracking_device", attributes, get_data);
	}

	{
		godot_instance_method get_data = { NULL, NULL, NULL };
		get_data.method = &openhmd_config_init_controller_device;

		godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

		nativescript_api->godot_nativescript_register_method(p_handle, "OpenHMDConfig", "init_controller_device", attributes, get_data);
	}
}
