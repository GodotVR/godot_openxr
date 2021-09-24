#ifndef XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_WRAPPER_H
#define XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_WRAPPER_H

#include "openxr/OpenXRApi.h"
#include "openxr/include/openxr_inc.h"
#include "openxr/include/util.h"
#include "xr_extension_wrapper.h"

// Wrapper for the XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME extension.
class XRExtPerformanceSettingsExtensionWrapper : public XRExtensionWrapper {
public:
	static XRExtPerformanceSettingsExtensionWrapper *get_singleton();

	void on_instance_initialized(const XrInstance instance) override;

	void on_state_ready() override;

	void on_instance_destroyed() override;

	bool on_event_polled(const XrEventDataBuffer &event) override;

	bool set_cpu_level(XrPerfSettingsLevelEXT level);

	XrPerfSettingsLevelEXT get_cpu_level() {
		return cpu_level;
	}

	bool set_gpu_level(XrPerfSettingsLevelEXT level);

	XrPerfSettingsLevelEXT get_gpu_level() {
		return gpu_level;
	}

protected:
	XRExtPerformanceSettingsExtensionWrapper();
	~XRExtPerformanceSettingsExtensionWrapper();

private:
	EXT_PROTO_XRRESULT_FUNC3(xrPerfSettingsSetPerformanceLevelEXT,
			(XrSession), session,
			(XrPerfSettingsDomainEXT), domain,
			(XrPerfSettingsLevelEXT), level);

	bool update_perf_settings_level(XrPerfSettingsDomainEXT domain, XrPerfSettingsLevelEXT level);

	void cleanup();

	XrResult initialize_ext_performance_settings_extension(XrInstance instance);

	static XRExtPerformanceSettingsExtensionWrapper *singleton;
	static const XrPerfSettingsLevelEXT DEFAULT_PERF_SETTINGS_LEVEL = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;

	OpenXRApi *openxr_api = nullptr;
	bool performance_settings_ext = false;
	XrPerfSettingsLevelEXT cpu_level = DEFAULT_PERF_SETTINGS_LEVEL;
	XrPerfSettingsLevelEXT gpu_level = DEFAULT_PERF_SETTINGS_LEVEL;
};

#endif // XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_WRAPPER_H
