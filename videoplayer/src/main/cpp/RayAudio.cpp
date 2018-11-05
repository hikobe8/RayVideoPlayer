//
// Created by Administrator on 2018/8/8.
//

#include "RayAudio.h"

RayAudio::RayAudio(RayCallJava *callJava, RayPlayStatus *playStatus, int sample_rate) {
    this->callJava = callJava;
    this->playStatus = playStatus;
    this->sample_rate = sample_rate;
    this->packetQueue = new RayQueue(playStatus);
    buffer = (uint8_t *) (av_malloc(sample_rate * 2 * 2));

    sampleBuffer = static_cast<SAMPLETYPE *>(malloc(sample_rate * 2 * 2));
    soundTouch = new SoundTouch();
    soundTouch->setSampleRate(sample_rate);
    soundTouch->setChannels(2);
    soundTouch->setPitch(1.0f);
    soundTouch->setTempo(1.5f);
    this->startRecord = false;
    this->isCut = false;
    this->end_time = 0;
    this->showPcm = false;
    pthread_mutex_init(&codecMutex, NULL);
}

RayAudio::~RayAudio() {
    pthread_mutex_destroy(&codecMutex);
}

void *resampleRunnable(void *data) {
    //!!!fatal error :   RayAudio *audio = (RayAudio *) (&data);
    RayAudio *audio = (RayAudio *) (data);
    audio->initOpenSLES();
//    pthread_exit(&audio->play_thread);
    return 0;
}

void *pcmBufferRunnable(void *data) {
    RayAudio *rayAudio = static_cast<RayAudio *>(data);
    while (rayAudio->playStatus != NULL && !rayAudio->playStatus->exit) {
        RayPcmBean * pcmBean = NULL;
        rayAudio->rayBufferQueue->getBuffer(&pcmBean);
        if (pcmBean == NULL) {
            continue;
        }
        if (pcmBean->buffer_size <= rayAudio->default_buffer_size) {
            //不用分包
            if (rayAudio->callJava != NULL) {
                if (rayAudio->startRecord) {
                    rayAudio->callJava->onCallRecord(CHILD_THREAD, pcmBean->buffer_size, pcmBean->buffer);
                }
                if (rayAudio->showPcm) {
                    rayAudio->callJava->onGetPcmCutInfo(pcmBean->buffer, pcmBean->buffer_size);
                }
            }
        } else {
            int pack_num = pcmBean->buffer_size / rayAudio->default_buffer_size;
            int pack_sub = pcmBean->buffer_size % rayAudio->default_buffer_size;
            for (int i = 0; i < pack_num; ++i) {
                char* tmpBuffer = static_cast<char *>(malloc(rayAudio->default_buffer_size));
                memcpy(tmpBuffer, pcmBean->buffer + i * rayAudio->default_buffer_size, rayAudio->default_buffer_size);
                if (rayAudio->callJava != NULL) {
                if (rayAudio->startRecord) {
                    rayAudio->callJava->onCallRecord(CHILD_THREAD, rayAudio->default_buffer_size, tmpBuffer);
                }
                if (rayAudio->showPcm) {
                        rayAudio->callJava->onGetPcmCutInfo(tmpBuffer, rayAudio->default_buffer_size);
                    }
                }
            }
            if (pack_sub > 0) {
                char* tmpBuffer = static_cast<char *>(malloc(pack_sub));
                memcpy(tmpBuffer, pcmBean->buffer + pack_num * rayAudio->default_buffer_size, pack_sub);
                if (rayAudio->callJava != NULL) {
                    if (rayAudio->startRecord) {
                        rayAudio->callJava->onCallRecord(CHILD_THREAD, rayAudio->default_buffer_size, tmpBuffer);
                    }
                    if (rayAudio->showPcm) {
                        rayAudio->callJava->onGetPcmCutInfo(tmpBuffer, rayAudio->default_buffer_size);
                    }
                }
            }
        }
        delete(pcmBean);
        pcmBean = NULL;
    }
    pthread_exit(&rayAudio->pcmBufferThread);
}

void RayAudio::play() {
    if (playStatus != NULL && !playStatus->exit) {
        rayBufferQueue = new RayBufferQueue(playStatus);
        pthread_create(&play_thread, NULL, resampleRunnable, this);
        pthread_create(&pcmBufferThread, NULL, pcmBufferRunnable, this);
    }
}

