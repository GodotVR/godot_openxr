#include "xr_fb_color_space_extension_wrapper.h"

#include <core/Variant.hpp>

using namespace godot;

XRFbColorSpaceExtensionWrapper *XRFbColorSpaceExtensionWrapper::singleton = nullptr;

XRFbColorSpaceExtensionWrapper *XRFbColorSpaceExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRFbColorSpaceExtensionWrapper();
	}

	return singleton;
}

XRFbColorSpaceExtensionWrapper::XRFbColorSpaceExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	request_extensions[XR_FB_COLOR_SPACE_EXTENSION_NAME] = &fb_color_space_ext;
	color_space_properties.colorSpace = XR_COLOR_SPACE_UNMANAGED_FB;
}

XRFbColorSpaceExtensionWrapper::~XRFbColorSpaceExtensionWrapper() {
	cleanup();
	OpenXRApi::openxr_release_api();
}

void **XRFbColorSpaceExtensionWrapper::set_system_properties_and_get_next_pointer(void **property) {
	if (fb_color_space_ext) {
		color_space_properties.type = XR_TYPE_SYSTEM_COLOR_SPACE_PROPERTIES_FB;
		color_space_properties.next = nullptr;
		color_space_properties.colorSpace = XR_COLOR_SPACE_UNMANAGED_FB; // on success this will be overwritten with the current color space

		*property = &color_space_properties;
		return &color_space_properties.next;
	} else {
		return nullptr;
	}
}

void XRFbColorSpaceExtensionWrapper::cleanup() {
	fb_color_space_ext = false;
}

void XRFbColorSpaceExtensionWrapper::on_instance_initialized(const XrInstance instance) {
	if (fb_color_space_ext) {
		XrResult result = initialise_fb_color_space_extension(instance);
		if (!openxr_api->xr_result(result, "Failed to initialise color space extension")) {
			fb_color_space_ext = false; // I guess we don't support it...
		}
	}
}

uint32_t XRFbColorSpaceExtensionWrapper::get_color_space() const {
	return color_space_properties.colorSpace;
}

void XRFbColorSpaceExtensionWrapper::set_color_space(const uint32_t p_color_space) {
	if (fb_color_space_ext) {
		XrResult result = xrSetColorSpaceFB(openxr_api->get_session(), XrColorSpaceFB(p_color_space));
		if (!openxr_api->xr_result(result, "Failed to set color space")) {
			return;
		}

		// We successfully set this so remember the new value...
		color_space_properties.colorSpace = XrColorSpaceFB(p_color_space);
	}
}

godot::Dictionary XRFbColorSpaceExtensionWrapper::get_available_color_spaces() {
	godot::Dictionary dict;

	if (fb_color_space_ext) {
		uint32_t color_space_count;
		XrResult result = xrEnumerateColorSpacesFB(openxr_api->get_session(), 0, &color_space_count, nullptr);
		if (!openxr_api->xr_result(result, "Failed to enumerate color spaces")) {
			return dict;
		}

		if (color_space_count == 0) {
			return dict;
		}

		XrColorSpaceFB *color_spaces = (XrColorSpaceFB *)malloc(sizeof(color_spaces) * color_space_count);
		result = xrEnumerateColorSpacesFB(openxr_api->get_session(), color_space_count, &color_space_count, color_spaces);
		if (!openxr_api->xr_result(result, "Failed to enumerate color spaces")) {
			free(color_spaces);
			return dict;
		}

		for (uint32_t i = 0; i < color_space_count; i++) {
			unsigned int value = color_spaces[i]; // cast so we get the correct variant
			dict[Variant(value)] = get_color_space_enum_desc(color_spaces[i]);
		}

		free(color_spaces);
	}

	return dict;
}

const char *XRFbColorSpaceExtensionWrapper::get_color_space_enum_desc(XrColorSpaceFB p_color_space) {
	switch (p_color_space) {
		case XR_COLOR_SPACE_UNMANAGED_FB: {
			return "XR_COLOR_SPACE_UNMANAGED_FB";
		}; break;
		case XR_COLOR_SPACE_REC2020_FB: {
			return "XR_COLOR_SPACE_REC2020_FB";
		}; break;
		case XR_COLOR_SPACE_REC709_FB: {
			return "XR_COLOR_SPACE_REC709_FB";
		}; break;
		case XR_COLOR_SPACE_RIFT_CV1_FB: {
			return "XR_COLOR_SPACE_RIFT_CV1_FB";
		}; break;
		case XR_COLOR_SPACE_RIFT_S_FB: {
			return "XR_COLOR_SPACE_RIFT_S_FB";
		}; break;
		case XR_COLOR_SPACE_QUEST_FB: {
			return "XR_COLOR_SPACE_QUEST_FB";
		}; break;
		case XR_COLOR_SPACE_P3_FB: {
			return "XR_COLOR_SPACE_P3_FB";
		}; break;
		case XR_COLOR_SPACE_ADOBE_RGB_FB: {
			return "XR_COLOR_SPACE_ADOBE_RGB_FB";
		}; break;
		case XR_COLOR_SPACE_MAX_ENUM_FB: {
			return "XR_COLOR_SPACE_MAX_ENUM_FB";
		}; break;
		default: {
			return "Unknown";
		}; break;
	}
}

PFN_xrEnumerateColorSpacesFB xrEnumerateColorSpacesFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbColorSpaceExtensionWrapper::xrEnumerateColorSpacesFB(
		XrSession session,
		uint32_t colorSpaceCapacityInput,
		uint32_t *colorSpaceCountOutput,
		XrColorSpaceFB *colorSpaces) {
	if (xrEnumerateColorSpacesFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrEnumerateColorSpacesFB_ptr)(session, colorSpaceCapacityInput, colorSpaceCountOutput, colorSpaces);
}

PFN_xrSetColorSpaceFB xrSetColorSpaceFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbColorSpaceExtensionWrapper::xrSetColorSpaceFB(
		XrSession session,
		const XrColorSpaceFB colorspace) {
	if (xrSetColorSpaceFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrSetColorSpaceFB_ptr)(session, colorspace);
}

XrResult XRFbColorSpaceExtensionWrapper::initialise_fb_color_space_extension(XrInstance instance) {
	XrResult result;

	result = xrGetInstanceProcAddr(instance, "xrEnumerateColorSpacesFB", (PFN_xrVoidFunction *)&xrEnumerateColorSpacesFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}
	result = xrGetInstanceProcAddr(instance, "xrSetColorSpaceFB", (PFN_xrVoidFunction *)&xrSetColorSpaceFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	return XR_SUCCESS;
}
