////////////////////////////////////////////////////////////////////////////
// Oculus GDNative module for Godot
//
// Written by Bastiaan "Mux213" Olij, 
// with loads of help from Thomas "Karroffel" Herzog

#ifndef GODOT_OCULUS_H
#define GODOT_OCULUS_H

#include "GodotCalls.h"
#include <openhmd.h>
#include "openhmd_data.h"
#include "openhmd_config.h"
#include "ARVRInterface.h"

// declare our public functions for our ARVR Interface

void GDN_EXPORT godot_oculus_gdnative_init(godot_gdnative_init_options *p_options);
void GDN_EXPORT godot_oculus_gdnative_terminate(godot_gdnative_terminate_options *p_options);
void GDN_EXPORT godot_oculus_gdnative_singleton();
void GDN_EXPORT godot_oculus_nativescript_init(void *p_handle);

#endif /* !GODOT_OCULUS_H */
