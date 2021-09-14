////////////////////////////////////////////////////////////////////////////
// OpenXR GDExtension module for Godot

#include "register_types.h"

#include <godot/gdnative_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/xr_server.hpp>

#include "gdclasses/actions/OpenXRActionSets.h"
#include "gdclasses/actions/OpenXRActionSet.h"
#include "gdclasses/actions/OpenXRAction.h"
#include "gdclasses/actions/OpenXRBinding.h"
#include "gdclasses/actions/OpenXRInteractionProfile.h"

#include "gdclasses/XRInterfaceOpenXR.h"
// #include "gdclasses/OpenXRHand.h"
// #include "gdclasses/OpenXRSkeleton.h"

using namespace godot;

Ref<XRInterfaceOpenXR> xr_interface_openxr;

void register_scene_types() {
	ClassDB::register_class<OpenXRActionSets>();
	ClassDB::register_class<OpenXRActionSet>();
	ClassDB::register_class<OpenXRAction>();
	ClassDB::register_class<OpenXRInteractionProfile>();
	ClassDB::register_class<OpenXRBinding>();
}

void unregister_scene_types() {

}

void register_editor_types() {

}

void unregister_editor_types() {

}

void register_driver_types() {
	ClassDB::register_class<XRInterfaceOpenXR>();

	// disabled for now, having some issues with them
	// ClassDB::register_class<OpenXRHand>();
	// ClassDB::register_class<OpenXRSkeleton>();

	XRServer *xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	xr_interface_openxr.instantiate();
	xr_server->add_interface(xr_interface_openxr);
}

void unregister_driver_types() {
	if (xr_interface_openxr.is_valid()) {
		if (xr_interface_openxr->is_initialized()) {
			xr_interface_openxr->uninitialize();
		}

		XRServer *xr_server = XRServer::get_singleton();
		ERR_FAIL_NULL(xr_server);
		xr_server->remove_interface(xr_interface_openxr);

		xr_interface_openxr.unref();
	}

	// Note: our class will be unregistered automatically
}

extern "C" {
// Initialization.

GDNativeBool GDN_EXPORT openxr_library_init(const GDNativeInterface *p_interface, const GDNativeExtensionClassLibraryPtr p_library, GDNativeInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);

	// We register our resources in our scene 
	init_obj.register_scene_initializer(register_scene_types);
	init_obj.register_scene_terminator(unregister_scene_types);

	// We register our editor classes here, they are only registered if we're using the editor
	init_obj.register_editor_initializer(register_editor_types);
	init_obj.register_editor_terminator(unregister_editor_types);
	
	// Finally we register our openxr interface at our driver init
	init_obj.register_driver_initializer(register_driver_types);
	init_obj.register_driver_terminator(unregister_driver_types);

	return init_obj.init();
}
}
