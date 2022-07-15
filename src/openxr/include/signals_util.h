#ifndef SIGNALS_UTIL_H
#define SIGNALS_UTIL_H

#include <core/GodotGlobal.hpp>

using namespace godot;

static const char *SIGNAL_SESSION_BEGUN = "openxr_session_begun";
static const char *SIGNAL_SESSION_ENDING = "openxr_session_ending";
static const char *SIGNAL_SESSION_IDLE = "openxr_session_idle";
static const char *SIGNAL_SESSION_SYNCHRONIZED = "openxr_session_synchronized";
static const char *SIGNAL_SESSION_LOSS_PENDING = "openxr_session_loss_pending";
static const char *SIGNAL_SESSION_EXITING = "openxr_session_exiting";
static const char *SIGNAL_FOCUSED_STATE = "openxr_focused_state";
static const char *SIGNAL_VISIBLE_STATE = "openxr_visible_state";
static const char *SIGNAL_POSE_RECENTERED = "openxr_pose_recentered";

static void register_plugin_signals() {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	if (!arvr_server) {
		Godot::print_error("Unable to retrieve ARVRServer singleton", __FUNCTION__, __FILE__, __LINE__);
		return;
	}

	// Register our signals
	arvr_server->add_user_signal(SIGNAL_SESSION_BEGUN);
	arvr_server->add_user_signal(SIGNAL_SESSION_ENDING);
	arvr_server->add_user_signal(SIGNAL_SESSION_IDLE);
	arvr_server->add_user_signal(SIGNAL_SESSION_SYNCHRONIZED);
	arvr_server->add_user_signal(SIGNAL_SESSION_LOSS_PENDING);
	arvr_server->add_user_signal(SIGNAL_SESSION_EXITING);
	arvr_server->add_user_signal(SIGNAL_FOCUSED_STATE);
	arvr_server->add_user_signal(SIGNAL_VISIBLE_STATE);
	arvr_server->add_user_signal(SIGNAL_POSE_RECENTERED);
}

template <class... Args>
static void emit_plugin_signal(const String signal, Args... args) {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	if (!arvr_server) {
		Godot::print_error("Unable to retrieve ARVRServer singleton", __FUNCTION__, __FILE__, __LINE__);
		return;
	}

	if (!arvr_server->has_signal(signal)) {
		Godot::print_error("Invalid signal " + signal, __FUNCTION__, __FILE__, __LINE__);
		return;
	}

	arvr_server->emit_signal(signal, args...);
}

#endif // SIGNALS_UTIL_H
