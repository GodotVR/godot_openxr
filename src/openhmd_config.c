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

GDCALLINGCONV godot_variant openhmd_config_list_devices(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args) {
	godot_variant ret;
	const char device_no_key[] = "device_no";
	const char vendor_key[] = "vendor";
	const char product_key[] = "product";

	if (p_user_data == NULL || openhmd_data != p_user_data) {
		// this should never ever ever ever happen, just being paranoid....
		api->godot_variant_new_nil(&ret);
	} else {
		godot_array devices;
		api->godot_array_new(&devices);

		int num_devices = openhmd_device_count();
		for (int device = 0; device < num_devices; device++) {
			godot_variant v, k;
			godot_string s;
			godot_dictionary device_info;
			api->godot_dictionary_new(&device_info);

			// there has to be a simpler way to add one entry! geez....

			// add device no
			api->godot_string_new(&s);
			api->godot_string_parse_utf8(&s, device_no_key);
			api->godot_variant_new_string(&k, &s);
			api->godot_string_destroy(&s);
			api->godot_variant_new_int(&v, device);
			api->godot_dictionary_set(&device_info, &k, &v);
			api->godot_variant_destroy(&v);
			api->godot_variant_destroy(&k);

			// add device vendor
			api->godot_string_new(&s);
			api->godot_string_parse_utf8(&s, vendor_key);
			api->godot_variant_new_string(&k, &s);
			api->godot_string_destroy(&s);
			api->godot_string_new(&s);
			api->godot_string_parse_utf8(&s, openhmd_get_device_vendor(device));
			api->godot_variant_new_string(&v, &s);
			api->godot_string_destroy(&s);
			api->godot_dictionary_set(&device_info, &k, &v);
			api->godot_variant_destroy(&v);
			api->godot_variant_destroy(&k);

			// add device product
			api->godot_string_new(&s);
			api->godot_string_parse_utf8(&s, product_key);
			api->godot_variant_new_string(&k, &s);
			api->godot_string_destroy(&s);
			api->godot_string_new(&s);
			api->godot_string_parse_utf8(&s, openhmd_get_device_product(device));
			api->godot_variant_new_string(&v, &s);
			api->godot_string_destroy(&s);
			api->godot_dictionary_set(&device_info, &k, &v);
			api->godot_variant_destroy(&v);
			api->godot_variant_destroy(&k);


			api->godot_variant_new_dictionary(&v, &device_info);
			api->godot_array_push_back(&devices, &v);

			// cleanup
			api->godot_variant_destroy(&v);
			api->godot_dictionary_destroy(&device_info);
		}

		api->godot_variant_new_array(&ret, &devices);
		api->godot_array_destroy(&devices);
	}

	return ret;
}	

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
