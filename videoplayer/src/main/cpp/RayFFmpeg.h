//
// Created by Administrator on 2018/8/7.
//

#ifndef RAYMUSIC_RAYFFMPEG_H
#define RAYMUSIC_RAYFFMPEG_H

#include "RayCallJava.h"
#include "pthread.h"
#include "RayAudio.h"
#include "RayPlayStatus.h"
#include "RayVideo.h"

extern "C"
{
#include <libavutil/time.h>
#include "libavformat/avformat.h"
};

class RayFFmpeg {

public:
    RayCallJava *callJava;
    const char *url;
    pthread_t decodeThread;
    AVFormatContext *avFormatContext;
    RayAudio *rayAudio = NULL;
    RayVideo *rayVideo = NULL;
    RayPlayStatus* playStatus;
    pthread_mutex_t init_mutex;
    bool exit;
    int duration;
    pthread_mutex_t seek_mutex;
    bool startRecord;

public:
    RayFFmpeg(RayPlayStatus* playStatus, RayCallJava *rayCallJava, const char *url);

    ~RayFFmpeg();

    void prepare();

    void decodeByFFmepg();

    void start();

    void pause();

    void resume();

    void stop();

    void release();

    void seek(int64_t seconds);

    void setVolume(int percent);

    void setMute(int mute);

    void setPitch(float pitch);

    void setSpeed(float speed);

    int getSampleRate();

    void startStopRecord(bool start);

    bool cutAudioPlay(int start_time, int end_time, bool showPcm);

    int getAvCodecContext(AVCodecParameters *codecPar, AVCodecContext **avCodecContext);

};


#endif //RAYMUSIC_RAYFFMPEG_H
