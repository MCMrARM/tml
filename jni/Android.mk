LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := modloader
MODLOADER_SOURCES := $(wildcard $(LOCAL_PATH)/src/*.cpp)
LOCAL_SRC_FILES := $(MODLOADER_SOURCES:$(LOCAL_PATH)/%=%)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/
LOCAL_LDLIBS := -ldl -llog

include $(BUILD_SHARED_LIBRARY)
