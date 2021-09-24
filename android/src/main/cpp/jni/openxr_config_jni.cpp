#include <jni.h>

#include "jni/jni_util.h"
#include "openxr/extensions/xr_fb_color_space_extension_wrapper.h"
#include "openxr/extensions/xr_fb_display_refresh_rate_extension_wrapper.h"
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

JNIEXPORT jdouble JNICALL JNI_METHOD(getRefreshRate)(JNIEnv *, jclass, jobject) {
	return XRFbDisplayRefreshRateExtensionWrapper::get_singleton()->get_refresh_rate();
}

JNIEXPORT void JNICALL JNI_METHOD(setRefreshRate)(JNIEnv *, jclass, jobject, jdouble refresh_rate) {
	XRFbDisplayRefreshRateExtensionWrapper::get_singleton()->set_refresh_rate(refresh_rate);
}

JNIEXPORT jdoubleArray JNICALL JNI_METHOD(getAvailableRefreshRates)(JNIEnv *env, jclass, jobject) {
	godot::Array refresh_rates = XRFbDisplayRefreshRateExtensionWrapper::get_singleton()->get_available_refresh_rates();
	return array_to_jdoubleArray(env, refresh_rates);
}
};