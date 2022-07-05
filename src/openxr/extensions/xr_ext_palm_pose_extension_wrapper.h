#ifndef XR_PALM_POSE_EXTENSION_WRAPPER_H
#define XR_PALM_POSE_EXTENSION_WRAPPER_H

#include "openxr/OpenXRApi.h"
#include "openxr/include/openxr_inc.h"
#include "xr_extension_wrapper.h"

#include <map>

// Wrapper for the XR_EXT_PALM_POSE_EXTENSION_NAME extension.
class XRExtPalmPoseExtensionWrapper : public XRExtensionWrapper {
public:
	static XRExtPalmPoseExtensionWrapper *get_singleton();

protected:
	XRExtPalmPoseExtensionWrapper();
	~XRExtPalmPoseExtensionWrapper();

	virtual bool path_is_supported(const godot::String &p_io_path) override;

private:
	static XRExtPalmPoseExtensionWrapper *singleton;

	OpenXRApi *openxr_api = nullptr;
	bool palm_pose_ext = false;
};

#endif // !XR_PALM_POSE_EXTENSION_WRAPPER_H
