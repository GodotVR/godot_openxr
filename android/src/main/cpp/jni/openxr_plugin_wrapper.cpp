#include "openxr_plugin_wrapper.h"

#include "android_util.h"
#include <core/GodotGlobal.hpp>

jobject OpenXRPluginWrapper::openxr_plugin_instance = nullptr;
jmethodID OpenXRPluginWrapper::on_headset_mounted_id = nullptr;
jmethodID OpenXRPluginWrapper::on_headset_unmounted_id = nullptr;
jmethodID OpenXRPluginWrapper::on_focus_gained_id = nullptr;
jmethodID OpenXRPluginWrapper::on_focus_lost_id = nullptr;
jmethodID OpenXRPluginWrapper::on_session_begun_id = nullptr;
jmethodID OpenXRPluginWrapper::on_session_ending_id = nullptr;

OpenXRPluginWrapper::OpenXRPluginWrapper() {}

OpenXRPluginWrapper::~OpenXRPluginWrapper() {}

void OpenXRPluginWrapper::initialize_wrapper(JNIEnv *env, jobject openxr_plugin) {
	openxr_plugin_instance = env->NewGlobalRef(openxr_plugin);
	ALOG_ASSERT(openxr_plugin_instance, "Invalid jobject value.");

	jclass openxr_plugin_class = env->GetObjectClass(openxr_plugin_instance);
	ALOG_ASSERT(openxr_plugin_class, "Invalid jclass value.");

	on_headset_mounted_id = env->GetMethodID(openxr_plugin_class, "onHeadsetMounted", "()V");
	ALOG_ASSERT(on_headset_mounted_id, "Unable to find onHeadsetMounted");

	on_headset_unmounted_id = env->GetMethodID(openxr_plugin_class, "onHeadsetUnmounted", "()V");
	ALOG_ASSERT(on_headset_unmounted_id, "Unable to find onHeadsetUnmounted");

	on_focus_gained_id = env->GetMethodID(openxr_plugin_class, "onFocusGained", "()V");
	ALOG_ASSERT(on_focus_gained_id, "Unable to find onFocusGained");

	on_focus_lost_id = env->GetMethodID(openxr_plugin_class, "onFocusLost", "()V");
	ALOG_ASSERT(on_focus_lost_id, "Unable to find onFocusLost");

	on_session_begun_id = env->GetMethodID(openxr_plugin_class, "onSessionBegun", "()V");
	ALOG_ASSERT(on_session_begun_id, "Unable to find onSessionBegun");

	on_session_ending_id = env->GetMethodID(openxr_plugin_class, "onSessionEnding", "()V");
	ALOG_ASSERT(on_session_ending_id, "Unable to find onSessionEnding");
}

void OpenXRPluginWrapper::uninitialize_wrapper(JNIEnv *env) {
	if (openxr_plugin_instance) {
		env->DeleteGlobalRef(openxr_plugin_instance);

		openxr_plugin_instance = nullptr;
		on_headset_mounted_id = nullptr;
		on_headset_unmounted_id = nullptr;
		on_focus_gained_id = nullptr;
		on_focus_lost_id = nullptr;
		on_session_begun_id = nullptr;
		on_session_ending_id = nullptr;
	}
}

void OpenXRPluginWrapper::on_headset_mounted() {
	if (openxr_plugin_instance && on_headset_mounted_id) {
		JNIEnv *env = godot::android_api->godot_android_get_env();
		env->CallVoidMethod(openxr_plugin_instance, on_headset_mounted_id);
	}
}

void OpenXRPluginWrapper::on_headset_unmounted() {
	if (openxr_plugin_instance && on_headset_unmounted_id) {
		JNIEnv *env = godot::android_api->godot_android_get_env();
		env->CallVoidMethod(openxr_plugin_instance, on_headset_unmounted_id);
	}
}

void OpenXRPluginWrapper::on_focus_gained() {
	if (openxr_plugin_instance && on_focus_gained_id) {
		JNIEnv *env = godot::android_api->godot_android_get_env();
		env->CallVoidMethod(openxr_plugin_instance, on_focus_gained_id);
	}
}

void OpenXRPluginWrapper::on_focus_lost() {
	if (openxr_plugin_instance && on_focus_lost_id) {
		JNIEnv *env = godot::android_api->godot_android_get_env();
		env->CallVoidMethod(openxr_plugin_instance, on_focus_lost_id);
	}
}

void OpenXRPluginWrapper::on_session_begun() {
	if (openxr_plugin_instance && on_session_begun_id) {
		JNIEnv *env = godot::android_api->godot_android_get_env();
		env->CallVoidMethod(openxr_plugin_instance, on_session_begun_id);
	}
}

void OpenXRPluginWrapper::on_session_ending() {
	if (openxr_plugin_instance && on_session_ending_id) {
		JNIEnv *env = godot::android_api->godot_android_get_env();
		env->CallVoidMethod(openxr_plugin_instance, on_session_ending_id);
	}
}