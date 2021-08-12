////////////////////////////////////////////////////////////////////////////
// OpenXR GDNative module for Godot
//
// Based on code written by Bastiaan "Mux213" Olij,
// with loads of help from Thomas "Karroffel" Herzog

#ifndef GODOT_OPENXR_H
#define GODOT_OPENXR_H

#include "gdclasses/XRInterfaceOpenXR.h"
#include <openxr/openxr.h>

#ifdef __cplusplus
extern "C" {
#endif

// declare our public functions for our ARVR Interface

void GDN_EXPORT godot_openxr_gdnative_init(godot_gdnative_init_options *o);
void GDN_EXPORT godot_openxr_gdnative_terminate(godot_gdnative_terminate_options *o);
void GDN_EXPORT godot_openxr_gdnative_singleton();
void GDN_EXPORT godot_openxr_nativescript_init(void *p_handle);

#ifdef __cplusplus
}
#endif

extern "C" const godot_arvr_interface_gdnative arvr_interface_struct;
extern "C" const godot_gdnative_ext_arvr_1_2_api_struct *arvr_1_2_api;

#endif /* !GODOT_OPENXR_H */
