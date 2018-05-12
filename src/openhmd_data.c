////////////////////////////////////////////////////////////////////////////////////////////////
// Shared data structure for our OpenHMD GDNative module

#include "openhmd_data.h"

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
		openhmd_data->oversample = 2.0;
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
//		} else {
//			printf("OpenHMD: found %i devices\n", openhmd_data->num_devices);
		}
	}
}

int openhmd_device_count() {
	if (openhmd_data == NULL) {
		// Not yet initialised!
		return 0;
	} else if (openhmd_data->ohmd_ctx == NULL) {
		// Not yet initialised!
		return 0;
	} else {
		return openhmd_data->num_devices;
	}	
}

const char * openhmd_get_device_vendor(int p_device) {
	static char empty[] = "";
	if (openhmd_data == NULL) {
		// Not yet initialised!
		return empty;
	} else if (openhmd_data->ohmd_ctx == NULL) {
		// Not yet initialised!
		return empty;
	} else if (p_device >= openhmd_data->num_devices) {
		// Out of bounds!
		return empty;
	} else {
		return ohmd_list_gets(openhmd_data->ohmd_ctx, p_device, OHMD_VENDOR);
	}
}

const char * openhmd_get_device_product(int p_device) {
	static char empty[] = "";
	if (openhmd_data == NULL) {
		// Not yet initialised!
		return empty;
	} else if (openhmd_data->ohmd_ctx == NULL) {
		// Not yet initialised!
		return empty;
	} else if (p_device >= openhmd_data->num_devices) {
		// Out of bounds!
		return empty;
	} else {
		return ohmd_list_gets(openhmd_data->ohmd_ctx, p_device, OHMD_PRODUCT);
	}
}

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

float openhmd_get_oversample() {
	if (openhmd_data == NULL) {
		// Not yet initialised!
		return 1.0;
	} else {
		return openhmd_data->oversample;
	}
}

void openhmd_set_oversample(float p_new_value) {
	if (openhmd_data == NULL) {
		// Not yet initialised!
	} else {
		openhmd_data->oversample = p_new_value;		
	}
}
