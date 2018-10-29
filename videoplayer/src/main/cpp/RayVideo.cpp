//
// Created by Administrator on 2018/10/26.
//

#include "RayVideo.h"

RayVideo::RayVideo(RayPlayStatus *status, RayCallJava *callJava) {

    this->playStatus = status;
    this->rayCallJava = callJava;
    queue = new RayQueue(playStatus);

}

RayVideo::~RayVideo() {

}

void * playVideo(void *data)
{

    RayVideo *video = static_cast<RayVideo *>(data);
    while (video->playStatus != NULL && !video->playStatus->exit)
    {
        if (video->playStatus->doSeek) {
            av_usleep(1000*100);
            continue;
        }
        if (video->queue->getQueueSize() == 0) {
            if (!video->playStatus->isLoading) {
                video->playStatus->isLoading = true;
                video->rayCallJava->onLoad(CHILD_THREAD, true);
            }
            av_usleep(1000*100);
            continue;
        } else {
            if (video->playStatus->isLoading) {
                video->playStatus->isLoading = false;
                video->rayCallJava->onLoad(CHILD_THREAD, false);
            }
        }
        AVPacket *avPacket = av_packet_alloc();
        if (video->queue->getPacket(avPacket) != 0)
        {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }
        if (avcodec_send_packet(video->avCodecContext, avPacket) != 0)
        {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }
        AVFrame *avFrame = av_frame_alloc();
        if (avcodec_receive_frame(video->avCodecContext, avFrame) != 0) {
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }
        LOGI("子线程解码一个AVFrame成功")
    }
    pthread_exit(&video->play_thread);

}

void RayVideo::play() {
    pthread_create(&play_thread, NULL, playVideo, this);
}

void RayVideo::release() {
    if (queue != NULL) {
        delete(queue);
        queue = NULL;
    }
    if (avCodecContext != NULL) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = NULL;
    }
    if (playStatus != NULL) {
        playStatus = NULL;
    }
    if (rayCallJava != NULL) {
        rayCallJava = NULL;
    }
}
