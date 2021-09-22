#include <jni.h>

#include "jni/jni_util.h"
#include "jni/openxr_plugin_wrapper.h"

#undef JNI_PACKAGE_NAME
#define JNI_PACKAGE_NAME org_godotengine_plugin_vr_openxr

#undef JNI_CLASS_NAME
#define JNI_CLASS_NAME OpenXRPlugin

extern "C" {
JNIEXPORT void JNICALL JNI_METHOD(initializeWrapper)(JNIEnv *env, jobject object) {
	OpenXRPluginWrapper::initialize_wrapper(env, object);
}

JNIEXPORT void JNICALL JNI_METHOD(uninitializeWrapper)(JNIEnv *env, jobject) {
	OpenXRPluginWrapper::uninitialize_wrapper(env);
}
}
