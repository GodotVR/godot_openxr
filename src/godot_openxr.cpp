////////////////////////////////////////////////////////////////////////////
// OpenXR GDNative module for Godot
//
// Based on code written by Bastiaan "Mux213" Olij,
// with loads of help from Thomas "Karroffel" Herzog

#include "godot_openxr.h"
#include "gdclasses/OpenXRConfig.h"
#include "gdclasses/OpenXRHand.h"
#include "gdclasses/OpenXRPose.h"
#include "gdclasses/OpenXRSkeleton.h"

void GDN_EXPORT godot_openxr_gdnative_init(godot_gdnative_init_options *o) {
	godot::Godot::gdnative_init(o);
}

void GDN_EXPORT godot_openxr_gdnative_terminate(godot_gdnative_terminate_options *o) {
	godot::Godot::gdnative_terminate(o);
}

void GDN_EXPORT godot_openxr_gdnative_singleton() {
	if (godot::arvr_api != nullptr) {
		godot::arvr_api->godot_arvr_register_interface(&interface_struct);
	}
}

void GDN_EXPORT godot_openxr_nativescript_init(void *p_handle) {
	godot::Godot::nativescript_init(p_handle);

	godot::register_tool_class<godot::OpenXRConfig>();
	godot::register_class<godot::OpenXRHand>();
	godot::register_class<godot::OpenXRPose>();
	godot::register_class<godot::OpenXRSkeleton>();
}
