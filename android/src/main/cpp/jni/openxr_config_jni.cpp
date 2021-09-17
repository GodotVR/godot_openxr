#include <jni.h>

#include "jni/jni_util.h"
#include "openxr/extensions/xr_ext_performance_settings_extension_wrapper.h"
#include "openxr/extensions/xr_fb_color_space_extension_wrapper.h"
#include "openxr/extensions/xr_fb_display_refresh_rate_extension_wrapper.h"
#include "openxr/extensions/xr_fb_foveation_extension_wrapper.h"
#include "openxr/extensions/xr_fb_passthrough_extension_wrapper.h"
#include <core/Array.hpp>
#include <core/Dictionary.hpp>

#undef JNI_PACKAGE_NAME
#define JNI_PACKAGE_NAME org_godotengine_plugin_vr_openxr_api

#undef JNI_CLASS_NAME
#define JNI_CLASS_NAME OpenXRConfig

extern "C" {
JNIEXPORT jint JNICALL JNI_METHOD(nativeGetXrColorSpace)(JNIEnv *, jclass) {
	return XRFbColorSpaceExtensionWrapper::get_singleton()->get_color_space();
}

JNIEXPORT void JNICALL JNI_METHOD(nativeSetXrColorSpace)(JNIEnv *, jclass, jint color_space) {
	XRFbColorSpaceExtensionWrapper::get_singleton()->set_color_space(color_space);
}

JNIEXPORT jintArray JNICALL JNI_METHOD(nativeGetAvailableXrColorSpaces)(JNIEnv *env, jclass) {
	godot::Dictionary color_spaces = XRFbColorSpaceExtensionWrapper::get_singleton()->get_available_color_spaces();
	godot::Array color_spaces_values = color_spaces.keys();
	return array_to_jintArray(env, color_spaces_values);
}

JNIEXPORT jdouble JNICALL JNI_METHOD(nativeGetRefreshRate)(JNIEnv *, jclass) {
	return XRFbDisplayRefreshRateExtensionWrapper::get_singleton()->get_refresh_rate();
}

JNIEXPORT void JNICALL JNI_METHOD(nativeSetRefreshRate)(JNIEnv *, jclass, jdouble refresh_rate) {
	XRFbDisplayRefreshRateExtensionWrapper::get_singleton()->set_refresh_rate(refresh_rate);
}

JNIEXPORT jdoubleArray JNICALL JNI_METHOD(nativeGetAvailableRefreshRates)(JNIEnv *env, jclass) {
	godot::Array refresh_rates = XRFbDisplayRefreshRateExtensionWrapper::get_singleton()->get_available_refresh_rates();
	return array_to_jdoubleArray(env, refresh_rates);
}

JNIEXPORT jstring JNICALL JNI_METHOD(nativeGetSystemName)(JNIEnv *env, jclass) {
	OpenXRApi *openxr_api = OpenXRApi::openxr_get_api();
	String system_name = openxr_api->get_system_name();
	jstring j_system_name = string_to_jstring(env, system_name);
	OpenXRApi::openxr_release_api();
	return j_system_name;
}

JNIEXPORT jint JNICALL JNI_METHOD(nativeGetCpuLevel)(JNIEnv *, jclass) {
	return XRExtPerformanceSettingsExtensionWrapper::get_singleton()->get_cpu_level();
}

JNIEXPORT jboolean JNICALL JNI_METHOD(nativeSetCpuLevel)(JNIEnv *, jclass, jint level) {
	return XRExtPerformanceSettingsExtensionWrapper::get_singleton()->set_cpu_level(static_cast<XrPerfSettingsLevelEXT>(level));
}

JNIEXPORT jint JNICALL JNI_METHOD(nativeGetGpuLevel)(JNIEnv *, jclass) {
	return XRExtPerformanceSettingsExtensionWrapper::get_singleton()->get_gpu_level();
}

JNIEXPORT jboolean JNICALL JNI_METHOD(nativeSetGpuLevel)(JNIEnv *, jclass, jint level) {
	return XRExtPerformanceSettingsExtensionWrapper::get_singleton()->set_gpu_level(static_cast<XrPerfSettingsLevelEXT>(level));
}

JNIEXPORT jfloat JNICALL JNI_METHOD(nativeGetRenderTargetSizeMultiplier)(JNIEnv *, jclass) {
	OpenXRApi *openxr_api = OpenXRApi::openxr_get_api();
	jfloat multiplier = openxr_api->get_render_target_size_multiplier();
	OpenXRApi::openxr_release_api();
	return multiplier;
}

JNIEXPORT jboolean JNICALL JNI_METHOD(nativeSetRenderTargetSizeMultiplier)(JNIEnv *, jclass, jfloat multiplier) {
	OpenXRApi *openxr_api = OpenXRApi::openxr_get_api();
	jboolean result = openxr_api->set_render_target_size_multiplier(multiplier);
	OpenXRApi::openxr_release_api();
	return result;
}

JNIEXPORT void JNICALL JNI_METHOD(nativeSetFoveationLevel)(JNIEnv *, jclass, jint foveation_level, jboolean is_dynamic) {
	XrFoveationDynamicFB foveation_dynamic = is_dynamic ? XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB : XR_FOVEATION_DYNAMIC_DISABLED_FB;
	XRFbFoveationExtensionWrapper::get_singleton()->set_foveation_level(static_cast<XrFoveationLevelFB>(foveation_level), foveation_dynamic);
}

JNIEXPORT jboolean JNICALL JNI_METHOD(nativeStartPassthrough)(JNIEnv *, jclass) {
	return XRFbPassthroughExtensionWrapper::get_singleton()->start_passthrough();
}

JNIEXPORT void JNICALL JNI_METHOD(nativeStopPassthrough)(JNIEnv *, jclass) {
	XRFbPassthroughExtensionWrapper::get_singleton()->stop_passthrough();
}
};
