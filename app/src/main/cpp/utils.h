//
// Created by wangzy on 2022/4/30.
//

#ifndef MYANDROIDJVMTIDEMO_UTILS_H
#define MYANDROIDJVMTIDEMO_UTILS_H


#include <jni.h>
#include <string>
#include <sstream>
#include <android/log.h>
#include <sys/types.h>
#include <unistd.h>



#define LOG_TAG "jvmti"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


#endif //MYANDROIDJVMTIDEMO_UTILS_H
