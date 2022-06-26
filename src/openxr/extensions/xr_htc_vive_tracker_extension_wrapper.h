#ifndef XR_HTC_VIVE_TRACKER_EXTENSION_H
#define XR_HTC_VIVE_TRACKER_EXTENSION_H

#include "openxr/OpenXRApi.h"
#include "openxr/include/util.h"
#include "xr_extension_wrapper.h"

class OpenXRInputHTCTracker : public OpenXRInputBase {
public:
	OpenXRInputHTCTracker(const char *p_name);

	virtual void update(OpenXRApi *p_openxr_api) override;
};

class XRHTCViveTrackerExtensionWrapper : public XRExtensionWrapper {
public:
	static XRHTCViveTrackerExtensionWrapper *get_singleton();

	XRHTCViveTrackerExtensionWrapper();
	virtual ~XRHTCViveTrackerExtensionWrapper() override;

	bool is_available();
	virtual void add_input_maps() override;
	virtual bool on_event_polled(const XrEventDataBuffer &event) override;

private:
	static XRHTCViveTrackerExtensionWrapper *singleton;

	OpenXRApi *openxr_api = nullptr;
	bool available = false;
};

#endif // !XR_HTC_VIVE_TRACKER_EXTENSION_H
