LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := modloader
MODLOADER_SOURCES := $(wildcard $(LOCAL_PATH)/src/*.cpp)
LIBYAML_PATH := $(LOCAL_PATH)/lib/libyaml
LIBYAML_SOURCES := $(wildcard $(LIBYAML_PATH)/src/*.c)
LOCAL_SRC_FILES := $(MODLOADER_SOURCES:$(LOCAL_PATH)/%=%) $(LIBYAML_SOURCES:$(LOCAL_PATH)/%=%) main.cpp lib/linkerutils/src/linkerutils.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/ $(LIBYAML_PATH)/include/ $(LOCAL_PATH)/lib/linkerutils/include
LOCAL_LDLIBS := -ldl -llog

LIBKECCAK_PATH := lib/keccak
ifeq ($(TARGET_ARCH),x86)
    LOCAL_SRC_FILES += $(LIBKECCAK_PATH)/SnP/KeccakP-1600/Inplace32BI/KeccakP-1600-inplace32BI.c
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBKECCAK_PATH)/SnP/KeccakP-1600/Inplace32BI/
else ifeq ($(TARGET_ARCH),armeabi_v7a)
    LOCAL_SRC_FILES += $(LIBKECCAK_PATH)/SnP/KeccakP-1600/Optimized32biAsmARM/KeccakP-1600-inplace-32bi-armv7a-le-gcc.s
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBKECCAK_PATH)/SnP/KeccakP-1600/Optimized32biAsmARM/
endif
LOCAL_SRC_FILES += $(LIBKECCAK_PATH)/Constructions/KeccakSponge.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(LIBKECCAK_PATH)/Common/ $(LOCAL_PATH)/$(LIBKECCAK_PATH)/Constructions/ \
    $(LOCAL_PATH)/$(LIBKECCAK_PATH)/SnP/

include $(BUILD_SHARED_LIBRARY)
