//
// Created by Administrator on 2018/8/7.
//
#include <android/log.h>

#ifndef RAYMUSIC_ANDNROIDLOG_H
#define RAYMUSIC_ANDNROIDLOG_H

#endif //RAYMUSIC_ANDNROIDLOG_H

#define LOG_DEBUG true


#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"RayVideo_native",FORMAT,##__VA_ARGS__);
#define LOGW(FORMAT,...) __android_log_print(ANDROID_LOG_WARN,"RayVideo_native",FORMAT,##__VA_ARGS__);
#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_DEBUG,"RayVideo_native",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"RayVideo_native",FORMAT,##__VA_ARGS__);
