
LOCAL_PATH := $(call my-dir)

#################
#  SDP library  #
#################

include $(CLEAR_VARS)
LOCAL_MODULE := libsdp
LOCAL_DESCRIPTION := Session Description Protocol library
LOCAL_CATEGORY_PATH := libs
LOCAL_SRC_FILES := \
    src/sdp.c \
    src/sdp_base64.c \
    src/sdp_log.c
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_LIBRARIES := libfutils
LOCAL_CONDITIONAL_LIBRARIES := OPTIONAL:libulog

include $(BUILD_LIBRARY)

#####################
#  Test executable  #
#####################

include $(CLEAR_VARS)
LOCAL_MODULE := sdp_test
LOCAL_DESCRIPTION := Session Description Protocol library test program
LOCAL_CATEGORY_PATH := multimedia
LOCAL_SRC_FILES := test/sdp_test.c
LOCAL_LIBRARIES := libsdp libulog
include $(BUILD_EXECUTABLE)
