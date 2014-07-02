LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := entityx 

LOCAL_MODULE_FILENAME := libentityx

LOCAL_SRC_FILES := \
entityx/Entity.cc  \
entityx/Event.cc  \
entityx/System.cc \
entityx/help/Pool.cc \
entityx/help/Timer.cc \


LOCAL_C_INCLUDES := $(LOCAL_PATH)

include $(BUILD_STATIC_LIBRARY)
