////////////////////////////////////////////////////////////////////////////////////////////////
// Shared data structure for our OpenHMD GDNative module

#ifndef OPENHMD_DATA_H
#define OPENHMD_DATA_H

#include "GodotCalls.h"
#include <openhmd.h>
#include "openhmd_shader.h"

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
	float oversample;
	ohmd_context *ohmd_ctx; /* OpenHMD context we're using */
	ohmd_device_settings *ohmd_settings; /* Settings we're using */
	ohmd_device *hmd_device; /* HMD device we're rendering to */
	ohmd_device *tracking_device; /* if not NULL, alternative device we're using to track the position and orientation of our HMD */
	openhmd_controller_tracker controller_tracker_mapping[16];
} openhmd_data_struct;

openhmd_data_struct *openhmd_data;

openhmd_data_struct *get_openhmd_data();
void release_openhmd_data();
void openhmd_scan_for_devices();
int openhmd_device_count();
const char * openhmd_get_device_vendor(int p_device);
const char * openhmd_get_device_product(int p_device);
void openhmd_close_hmd_device();
bool openhmd_init_hmd_device(int p_device);
void openhmd_close_tracking_device();
bool openhmd_init_tracking_device(int p_device);
void openhmd_close_controller_device(int p_index);
bool openhmd_init_controller_device(int p_device);
float openhmd_get_oversample();
void openhmd_set_oversample(float p_new_value);

#endif /* !OPENHMD_DATA_H */

