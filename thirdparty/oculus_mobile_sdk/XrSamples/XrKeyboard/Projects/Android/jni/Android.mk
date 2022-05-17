LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := xrkeyboard

include ../../../../cflags.mk

LOCAL_C_INCLUDES := \
                    $(LOCAL_PATH)/../../../../../SampleCommon/Src \
                    $(LOCAL_PATH)/../../../../../SampleXrFramework/Src \
  					$(LOCAL_PATH)/../../../../../1stParty/OVR/Include \
  					$(LOCAL_PATH)/../../../../../1stParty/utilities/include \
  					$(LOCAL_PATH)/../../../../../3rdParty/stb/src \
  					$(LOCAL_PATH)/../../../../../3rdParty/khronos/openxr/OpenXR-SDK/include \

LOCAL_SRC_FILES		:= 	../../../Src/main.cpp \
                        ../../../Src/Input/KeyboardRenderer.cpp \

# include default libraries
LOCAL_LDLIBS 			:= -llog -landroid -lGLESv3 -lEGL
LOCAL_STATIC_LIBRARIES 	:= samplexrframework
LOCAL_SHARED_LIBRARIES := openxr_loader

include $(BUILD_SHARED_LIBRARY)

$(call import-module,SampleXrFramework/Projects/Android/jni)
$(call import-module,OpenXR/Projects/AndroidPrebuilt/jni)
