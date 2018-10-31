//
// Created by Administrator on 2018/8/7.
//

#include "RayFFmpeg.h"

RayFFmpeg::RayFFmpeg(RayPlayStatus *playStatus, RayCallJava *rayCallJava, const char *url) {
    this->playStatus = playStatus;
    this->callJava = rayCallJava;
    this->url = url;
    this->avFormatContext = NULL;
    pthread_mutex_init(&init_mutex, NULL);
    pthread_mutex_init(&seek_mutex, NULL);
}

RayFFmpeg::~RayFFmpeg() {
    pthread_mutex_destroy(&init_mutex);
    pthread_mutex_destroy(&seek_mutex);
}

void *decodeRunnable(void *data) {
    RayFFmpeg *rayFFmpeg = (RayFFmpeg *) (data);
    rayFFmpeg->decodeByFFmepg();
    pthread_exit(&rayFFmpeg->decodeThread);
}

void RayFFmpeg::prepare() {
    pthread_create(&decodeThread, NULL, decodeRunnable, this);
}

int interrupt_callback(void *ctx) {
    RayFFmpeg *fFmpeg = (RayFFmpeg *) (ctx);
    if (fFmpeg->playStatus->exit) {
        return AVERROR_EOF;
    }
    return 0;
}

void RayFFmpeg::decodeByFFmepg() {
    pthread_mutex_lock(&init_mutex);
    av_register_all();
    avformat_network_init();
    LOGD("network initialized!");
    avFormatContext = avformat_alloc_context();
    avFormatContext->interrupt_callback.callback = interrupt_callback;
    avFormatContext->interrupt_callback.opaque = this;
    if (avformat_open_input(&avFormatContext, url, NULL, NULL) != 0) {
        if (LOG_DEBUG) {
            LOGE("can't open url : %s", url);
        }
        callJava->onCallError(CHILD_THREAD, 1001, "can't open url");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can't find stream from url : %s", url);
        }
        callJava->onCallError(CHILD_THREAD, 1002, "can't find stream from url");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) { //得到音频流
            if (rayAudio == NULL) {
                rayAudio = new RayAudio(callJava, playStatus,
                                        avFormatContext->streams[i]->codecpar->sample_rate);
                rayAudio->streamIndex = i;
                rayAudio->codecPar = avFormatContext->streams[i]->codecpar;
                rayAudio->duration = avFormatContext->duration / AV_TIME_BASE;
                rayAudio->time_base = avFormatContext->streams[i]->time_base;
                this->duration = rayAudio->duration;
                if (callJava != NULL) {
                    callJava->onGetPcmCutInfoSampleRate(
                            avFormatContext->streams[i]->codecpar->sample_rate);
                }
            }
        } else if (avFormatContext->streams[i]->codecpar->codec_type ==
                   AVMEDIA_TYPE_VIDEO) { //得到视频流
            if (rayVideo == NULL) {
                rayVideo = new RayVideo(playStatus, callJava);
                rayVideo->streamIndex = i;
                rayVideo->codecPar = avFormatContext->streams[i]->codecpar;
                rayVideo->time_base = avFormatContext->streams[i]->time_base;

                int num = avFormatContext->streams[i]->avg_frame_rate.num;
                int den = avFormatContext->streams[i]->avg_frame_rate.den;
                if (num != 0 && den != 0) {
                    int fps = num / den;
                    rayVideo->defaultDelayTime = 1.0/fps;
                }
            }
        }
    }
    if (rayAudio == NULL) {
        if (LOG_DEBUG) {
            LOGW("no audio stream founded : %s!", url);
        }
        callJava->onCallError(CHILD_THREAD, 1003, "no audio stream founded");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    getAvCodecContext(rayAudio->codecPar, &rayAudio->avCodecContext);
    if (rayVideo == NULL) {
        if (LOG_DEBUG) {
            LOGW("no video stream founded : %s!", url);
        }
        callJava->onCallError(CHILD_THREAD, 1003, "no video stream founded");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    getAvCodecContext(rayVideo->codecPar, &rayVideo->avCodecContext);
    if (playStatus != NULL) {
        if (callJava != NULL && !playStatus->exit) {
            callJava->onCallPrepared(CHILD_THREAD);
        } else {
            exit = true;
        }
    }
    pthread_mutex_unlock(&init_mutex);
}

void RayFFmpeg::start() {
    if (rayAudio == NULL) {
        callJava->onCallError(CHILD_THREAD, 1003, "no audio stream founded");
        return;
    }
    if (rayVideo == NULL) {
        callJava->onCallError(CHILD_THREAD, 1003, "no video stream founded");
        return;
    }
    rayVideo->audio = rayAudio;

    const char* codecName = ((const AVCodec *)rayVideo->avCodecContext->codec)->name;
    if (callJava->onCallSupportHardwareDecode(codecName)) {
        LOGI("当前设备支持硬解码该视频");
    }

    rayAudio->play();
    rayVideo->play();
    while (playStatus != NULL && !playStatus->exit) {

        if (playStatus->doSeek) {
            av_usleep(1000 * 100);
            continue;
        }

        if (rayAudio->packetQueue->getQueueSize() > 40) {
            av_usleep(1000 * 100);
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();
        pthread_mutex_lock(&seek_mutex);
        int retCode = av_read_frame(avFormatContext, avPacket);
        pthread_mutex_unlock(&seek_mutex);
        if (retCode == 0) {
            if (avPacket->stream_index == rayAudio->streamIndex) {
                rayAudio->packetQueue->putPacket(avPacket);
            } else if (avPacket->stream_index == rayVideo->streamIndex) {
                rayVideo->queue->putPacket(avPacket);
            } else {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
            }
        } else {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            while (playStatus != NULL && !playStatus->exit) {
                if (rayAudio->packetQueue->getQueueSize() > 0) {
                    av_usleep(1000 * 100);
                    continue;
                } else {
                    if (!playStatus->doSeek) {
                        av_usleep(1000 * 500);
                        playStatus->exit = true;
                    }
                    break;
                }
            }
        }
    }
    if (callJava != NULL) {
        callJava->onCallComplete(CHILD_THREAD);
    }
    exit = true;
    if (LOG_DEBUG) {
        LOGD("解码完成");
    }

}

void RayFFmpeg::pause() {

    if (playStatus != NULL) {
        playStatus->pause = true;
    }

    if (rayAudio != NULL) {
        rayAudio->pause();
    }
}

void RayFFmpeg::resume() {

    if (playStatus != NULL) {
        playStatus->pause = false;
    }

    if (rayAudio != NULL) {
        rayAudio->resume();
    }
}

void RayFFmpeg::stop() {
    if (rayAudio != NULL) {
        rayAudio->stop();
    }
}

void RayFFmpeg::release() {
    if (LOG_DEBUG) {
        LOGD("开始释放ffmpeg");
    }
    playStatus->exit = true;
    pthread_mutex_lock(&init_mutex);
    int sleepCount = 0;
    while (!exit) {
        if (sleepCount > 1000) {
            exit = true;
        }
        if (LOG_DEBUG) {
            LOGE("wait ffmpeg  exit %d", sleepCount);
        }
        sleepCount++;
        av_usleep(1000 * 10);//暂停10毫秒
    }


    if (rayAudio != NULL) {
        if (LOG_DEBUG) {
            LOGE("释放 Audio");
        }
        rayAudio->release();
        delete (rayAudio);
        rayAudio = NULL;
    }

    if (rayVideo != NULL) {
        if (LOG_DEBUG) {
            LOGE("释放 video");
        }
        rayVideo->release();
        delete (rayVideo);
        rayVideo = NULL;
    }

    if (LOG_DEBUG) {
        LOGE("释放 封装格式上下文");
    }

    if (avFormatContext != NULL) {
        avformat_close_input(&avFormatContext);
        avformat_free_context(avFormatContext);
        avFormatContext = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放 callJava");
    }
    if (callJava != NULL) {
        callJava = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放 playstatus");
    }
    if (playStatus != NULL) {
        playStatus = NULL;
    }
    pthread_mutex_unlock(&init_mutex);
}

void RayFFmpeg::seek(int64_t seconds) {
    if (duration <= 0) {
        //直播duration为0
        return;
    }
    if (seconds >= 0 && seconds <= duration) {
        playStatus->doSeek = true;
        pthread_mutex_lock(&seek_mutex);
        int64_t real = seconds * AV_TIME_BASE;
        avformat_seek_file(avFormatContext, -1, INT64_MIN, real, INT64_MAX, 0);
        if (rayAudio != NULL) {
            rayAudio->packetQueue->clearAVPacket();
            rayAudio->clock = 0;
            rayAudio->lastTime = 0;
            pthread_mutex_lock(&rayAudio->codecMutex);
            avcodec_flush_buffers(rayAudio->avCodecContext);
            pthread_mutex_unlock(&rayAudio->codecMutex);
        }

        if (rayVideo != NULL) {
            rayVideo->queue->clearAVPacket();
            rayVideo->clock = 0;
            pthread_mutex_lock(&rayVideo->codecMutex);
            avcodec_flush_buffers(rayVideo->avCodecContext);
            pthread_mutex_unlock(&rayVideo->codecMutex);
        }
        pthread_mutex_unlock(&seek_mutex);
        playStatus->doSeek = false;
    }
}

void RayFFmpeg::setVolume(int percent) {
    if (rayAudio != NULL) {
        rayAudio->setVolume(percent);
    }
}

void RayFFmpeg::setMute(int mute) {
    if (rayAudio != NULL) {
        rayAudio->setMute(mute);
    }
}

void RayFFmpeg::setPitch(float pitch) {
    if (rayAudio != NULL) {
        rayAudio->setPitch(pitch);
    }
}

void RayFFmpeg::setSpeed(float speed) {
    if (rayAudio != NULL) {
        rayAudio->setSpeed(speed);
    }
}

int RayFFmpeg::getSampleRate() {
    if (rayAudio != NULL) {
        return rayAudio->avCodecContext->sample_rate;
    }
    return 0;
}

void RayFFmpeg::startStopRecord(bool start) {
    if (rayAudio != NULL) {
        rayAudio->startStopRecord(start);
    }
}

bool RayFFmpeg::cutAudioPlay(int start_time, int end_time, bool showPcm) {
    if (start_time >= 0 && end_time < duration && start_time < end_time) {
        if (rayAudio != NULL) {
            rayAudio->isCut = true;
            rayAudio->end_time = end_time;
            rayAudio->showPcm = showPcm;
            seek(start_time);
            return true;
        }
    }
    return false;
}

int RayFFmpeg::getAvCodecContext(AVCodecParameters *codecPar, AVCodecContext **avCodecContext) {
    AVCodec *avCodec = avcodec_find_decoder(codecPar->codec_id);
    if (!avCodec) {
        if (LOG_DEBUG) {
            LOGE("can't find audio decoder!");
        }
        callJava->onCallError(CHILD_THREAD, 1004, "can't find audio decoder");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    *avCodecContext = avcodec_alloc_context3(avCodec);
    if (*avCodecContext == NULL) {
        if (LOG_DEBUG) {
            LOGE("can't alloc new decoderContext!");
        }
        callJava->onCallError(CHILD_THREAD, 1005, "can't alloc new decoderContext");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    if (avcodec_parameters_to_context(*avCodecContext, codecPar) < 0) {
        if (LOG_DEBUG) {
            LOGE("can't fill decoderContext!");
        }
        callJava->onCallError(CHILD_THREAD, 1006, "can't fill decoderContext");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    if (avcodec_open2(*avCodecContext, avCodec, 0) != 0) {
        if (LOG_DEBUG) {
            LOGE("can't open audio streams: %s!", url);
        }
        callJava->onCallError(CHILD_THREAD, 1007, "can't open audio streams");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    return 0;
}
