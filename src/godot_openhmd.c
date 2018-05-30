////////////////////////////////////////////////////////////////////////////
// OpenHMD ARVR module for the Godot game engine
//
// Written by Bastiaan "Mux213" Olij, with loads of help from Thomas "Karroffel" Herzog

#include "godot_openhmd.h"

void GDN_EXPORT godot_gdnative_singleton() {
	if (arvr_api != NULL) {
		arvr_api->godot_arvr_register_interface(&interface_struct);
	}
}

void GDN_EXPORT godot_nativescript_init(void *p_handle) {
	if (nativescript_api == NULL) {
		return;
	}

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
		get_data.method = &openhmd_config_list_devices;

		godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

		nativescript_api->godot_nativescript_register_method(p_handle, "OpenHMDConfig", "list_devices", attributes, get_data);
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

	{
		godot_instance_method get_data = { NULL, NULL, NULL };
		get_data.method = &openhmd_config_get_oversample;

		godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

		nativescript_api->godot_nativescript_register_method(p_handle, "OpenHMDConfig", "get_oversample", attributes, get_data);
	}

	{
		godot_instance_method get_data = { NULL, NULL, NULL };
		get_data.method = &openhmd_config_set_oversample;

		godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

		nativescript_api->godot_nativescript_register_method(p_handle, "OpenHMDConfig", "set_oversample", attributes, get_data);
	}

}
