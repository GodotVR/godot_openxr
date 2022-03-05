#include "xr_ext_performance_settings_extension_wrapper.h"

XRExtPerformanceSettingsExtensionWrapper *XRExtPerformanceSettingsExtensionWrapper::singleton = nullptr;

XRExtPerformanceSettingsExtensionWrapper *XRExtPerformanceSettingsExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRExtPerformanceSettingsExtensionWrapper();
	}

	return singleton;
}

XRExtPerformanceSettingsExtensionWrapper::XRExtPerformanceSettingsExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	request_extensions[XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME] = &performance_settings_ext;
}

XRExtPerformanceSettingsExtensionWrapper::~XRExtPerformanceSettingsExtensionWrapper() {
	cleanup();
	OpenXRApi::openxr_release_api();
}

void XRExtPerformanceSettingsExtensionWrapper::cleanup() {
	performance_settings_ext = false;
	cpu_level = DEFAULT_PERF_SETTINGS_LEVEL;
	gpu_level = DEFAULT_PERF_SETTINGS_LEVEL;
}

XrResult XRExtPerformanceSettingsExtensionWrapper::initialize_ext_performance_settings_extension(
		XrInstance instance) {
	std::map<const char *, PFN_xrVoidFunction *> func_pointer_map;
	LOAD_FUNC_POINTER_IN_MAP(func_pointer_map, xrPerfSettingsSetPerformanceLevelEXT);

	return initialize_function_pointer_map(instance, func_pointer_map);
}

void XRExtPerformanceSettingsExtensionWrapper::on_instance_initialized(const XrInstance instance) {
	if (performance_settings_ext) {
		XrResult result = initialize_ext_performance_settings_extension(instance);
		if (!openxr_api->xr_result(result, "Failed to initialize performance settings extension")) {
			performance_settings_ext = false;
		}
	}
}

void XRExtPerformanceSettingsExtensionWrapper::on_state_ready() {
	if (performance_settings_ext) {
		// Apply the cpu and gpu level.
		set_cpu_level(this->cpu_level);
		set_gpu_level(this->gpu_level);
	}
}

bool XRExtPerformanceSettingsExtensionWrapper::on_event_polled(const XrEventDataBuffer &event) {
	switch (event.type) {
		case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT: {
			const XrEventDataPerfSettingsEXT *perf_settings_event = (XrEventDataPerfSettingsEXT *)&event;
			// TODO: Might want to provide callback methods to properly notify interested parties.
#ifdef DEBUG
			Godot::print("Received XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT event: type {0} subdomain {1} : level {2} -> level {3}",
					perf_settings_event->type,
					perf_settings_event->subDomain,
					perf_settings_event->fromLevel,
					perf_settings_event->toLevel);
#endif
			return true;
		} break;

		default: {
			return false;
		} break;
	}
}

void XRExtPerformanceSettingsExtensionWrapper::on_instance_destroyed() {
	cleanup();
}

bool XRExtPerformanceSettingsExtensionWrapper::update_perf_settings_level(
		XrPerfSettingsDomainEXT domain, XrPerfSettingsLevelEXT level) {
	if (!performance_settings_ext) {
		return false;
	}

	XrResult result = xrPerfSettingsSetPerformanceLevelEXT(openxr_api->get_session(), domain, level);
	if (!openxr_api->xr_result(result, "Failed to update performance domain {0} to performance level {1}", domain, level)) {
		return false;
	}
	return true;
}

bool XRExtPerformanceSettingsExtensionWrapper::set_cpu_level(XrPerfSettingsLevelEXT level) {
	if (update_perf_settings_level(XR_PERF_SETTINGS_DOMAIN_CPU_EXT, level)) {
		this->cpu_level = level;
		return true;
	}
	return false;
}

bool XRExtPerformanceSettingsExtensionWrapper::set_gpu_level(XrPerfSettingsLevelEXT level) {
	if (update_perf_settings_level(XR_PERF_SETTINGS_DOMAIN_GPU_EXT, level)) {
		this->gpu_level = level;
		return true;
	}
	return false;
}
