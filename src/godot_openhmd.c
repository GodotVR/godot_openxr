////////////////////////////////////////////////////////////////////////////
// OpenHMD ARVR module for the Godot game engine
//
// Written by Bastiaan "Mux213" Olij, with loads of help from Thomas "Karroffel" Herzog

#include "godot_openhmd.h"

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
