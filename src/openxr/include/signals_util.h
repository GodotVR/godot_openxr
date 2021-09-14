#ifndef SIGNALS_UTIL_H
#define SIGNALS_UTIL_H

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

static const char *SIGNAL_SESSION_BEGUN = "openxr_session_begun";
static const char *SIGNAL_SESSION_ENDING = "openxr_session_ending";
static const char *SIGNAL_FOCUSED_STATE = "openxr_focused_state";
static const char *SIGNAL_VISIBLE_STATE = "openxr_visible_state";
static const char *SIGNAL_POSE_RECENTERED = "openxr_pose_recentered";

static void register_plugin_signals() {
	XRServer *xr_server = XRServer::get_singleton();
	if (!xr_server) {
		UtilityFunctions::printerr("Unable to retrieve XRServer singleton");
		return;
	}

	// Register our signals
	xr_server->add_user_signal(SIGNAL_SESSION_BEGUN);
	xr_server->add_user_signal(SIGNAL_SESSION_ENDING);
	xr_server->add_user_signal(SIGNAL_FOCUSED_STATE);
	xr_server->add_user_signal(SIGNAL_VISIBLE_STATE);
	xr_server->add_user_signal(SIGNAL_POSE_RECENTERED);
}

template <class... Args>
static void emit_plugin_signal(const String signal, Args... args) {
	XRServer *xr_server = XRServer::get_singleton();
	if (!xr_server) {
		UtilityFunctions::print("Unable to retrieve XRServer singleton");
		return;
	}

	if (!xr_server->has_signal(signal)) {
		UtilityFunctions::print("Invalid signal " + signal);
		return;
	}

	xr_server->emit_signal(signal, args...);
}

#endif // SIGNALS_UTIL_H
