////////////////////////////////////////////////////////////////////////////////////////////////
// Just exposing an interface to the OS functions reachable in GDNative
// Basically just ported some of the functions in the godot-cpp

#include "OS.h"

OSNative *OSNative::singleton = NULL;

OSNative *OSNative::get_singleton() {
	if (singleton == NULL) {
		singleton = new OSNative();
	}
	return singleton;
}

void OSNative::cleanup_singleton() {
	if (singleton != NULL) {
		delete singleton;
		singleton = NULL;
	}
}

OSNative::OSNative() {
	_os_singleton = api->godot_global_get_singleton((char *)"OS");
	mb_get_ticks_msec = api->godot_method_bind_get_method("_OS", "get_ticks_msec");
	mb_get_screen_size = api->godot_method_bind_get_method("_OS", "get_screen_size");
	mb_get_current_video_driver = api->godot_method_bind_get_method("_OS", "get_current_video_driver");
	mb_get_native_handle = api->godot_method_bind_get_method("_OS", "get_native_handle");
}

OSNative::~OSNative() {
	// nothing to do here
}

int64_t OSNative::get_ticks_msec() {
	if (mb_get_ticks_msec == NULL) {
		return 0;
	}
	return ___godot_icall_int(mb_get_ticks_msec, _os_singleton);
}

godot_vector2 OSNative::get_screen_size(const int64_t screen) {
	if (mb_get_screen_size == NULL) {
		godot_vector2 empty;
		api->godot_vector2_new(&empty, 0.0f, 0.0f);
		return empty;
	}
	return ___godot_icall_Vector2_int(mb_get_screen_size, _os_singleton, screen);
}

int64_t OSNative::get_current_video_driver() {
	if (mb_get_current_video_driver == NULL) {
		return 0;
	}
	return ___godot_icall_int(mb_get_current_video_driver, _os_singleton);
}

void *OSNative::get_native_handle(godot_int p_handle_type) {
	if (mb_get_native_handle == NULL) {
		return 0;
	}
	return (void *)___godot_icall_int_int(mb_get_native_handle, _os_singleton, p_handle_type);
}
