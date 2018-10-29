//
// Created by Administrator on 2018/10/26.
//

#ifndef RAYVIDEOPLAYER_RAYVIDEO_H
#define RAYVIDEOPLAYER_RAYVIDEO_H

#include "RayQueue.h"
#include "RayCallJava.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include <libavutil/time.h>
};

class RayVideo {

public:
    int streamIndex = -1;
    AVRational time_base;
    AVCodecContext *avCodecContext = NULL;
    AVCodecParameters *codecPar = NULL;
    RayQueue *queue = NULL;
    RayPlayStatus *playStatus = NULL;
    pthread_t play_thread;
    RayCallJava *rayCallJava = NULL;

public:
    RayVideo(RayPlayStatus *status, RayCallJava *callJava);

    ~RayVideo();

    void play();
    void release();
};


#endif //RAYVIDEOPLAYER_RAYVIDEO_H
