#ifndef XR_FB_COLOR_SPACE_EXTENSION_WRAPPER_H
#define XR_FB_COLOR_SPACE_EXTENSION_WRAPPER_H

#include "openxr/OpenXRApi.h"
#include "openxr/include/openxr_inc.h"
#include "xr_extension_wrapper.h"

#include <map>

// Wrapper for the XR_FB_COLOR_SPACE_EXTENSION_NAME extension.
class XRFbColorSpaceExtensionWrapper : public XRExtensionWrapper {
public:
	static XRFbColorSpaceExtensionWrapper *get_singleton();

	void **set_system_properties_and_get_next_pointer(void **property) override;

	void on_instance_initialized(const XrInstance instance) override;

	uint32_t get_color_space() const;
	void set_color_space(const uint32_t p_color_space);
	godot::Dictionary get_available_color_spaces();

protected:
	XRFbColorSpaceExtensionWrapper();
	~XRFbColorSpaceExtensionWrapper();

private:
	void cleanup();

	static XRFbColorSpaceExtensionWrapper *singleton;

	const char *get_color_space_enum_desc(XrColorSpaceFB p_color_space);

	OpenXRApi *openxr_api = nullptr;
	bool fb_color_space_ext = false;
	XrSystemColorSpacePropertiesFB color_space_properties;

	// OpenXR API call wrappers
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateColorSpacesFB, (XrSession), session, (uint32_t), colorSpaceCapacityInput, (uint32_t *), colorSpaceCountOutput, (XrColorSpaceFB *), colorSpaces);
	EXT_PROTO_XRRESULT_FUNC2(xrSetColorSpaceFB, (XrSession), session, (const XrColorSpaceFB), colorspace);
};

#endif // !XR_FB_COLOR_SPACE_EXTENSION_WRAPPER_H
