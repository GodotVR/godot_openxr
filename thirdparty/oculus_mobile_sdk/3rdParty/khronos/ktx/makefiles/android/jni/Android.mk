LOCAL_PATH := $(call my-dir)/../../..

include $(CLEAR_VARS)

LOCAL_MODULE := ktx
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

include $(LOCAL_PATH)/../../../cflags.mk
LOCAL_CFLAGS   := -w -DLIBKTX -DKTX_FEATURE_WRITE -D_HAS_EXCEPTIONS=0 -D_CRT_SECURE_NO_WARNINGS=1 -D_MBCS -DKHRONOS_STATIC
LOCAL_CPPFLAGS := -w -DLIBKTX -DKTX_FEATURE_WRITE -D_HAS_EXCEPTIONS=0 -D_CRT_SECURE_NO_WARNINGS=1 -D_MBCS -DKHRONOS_STATIC

LOCAL_SRC_FILES := \
  ${wildcard src/*.c} \
  ${wildcard src/*.cpp} \
  ${wildcard src/*.cxx} \
  ${wildcard src/basisu/*.c} \
  ${wildcard src/basisu/*.cpp} \
  ${wildcard src/basisu/transcoder/*.cpp} \
  ${wildcard src/dfdutils/*.c}

LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/src \
  $(LOCAL_PATH)/src/dfdutils \
  $(LOCAL_PATH)/other_include \
  $(LOCAL_PATH)/other_include/GL \
  $(LOCAL_PATH)/other_include/KHR

LOCAL_EXPORT_C_INCLUDES := \
  $(LOCAL_PATH)/include \
  $(LOCAL_PATH)/src \
  $(LOCAL_PATH)/src/basisu \
  $(LOCAL_PATH)/src/dfdutils


include $(BUILD_STATIC_LIBRARY)
