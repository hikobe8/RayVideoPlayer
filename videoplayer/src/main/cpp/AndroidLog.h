//
// Created by Administrator on 2018/8/7.
//
#include <android/log.h>

#ifndef RAYMUSIC_ANDNROIDLOG_H
#define RAYMUSIC_ANDNROIDLOG_H

#endif //RAYMUSIC_ANDNROIDLOG_H

#define LOG_DEBUG true


#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"RayMusic_native",FORMAT,##__VA_ARGS__);
#define LOGW(FORMAT,...) __android_log_print(ANDROID_LOG_WARN,"RayMusic_native",FORMAT,##__VA_ARGS__);
#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_DEBUG,"RayMusic_native",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"RayMusic_native",FORMAT,##__VA_ARGS__);
