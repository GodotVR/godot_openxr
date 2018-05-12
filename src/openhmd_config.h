////////////////////////////////////////////////////////////////////////////////////////////////
// GDNativeScript OpenHMD configuration object for our OpenHMD GDNative module

#ifndef OPENHMD_CONFIG_H
#define OPENHMD_CONFIG_H

#include "openhmd_data.h"

GDCALLINGCONV void *openhmd_config_constructor(godot_object *p_instance, void *p_method_data);
GDCALLINGCONV void openhmd_config_destructor(godot_object *p_instance, void *p_method_data, void *p_user_data);
GDCALLINGCONV godot_variant openhmd_config_scan_for_devices(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
GDCALLINGCONV godot_variant openhmd_config_list_devices(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
GDCALLINGCONV godot_variant openhmd_config_init_hmd_device(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
GDCALLINGCONV godot_variant openhmd_config_close_hmd_device(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
GDCALLINGCONV godot_variant openhmd_config_init_tracking_device(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
GDCALLINGCONV godot_variant openhmd_config_close_tracking_device(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);
GDCALLINGCONV godot_variant openhmd_config_init_controller_device(godot_object *p_instance, void *p_method_data, void *p_user_data, int p_num_args, godot_variant **p_args);

#endif /* !OPENHMD_CONFIG_H */