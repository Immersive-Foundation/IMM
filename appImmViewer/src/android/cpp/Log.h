#pragma once

#include <android/log.h>

#define ALOGV_TAG "OvrApp"
#define ALOGF(...) { __android_log_print( ANDROID_LOG_ERROR, ALOGV_TAG, __VA_ARGS__ ); abort(); }
#define ALOGE(...) __android_log_print( ANDROID_LOG_ERROR, ALOGV_TAG, __VA_ARGS__ )
#define ALOGW(...) __android_log_print( ANDROID_LOG_WARN, ALOGV_TAG, __VA_ARGS__ )
#define ALOGV(...) __android_log_print( ANDROID_LOG_VERBOSE, ALOGV_TAG, __VA_ARGS__ )