int RayAudio::resampleAudio(void **pcmBuff) {
    data_size = 0;
    while (playStatus != NULL && !playStatus->exit) {
        if (packetQueue->getQueueSize() == 0) {
            //正在加载
            if (!playStatus->isLoading) {
                playStatus->isLoading = true;
                callJava->onLoad(CHILD_THREAD, true);
            }
            av_usleep(1000 * 100);
            continue;
        } else {
            //加载完成
            if (playStatus->isLoading) {
                playStatus->isLoading = false;
                callJava->onLoad(CHILD_THREAD, false);
            }
        }
        avPacket = av_packet_alloc();
        if (packetQueue->getPacket(avPacket) != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_usleep(1000 * 100);
            continue;
        }
        pthread_mutex_lock(&codecMutex);
        ret = avcodec_send_packet(avCodecContext, avPacket);
        if (ret != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_usleep(1000 * 100);
            pthread_mutex_unlock(&codecMutex);
            continue;
        }
        avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        if (ret == 0) {
            if (avFrame->channels > 0 && avFrame->channel_layout == 0) {
                avFrame->channel_layout = av_get_default_channel_layout(avFrame->channels);
            } else if (avFrame->channels == 0 && avFrame->channel_layout > 0) {
                avFrame->channels = av_get_channel_layout_nb_channels(avFrame->channel_layout);
            }

            SwrContext *swr_ctx;
            swr_ctx = swr_alloc_set_opts(NULL,
                                         AV_CH_LAYOUT_STEREO,
                                         AV_SAMPLE_FMT_S16,
                                         avFrame->sample_rate,
                                         avFrame->channel_layout,
                                         (AVSampleFormat) (avFrame->format),
                                         avFrame->sample_rate,
                                         NULL, NULL);

            if (!swr_ctx || swr_init(swr_ctx) < 0) {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = NULL;
                swr_free(&swr_ctx);
                pthread_mutex_unlock(&codecMutex);
                continue;
            }
            nb = swr_convert(swr_ctx,
                             &buffer,
                             avFrame->nb_samples,
                             (const uint8_t **) (avFrame->data),
                             avFrame->nb_samples
            );

            int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            data_size = nb * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

            now_time = avFrame->pts * av_q2d(time_base);
            if (now_time < clock) {
                now_time = clock;
            }
            clock = now_time;
            *pcmBuff = buffer;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            swr_free(&swr_ctx);
            pthread_mutex_unlock(&codecMutex);
            break;
        } else {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            pthread_mutex_unlock(&codecMutex);
            continue;
        }
    }
    return data_size;
}

void pcmBufferCallback(SLAndroidSimpleBufferQueueItf caller,
                       void *pContext) {
    RayAudio *rayAudio = (RayAudio *) (pContext);
    if (rayAudio != NULL) {
        int dataSize = rayAudio->getSoundTouchData();
        if (dataSize > 0) {
            rayAudio->clock += dataSize / (double) (rayAudio->sample_rate * 2 * 2);
            if (rayAudio->callJava != NULL) {
                if (rayAudio->clock - rayAudio->lastTime > 0.1) {
                    rayAudio->lastTime = rayAudio->clock;
                    rayAudio->callJava->onTimeChanged(CHILD_THREAD, rayAudio->clock,
                                                      rayAudio->duration);
                }
                rayAudio->callJava->onDbValueChanged(CHILD_THREAD, rayAudio->getPcmDB(
                        (char *) (rayAudio->sampleBuffer), dataSize * 4));
            }
            rayAudio->rayBufferQueue->putBuffer(rayAudio->sampleBuffer, dataSize*4);
            (*caller)->Enqueue(caller, (char *) rayAudio->sampleBuffer, dataSize * 2 * 2);
//            if (rayAudio->isCut) {
//                if (rayAudio->clock > rayAudio->end_time) {
//                    LOGI("裁剪结束");
//                    rayAudio->playStatus->exit = true;
//                }
//            }
        }
    }
}

