//
// Created by Administrator on 2018/10/26.
//

#ifndef RAYVIDEOPLAYER_RAYVIDEO_H
#define RAYVIDEOPLAYER_RAYVIDEO_H

#define CODEC_YUV 0
#define CODEC_MEDIACODEC 1

#include "RayQueue.h"
#include "RayCallJava.h"
#include "RayAudio.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
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
    RayAudio *audio = NULL;
    double clock = 0;
    double delayTime = 0;
    double defaultDelayTime = 0;
    pthread_mutex_t codecMutex;
    int codecType = CODEC_YUV;
    AVBSFContext *abs_ctx;

public:
    RayVideo(RayPlayStatus *status, RayCallJava *callJava);

    ~RayVideo();

    void play();
    void release();
    double getFrameDiffTime(AVFrame *avFrame);
    double getDelayTime(double diff);
};


#endif //RAYVIDEOPLAYER_RAYVIDEO_H
