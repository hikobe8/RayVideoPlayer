//
// Created by Administrator on 2018/9/4.
//

#include "RayBufferQueue.h"

RayBufferQueue::RayBufferQueue(RayPlayStatus *playStatus) {
    this->playStatus = playStatus;
    pthread_mutex_init(&mutexBuffer, NULL);
    pthread_cond_init(&condBuffer, NULL);
}

RayBufferQueue::~RayBufferQueue() {
    playStatus = NULL;
    pthread_mutex_destroy(&mutexBuffer);
    pthread_cond_destroy(&condBuffer);
    if (LOG_DEBUG) {
        LOGI("RayBufferQueue 清除完成.");
    }
}

int RayBufferQueue::putBuffer(SAMPLETYPE *buffer, int size) {
    pthread_mutex_lock(&mutexBuffer);
    RayPcmBean *pcmBean = new RayPcmBean(buffer, size);
    queueBuffer.push_back(pcmBean);
    pthread_cond_signal(&condBuffer);
    pthread_mutex_unlock(&mutexBuffer);
    return 0;
}

int RayBufferQueue::getBuffer(RayPcmBean **pcmBean) {
    pthread_mutex_lock(&mutexBuffer);
    while (playStatus != NULL && !playStatus->exit) {
        if (queueBuffer.size() > 0) {
            *pcmBean = queueBuffer.front();
            queueBuffer.pop_front();
            break;
        } else {
            if (!playStatus->exit) {
                pthread_cond_wait(&condBuffer, &mutexBuffer);
            }
        }
    }
    pthread_mutex_unlock(&mutexBuffer);
    return 0;
}

int RayBufferQueue::clearBuffer() {
    pthread_cond_signal(&condBuffer);
    pthread_mutex_lock(&mutexBuffer);
    while (!queueBuffer.empty()) {
        RayPcmBean *bean = queueBuffer.front();
        queueBuffer.pop_front();
        delete(bean);
    }
    pthread_mutex_unlock(&mutexBuffer);
    return 0;
}

void RayBufferQueue::release() {
    if (LOG_DEBUG) {
        LOGW("开始释放RayBufferQueue资源")
    }
    notifyThread();
    clearBuffer();
    if (LOG_DEBUG) {
        LOGW("RayBufferQueue资源释放成功")
    }
}

int RayBufferQueue::getBufferSize() {
    int size = 0;
    pthread_mutex_lock(&mutexBuffer);
    size = queueBuffer.size();
    pthread_mutex_unlock(&mutexBuffer);
    return size;
}

int RayBufferQueue::notifyThread(){
    pthread_cond_signal(&condBuffer);
    return 0;
}
