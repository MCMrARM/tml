APP_ABI := x86
#APP_ABI := armeabi-v7a
APP_PLATFORM := android-14
APP_CFLAGS += -O2
APP_CFLAGS += -DYAML_VERSION_MAJOR=0 -DYAML_VERSION_MINOR=0 -DYAML_VERSION_PATCH=0 -DYAML_VERSION_STRING=\"test\"
APP_CFLAGS += -DKeccakP200_excluded -DKeccakP400_excluded -DKeccakP800_excluded
APP_CPPFLAGS += -std=c++0x -fexceptions

APP_STL := gnustl_static
NDK_TOOLCHAIN_VERSION := 4.9
