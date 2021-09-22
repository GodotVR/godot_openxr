#ifndef OPENXR_PLUGIN_WRAPPER_H
#define OPENXR_PLUGIN_WRAPPER_H

#include <jni.h>

class OpenXRPluginWrapper {
public:
	static void initialize_wrapper(JNIEnv *env, jobject openxr_plugin);

	static void uninitialize_wrapper(JNIEnv *env);

	static void on_headset_mounted();

	static void on_headset_unmounted();

	static void on_focus_gained();

	static void on_focus_lost();

	static void on_session_begun();

	static void on_session_ending();

private:
	OpenXRPluginWrapper();
	~OpenXRPluginWrapper();

	static jobject openxr_plugin_instance;
	static jmethodID on_headset_mounted_id;
	static jmethodID on_headset_unmounted_id;
	static jmethodID on_focus_gained_id;
	static jmethodID on_focus_lost_id;
	static jmethodID on_session_begun_id;
	static jmethodID on_session_ending_id;
};

#endif // OPENXR_PLUGIN_WRAPPER_H
