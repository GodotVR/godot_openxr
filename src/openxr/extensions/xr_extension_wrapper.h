#ifndef XR_EXTENSION_WRAPPER_H
#define XR_EXTENSION_WRAPPER_H

#include "openxr/include/openxr_inc.h"
#include <String.hpp>

#include <map>

// Interface for wrapping OpenXR extensions and registering them into
// Godot's OpenXR lifecycle.
class XRExtensionWrapper {
protected:
	XRExtensionWrapper() = default;
	virtual ~XRExtensionWrapper() = default;

public:
	virtual std::map<const char *, bool *> get_request_extensions() {
		return request_extensions;
	}

	virtual void on_instance_initialized(const XrInstance instance) {}

	virtual void **set_system_properties_and_get_next_pointer(void **property) { return nullptr; }

	virtual void **set_swapchain_create_info_and_get_next_pointer(void **swapchain_create_info) { return nullptr; }

	virtual void on_session_initialized(const XrSession session) {}

	virtual void on_state_idle() {}

	virtual void on_state_ready() {}

	virtual void on_state_synchronized() {}

	virtual void on_state_visible() {}

	virtual void on_state_focused() {}

	virtual void on_process_openxr() {}

	virtual void on_state_stopping() {}

	virtual void on_state_loss_pending() {}

	virtual void on_state_exiting() {}

	virtual void on_session_destroyed() {}

	virtual void on_instance_destroyed() {}

	// Returns true if the event was handled, false otherwise.
	virtual bool on_event_polled(const XrEventDataBuffer &event) {
		return false;
	}

	// Return false if we can't use this input path
	virtual bool path_is_supported(const godot::String &p_io_path) {
		return true;
	}

protected:
	std::map<const char *, bool *> request_extensions;
};

#endif // XR_EXTENSION_WRAPPER_H
