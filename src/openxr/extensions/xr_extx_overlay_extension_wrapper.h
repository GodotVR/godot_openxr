#ifndef XR_EXTX_OVERLAY_EXTENSION_WRAPPER_H
#define XR_EXTX_OVERLAY_EXTENSION_WRAPPER_H

#include "openxr/OpenXRApi.h"
#include "openxr/include/openxr_inc.h"
#include "xr_extension_wrapper.h"

#include <map>

// Wrapper for the XR_EXTX_OVERLAY_EXTENSION_NAME extension.
class XRExtxOverlayExtensionWrapper : public XRExtensionWrapper {
public:
	static XRExtxOverlayExtensionWrapper *get_singleton();

	void on_instance_initialized(const XrInstance instance) override;

	void on_instance_destroyed() override;

	int overlay_placement = 1;
	bool main_session_visible = false;
	bool extx_overlay_ext = false;

protected:
	XRExtxOverlayExtensionWrapper();
	~XRExtxOverlayExtensionWrapper();

private:
	static XrResult initialise_extx_overlay_extension(XrInstance instance);

	void cleanup();

	static XRExtxOverlayExtensionWrapper *singleton;

	OpenXRApi *openxr_api = nullptr;
};

#endif // XR_EXTX_OVERLAY_EXTENSION_WRAPPER_H