void RayAudio::initOpenSLES() {
    //1.创建引擎对象
    slCreateEngine(&slEngineObjectItf, 0, 0, 0, 0, 0);
    (*slEngineObjectItf)->Realize(slEngineObjectItf, SL_BOOLEAN_FALSE);
    (*slEngineObjectItf)->GetInterface(slEngineObjectItf, SL_IID_ENGINE, &slEngineItf);

    const SLInterfaceID pInterfaceIds[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean pInterfaceRequireds[1] = {SL_BOOLEAN_FALSE};
    //2.创建混音器
    (*slEngineItf)->CreateOutputMix(slEngineItf, &outputObjectItf, 1, pInterfaceIds,
                                    pInterfaceRequireds);
    (*outputObjectItf)->Realize(outputObjectItf, SL_BOOLEAN_FALSE);
    (*outputObjectItf)->GetInterface(outputObjectItf, SL_IID_ENVIRONMENTALREVERB,
                                     &outputMixEnvironmentalReverb);

    (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb,
                                                                      &reverbSettings);
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputObjectItf};
    //3.创建播放器
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//数据格式
            2,//声道格式
            (SLuint32) (getSampleRateForOpenSLES(sample_rate)), //采样率
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSource dataSource = {&android_queue, &pcm};
    SLDataSink dataSink = {&outputMix, NULL};
    const SLInterfaceID ids[4] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_PLAYBACKRATE,
                                  SL_IID_MUTESOLO};
    const SLboolean reqs[4] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    (*slEngineItf)->CreateAudioPlayer(slEngineItf, &pcmPlayerObject, &dataSource, &dataSink, 4, ids,
                                      reqs);
    //初始化播放器
    (*pcmPlayerObject)->Realize(pcmPlayerObject, SL_BOOLEAN_FALSE);
    //得到接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_PLAY, &pcmPlayerPlay);

    //4创建缓冲区和回调函数
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_BUFFERQUEUE, &pcmBufferQueue);
    //缓冲接口回调
    (*pcmBufferQueue)->RegisterCallback(pcmBufferQueue, pcmBufferCallback, this);
    //获取音量接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_VOLUME, &pcmVolumePlay);
    setVolume(volumePercent);
    //获取声道接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_MUTESOLO, &pcmMutePlay);
    setMute(mute);
    setPitch(pitch);
    setSpeed(speed);
    //设置播放状态
    (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);

    pcmBufferCallback(pcmBufferQueue, this);
}

int RayAudio::getSampleRateForOpenSLES(int sample_rate) {
    int rate = 0;
    switch (sample_rate) {
        case 8000000:
            rate = SL_SAMPLINGRATE_8;
            break;
        case 11025000:
            rate = SL_SAMPLINGRATE_11_025;
            break;
        case 16000000:
            rate = SL_SAMPLINGRATE_12;
            break;
        case 22050000:
            rate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000000:
            rate = SL_SAMPLINGRATE_24;
            break;
        case 32000000:
            rate = SL_SAMPLINGRATE_32;
            break;
        case 44100000:
            rate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000000:
            rate = SL_SAMPLINGRATE_48;
            break;
        case 64000000:
            rate = SL_SAMPLINGRATE_64;
            break;
        case 88200000:
            rate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000000:
            rate = SL_SAMPLINGRATE_96;
            break;
        case 192000000:
            rate = SL_SAMPLINGRATE_192;
            break;
        default:
            rate = SL_SAMPLINGRATE_44_1;
            break;
    }
    return rate;
}

void RayAudio::pause() {
    if (pcmPlayerPlay != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PAUSED);
    }
}

void RayAudio::resume() {
    if (pcmPlayerPlay != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    }
}

void RayAudio::stop() {
    if (pcmPlayerPlay != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_STOPPED);
    }
}

