////////////////////////////////////////////////////////////////////////////
// OpenXR GDNative module for Godot
//
// Based on code written by Bastiaan "Mux213" Olij,
// with loads of help from Thomas "Karroffel" Herzog

#include "godot_openxr.h"

void GDN_EXPORT godot_openxr_gdnative_singleton() {
	if (arvr_api != NULL) {
		arvr_api->godot_arvr_register_interface(&interface_struct);
	}
}

void GDN_EXPORT godot_opexr_nativescript_init(void *p_handle) {
	if (nativescript_api == NULL) {
		return;
	}
}
