//
// Created by Administrator on 2018/10/26.
//

#include "RayVideo.h"

RayVideo::RayVideo(RayPlayStatus *status, RayCallJava *callJava) {

    this->playStatus = status;
    this->rayCallJava = callJava;
    queue = new RayQueue(playStatus);
    pthread_mutex_init(&codecMutex, NULL);
}

RayVideo::~RayVideo() {
    pthread_mutex_destroy(&codecMutex);
}

void *playVideo(void *data) {

    RayVideo *video = static_cast<RayVideo *>(data);
    while (video->playStatus != NULL && !video->playStatus->exit) {
        if (video->playStatus->doSeek) {
            av_usleep(1000 * 100);
            continue;
        }

        if (video->playStatus->pause) {
            av_usleep(1000 * 100);
            continue;
        }

        if (video->queue->getQueueSize() == 0) {
            if (!video->playStatus->isLoading) {
                video->playStatus->isLoading = true;
                video->rayCallJava->onLoad(CHILD_THREAD, true);
            }
            av_usleep(1000 * 100);
            continue;
        } else {
            if (video->playStatus->isLoading) {
                video->playStatus->isLoading = false;
                video->rayCallJava->onLoad(CHILD_THREAD, false);
            }
        }
        AVPacket *avPacket = av_packet_alloc();
        if (video->queue->getPacket(avPacket) != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }
        if (video->codecType == CODEC_MEDIACODEC) {
            if (av_bsf_send_packet(video->abs_ctx, avPacket) != 0) {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                continue;
            }

            while (av_bsf_receive_packet(video->abs_ctx, avPacket) == 0) {
                LOGI("开始解码");

                double diff = video->getFrameDiffTime(NULL, avPacket);

                av_usleep(video->getDelayTime(diff) * 1000000);
                video->rayCallJava->onCallDecodeAVPacket(avPacket->size, avPacket->data);

                av_packet_free(&avPacket);
                av_free(avPacket);
                continue;
            }
            avPacket = NULL;
        } else if (video->codecType == CODEC_YUV) {
            pthread_mutex_lock(&video->codecMutex);
            if (avcodec_send_packet(video->avCodecContext, avPacket) != 0) {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                pthread_mutex_unlock(&video->codecMutex);
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
                pthread_mutex_unlock(&video->codecMutex);
                continue;
            }
            LOGI("子线程解码一个AVFrame成功")
            if (avFrame->format == AV_PIX_FMT_YUV420P) {
                LOGI("当前视频格式为YUV420P格式")

                double diff = video->getFrameDiffTime(avFrame, NULL);

                av_usleep(static_cast<unsigned int>(video->getDelayTime(diff) * 1000000));

                if (video->rayCallJava != NULL) {
                    video->rayCallJava->onCallRenderYUV(
//                        video->avCodecContext->width,
                            avFrame->linesize[0],
                            video->avCodecContext->height,
                            avFrame->data[0],
                            avFrame->data[1],
                            avFrame->data[2]);
                }
            } else {
                AVFrame *pFrameYUV420P = av_frame_alloc();
                int num = av_image_get_buffer_size(
                        AV_PIX_FMT_YUV420P,
                        video->avCodecContext->width,
                        video->avCodecContext->height, 1);
                uint8_t *buffer = static_cast<uint8_t *>(av_malloc(num * sizeof(uint8_t)));
                av_image_fill_arrays(
                        pFrameYUV420P->data,
                        pFrameYUV420P->linesize,
                        buffer,
                        AV_PIX_FMT_YUV420P,
                        video->avCodecContext->width,
                        video->avCodecContext->height, 1);
                SwsContext *swsContext = sws_getContext(
                        video->avCodecContext->width,
                        video->avCodecContext->height,
                        video->avCodecContext->pix_fmt,
                        video->avCodecContext->width,
                        video->avCodecContext->height,
                        AV_PIX_FMT_YUV420P,
                        SWS_BICUBIC, NULL, NULL, NULL);
                if (!swsContext) {
                    av_frame_free(&pFrameYUV420P);
                    av_free(pFrameYUV420P);
                    av_free(buffer);
                    pthread_mutex_unlock(&video->codecMutex);
                    continue;
                }
                sws_scale(
                        swsContext,
                        reinterpret_cast<const uint8_t *const *>(avFrame->data),
                        avFrame->linesize,
                        0,
                        avFrame->height,
                        pFrameYUV420P->data,
                        pFrameYUV420P->linesize
                );
                //渲染

                double diff = video->getFrameDiffTime(avFrame, NULL);

                av_usleep(static_cast<unsigned int>(video->getDelayTime(diff) * 1000000));

                if (video->rayCallJava != NULL) {
                    video->rayCallJava->onCallRenderYUV(
                            video->avCodecContext->width,
                            video->avCodecContext->height,
                            pFrameYUV420P->data[0],
                            pFrameYUV420P->data[1],
                            pFrameYUV420P->data[2]);
                }
                av_frame_free(&pFrameYUV420P);
                av_free(pFrameYUV420P);
                av_free(buffer);
                sws_freeContext(swsContext);
            }
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&video->codecMutex);
        }
    }
    pthread_exit(&video->play_thread);

}

void RayVideo::play() {
    pthread_create(&play_thread, NULL, playVideo, this);
}

void RayVideo::release() {
    if (queue != NULL) {
        delete (queue);
        queue = NULL;
    }

    if (abs_ctx != NULL) {
        av_bsf_free(&abs_ctx);
        abs_ctx = NULL;
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
    if (rayCallJava != NULL) {
        rayCallJava = NULL;
    }
}

double RayVideo::getFrameDiffTime(AVFrame *avFrame, AVPacket *avPacket) {
    double pts = 0;
    if (avFrame != NULL) {
        pts = av_frame_get_best_effort_timestamp(avFrame);
    }

    if (avPacket != NULL) {
        pts = avPacket->pts;
    }

    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }
    pts = pts * av_q2d(time_base);
    if (pts > 0) {
        clock = pts;
    }
    double diff = audio->clock - clock;
    return diff;
}

double RayVideo::getDelayTime(double diff) {

    if (diff > 0.003) {
        delayTime = delayTime * 2.0 / 3;
        if (delayTime < defaultDelayTime/2) {
            delayTime = defaultDelayTime * 2.0 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff < -0.003) {
        delayTime = delayTime * 3.0 / 2;
        if (delayTime < defaultDelayTime/2) {
            delayTime = defaultDelayTime * 2.0 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff == 0.003){

    }

    if (diff >= 0.5) {
        delayTime = 0;
    } else if (diff <= -0.5) {
        delayTime = defaultDelayTime * 2;
    }

    if (fabs(diff) > 10) {
        delayTime = defaultDelayTime;
    }

    return delayTime;
}
