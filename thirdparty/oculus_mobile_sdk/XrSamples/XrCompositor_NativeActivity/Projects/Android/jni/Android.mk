LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := xrcompositor


LOCAL_CFLAGS += -std=c99 -Werror
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../../3rdParty/khronos/openxr/OpenXR-SDK/include/

LOCAL_SRC_FILES := ../../../Src/XrCompositor_NativeActivity.c

LOCAL_LDLIBS := -lEGL -lGLESv3 -landroid -llog

LOCAL_LDFLAGS := -u ANativeActivity_onCreate

LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_SHARED_LIBRARIES := openxr_loader

include $(BUILD_SHARED_LIBRARY)

$(call import-module,OpenXR/Projects/AndroidPrebuilt/jni)
$(call import-module,android/native_app_glue)
