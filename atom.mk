
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libsdp
LOCAL_CATEGORY_PATH := libs
LOCAL_DESCRIPTION := Session Description Protocol library
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_CFLAGS := -DSDP_API_EXPORTS -fvisibility=hidden -std=gnu99
LOCAL_SRC_FILES := \
	src/sdp.c \
	src/sdp_base64.c
LOCAL_LIBRARIES := \
	libfutils \
	libulog
include $(BUILD_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := sdp-test
LOCAL_CATEGORY_PATH := multimedia
LOCAL_DESCRIPTION := Session Description Protocol library test program
LOCAL_SRC_FILES := \
	tools/sdp_test.c
LOCAL_LIBRARIES := \
	libsdp \
	libulog
include $(BUILD_EXECUTABLE)


ifdef TARGET_TEST

include $(CLEAR_VARS)

LOCAL_MODULE := tst-libsdp
LOCAL_CFLAGS += -DTARGET_TEST -D_GNU_SOURCE
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src
LOCAL_SRC_FILES := \
	tests/sdp_test_base64.c \
	tests/sdp_test.c
LOCAL_LIBRARIES := \
	libcunit \
	libsdp

include $(BUILD_EXECUTABLE)

endif
