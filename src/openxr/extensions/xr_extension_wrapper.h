#ifndef XR_EXTENSION_WRAPPER_H
#define XR_EXTENSION_WRAPPER_H

#include "openxr/include/openxr_inc.h"

#include <map>

// Interface for wrapping OpenXR extensions and registering them into
// Godot's OpenXR lifecycle.
class XRExtensionWrapper {
protected:
	XRExtensionWrapper() = default;
	virtual ~XRExtensionWrapper() = default;

public:
	virtual std::map<const char *, bool *> get_request_extensions() = 0;

	virtual void on_instance_initialized(const XrInstance instance) {}

	virtual void *get_system_properties(void **property_next) { return nullptr; }

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
};

#endif // XR_EXTENSION_WRAPPER_H
