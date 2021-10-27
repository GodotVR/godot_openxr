LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../../cflags.mk

LOCAL_MODULE := samplexrframework

# full speed arm instead of thumb
LOCAL_ARM_MODE := arm
# compile with neon support enabled
LOCAL_ARM_NEON := true


LOCAL_CFLAGS += -Wno-invalid-offsetof
LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/../../../../SampleCommon/Src \
  $(LOCAL_PATH)/../../../Src \
  $(LOCAL_PATH)/../../../../1stParty/OVR/Include \
  $(LOCAL_PATH)/../../../../1stParty/utilities/include \
  $(LOCAL_PATH)/../../../../3rdParty/stb/src \
  $(LOCAL_PATH)/../../../../3rdParty/khronos/openxr/OpenXR-SDK/include \
  $(LOCAL_PATH)/../../../../OpenXR/Include \

LOCAL_STATIC_LIBRARIES += minizip stb android_native_app_glue samplecommon
LOCAL_SRC_FILES := \
  ../../../Src/XrApp.cpp \
  ../../../Src/Input/HandMaskRenderer.cpp \
  ../../../Src/Input/HandRenderer.cpp \
  ../../../Src/Render/Framebuffer.cpp \

# start building based on everything since CLEAR_VARS
include $(BUILD_STATIC_LIBRARY)

$(call import-module,3rdParty/minizip/build/android/jni)
$(call import-module,3rdParty/stb/build/android/jni)
$(call import-module,android/native_app_glue)
$(call import-module,SampleCommon/Projects/Android/jni)
