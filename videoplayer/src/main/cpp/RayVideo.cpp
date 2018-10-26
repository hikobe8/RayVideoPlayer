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
        AVPacket *avPacket = av_packet_alloc();
        if (video->queue->getPacket(avPacket) == 0)
        {
            //解码渲染
            LOGE("视频解码渲染!");
        }
        av_packet_free(&avPacket);
        av_free(avPacket);
        avPacket = NULL;
    }
    pthread_exit(&video->play_thread);

}

void RayVideo::play() {
    pthread_create(&play_thread, NULL, playVideo, this);
}