void RayAudio::release() {
    stop();

    if (packetQueue != NULL) {
        packetQueue->noticeQueue();
    }
    pthread_join(play_thread, NULL);

    //删除AVPacket 队列 会调用RayQueue的析构函数
    if (packetQueue != NULL) {
        delete (packetQueue);
        packetQueue = NULL;
    }
    if (rayBufferQueue != NULL) {
        rayBufferQueue->notifyThread();
        pthread_join(pcmBufferThread, NULL);
        rayBufferQueue->release();
        delete(rayBufferQueue);
        rayBufferQueue = NULL;
    }

    if (pcmPlayerObject != NULL) {
        (*pcmPlayerObject)->Destroy(pcmPlayerObject);
        pcmPlayerObject = NULL;
        pcmPlayerPlay = NULL;
        pcmBufferQueue = NULL;
    }

    if (outputObjectItf != NULL) {
        (*outputObjectItf)->Destroy(outputObjectItf);
        outputObjectItf = NULL;
        outputMixEnvironmentalReverb = NULL;
    }

    if (slEngineObjectItf != NULL) {
        (*slEngineObjectItf)->Destroy(slEngineObjectItf);
        slEngineObjectItf = NULL;
        slEngineItf = NULL;
    }

    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }

    if (out_buffer != NULL) {
        out_buffer = NULL;
    }

    if (soundTouch != NULL) {
        delete (soundTouch);
        soundTouch = NULL;
    }

    if (sampleBuffer != NULL) {
        free(sampleBuffer);
        sampleBuffer = NULL;
    }

    if (avCodecContext != NULL) {
        pthread_mutex_lock(&codecMutex);
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = NULL;
        pthread_mutex_unlock(&codecMutex);
    }

    if (playStatus != NULL) {
        playStatus = NULL;
    }

    if (callJava != NULL) {
        callJava = NULL;
    }

}

void RayAudio::setVolume(int percent) {
    volumePercent = percent;
    if (pcmVolumePlay != NULL) {
        if (percent > 30) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -20);
        } else if (percent > 25) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -22);
        } else if (percent > 20) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -25);
        } else if (percent > 15) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -28);
        } else if (percent > 10) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -30);
        } else if (percent > 5) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -34);
        } else if (percent > 3) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -37);
        } else if (percent > 0) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -40);
        } else {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -100);
        }
    }
}

void RayAudio::setMute(int mute) {
    this->mute = mute;
    if (pcmMutePlay != NULL) {
        switch (mute) {
            case 0:
                //左声道
                (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, SL_BOOLEAN_FALSE);
                (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, SL_BOOLEAN_TRUE);
                break;
            case 1:
                //右声道
                (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, SL_BOOLEAN_TRUE);
                (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, SL_BOOLEAN_FALSE);
                break;
            default:
                //立体声
                (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, SL_BOOLEAN_FALSE);
                (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, SL_BOOLEAN_FALSE);
                break;
        }
    }
}

int RayAudio::getSoundTouchData() {
    while (playStatus != NULL && !playStatus->exit) {
        out_buffer = NULL;
        if (finished) {
            finished = false;
            data_size = resampleAudio(reinterpret_cast<void **>(&out_buffer));
            if (data_size > 0) {
                for (int i = 0; i < data_size / 2 + 1; i++) {
                    sampleBuffer[i] = (out_buffer[i * 2] | ((out_buffer[i * 2 + 1]) << 8));
                }
                soundTouch->putSamples(sampleBuffer, nb);
                num = soundTouch->receiveSamples(sampleBuffer, data_size / 4);
            } else {
                soundTouch->flush();
            }

        }
        if (num == 0) {
            finished = true;
            continue;
        } else {
            if (out_buffer == NULL) {
                num = soundTouch->receiveSamples(sampleBuffer, data_size / 4);
                if (num == 0) {
                    finished = true;
                    continue;
                }
            }
            return num;
        }
    }
    return 0;
}

void RayAudio::setPitch(float pitch) {
    this->pitch = pitch;
    if (soundTouch != NULL) {
        soundTouch->setPitch(pitch);
    }
}

void RayAudio::setSpeed(float speed) {
    this->speed = speed;
    if (soundTouch != NULL) {
        soundTouch->setTempo(speed);
    }
}

int RayAudio::getPcmDB(char *pcmData, size_t pcmSize) {
    int db = 0;
    short int perValue = 0;
    double sum = 0;
    for (int i = 0; i < pcmSize; i += 2) {
        memcpy(&perValue, pcmData + i, 2);
        sum += abs(perValue);
    }
    sum = sum / (pcmSize / 2);
    if (sum > 0) {
        db = (int) 20.0 * log10(sum);
    }
    return db;
}

void RayAudio::startStopRecord(bool start) {
    this->startRecord = start;
}


