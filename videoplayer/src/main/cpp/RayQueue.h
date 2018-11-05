//
// Created by Administrator on 2018/8/8.
//

#ifndef RAYMUSIC_RAYQUEUE_H
#define RAYMUSIC_RAYQUEUE_H

#include "queue"
#include "pthread.h"
#include "AndroidLog.h"
#include "RayPlayStatus.h"

extern "C"{
#include "libavcodec/avcodec.h"
};

class RayQueue {

public:
    std::queue<AVPacket*> queuePacket;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;
    RayPlayStatus* playStatus;

public:
    RayQueue(RayPlayStatus* playStatus);
    ~RayQueue();
    void putPacket(AVPacket * packet);
    int getPacket(AVPacket * packet);

    long getQueueSize();
    void clearAVPacket();

    void noticeQueue();
};


#endif //RAYMUSIC_RAYQUEUE_H
