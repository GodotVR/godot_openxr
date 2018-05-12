////////////////////////////////////////////////////////////////////////////////////////////////
// GDNativeScript OpenHMD configuration object for our OpenHMD GDNative module

#include "openhmd_config.h"

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
