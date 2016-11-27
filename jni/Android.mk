LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := modloader
MODLOADER_SOURCES := $(wildcard $(LOCAL_PATH)/src/*.cpp)
LIBYAML_PATH := $(LOCAL_PATH)/lib/libyaml
LIBYAML_SOURCES := $(wildcard $(LIBYAML_PATH)/src/*.c)
LOCAL_SRC_FILES := $(MODLOADER_SOURCES:$(LOCAL_PATH)/%=%) $(LIBYAML_SOURCES:$(LOCAL_PATH)/%=%)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/ $(LIBYAML_PATH)/include/
LOCAL_LDLIBS := -ldl -llog

include $(BUILD_SHARED_LIBRARY)
