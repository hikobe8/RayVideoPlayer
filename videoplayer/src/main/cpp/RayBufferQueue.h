//
// Created by Administrator on 2018/9/4.
//

#ifndef RAYMUSIC_RAYBUFFERQUEUE_H
#define RAYMUSIC_RAYBUFFERQUEUE_H

#include "deque"
#include "RayPcmBean.h"
#include "RayPlayStatus.h"
#include "pthread.h"
#include "AndroidLog.h"

class RayBufferQueue {
public:
    std::deque<RayPcmBean *> queueBuffer;
    pthread_cond_t condBuffer;
    pthread_mutex_t mutexBuffer;
    RayPlayStatus *playStatus;

public:
    RayBufferQueue(RayPlayStatus * playStatus);
    ~RayBufferQueue();
    int putBuffer(SAMPLETYPE * buffer, int size);
    int getBuffer(RayPcmBean ** pcmBean);

    int clearBuffer();
    void release();
    int getBufferSize();
    int notifyThread();
};


#endif //RAYMUSIC_RAYBUFFERQUEUE_H
