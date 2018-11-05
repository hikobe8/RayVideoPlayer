#include <jni.h>
#include <string>

extern "C"
{
#include <libavformat/avformat.h>
}

#include "RayFFmpeg.h"

JavaVM *javaVM = NULL;
RayCallJava *rayCallJava = NULL;
RayFFmpeg *rayFFmpeg = NULL;
RayPlayStatus* playStatus = NULL;
bool nativeStopping = false;
pthread_t startThread;

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reversed) {
    javaVM = vm;
    int result = -1;
    JNIEnv *env;
    if (javaVM->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK) {
        return result;
    }
    return JNI_VERSION_1_4;
}

void* startCallback(void* ctx) {
    RayFFmpeg *fFmpeg = (RayFFmpeg *)(ctx);
    fFmpeg->start();
//    pthread_exit(&startThread);
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ray_player_RayPlayer_native_1prepare(JNIEnv *env, jobject instance, jstring source_) {
    const char *source = env->GetStringUTFChars(source_, 0);
    if (rayFFmpeg == NULL) {
        if (rayCallJava == NULL) {
            rayCallJava = new RayCallJava(javaVM, env, instance);
        }
        rayCallJava->onLoad(MAIN_THREAD, true);
        playStatus = new RayPlayStatus();
        rayFFmpeg = new RayFFmpeg(playStatus, rayCallJava, source);
        rayFFmpeg->prepare();
    }

//    env->ReleaseStringUTFChars(source_, source);
}extern "C"
JNIEXPORT void JNICALL
Java_com_ray_player_RayPlayer_native_1start(JNIEnv *env, jobject instance) {

    if (rayFFmpeg != NULL) {
        pthread_create(&startThread, NULL, startCallback, rayFFmpeg);
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_ray_player_RayPlayer_native_1pause(JNIEnv *env, jobject instance) {

    if (rayFFmpeg != NULL) {
        rayFFmpeg->pause();
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_ray_player_RayPlayer_native_1resume(JNIEnv *env, jobject instance) {

    if (rayFFmpeg != NULL) {
        rayFFmpeg->resume();
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_ray_player_RayPlayer_native_1stop(JNIEnv *env, jobject instance) {

    if (nativeStopping) {
        return;
    }

    jclass jclz = env->GetObjectClass(instance);
    jmethodID jmethodIDNext =  env->GetMethodID(jclz, "onCallNext", "()V");

    nativeStopping = true;
    if (rayFFmpeg != NULL) {
        rayFFmpeg->release();

        pthread_join(startThread, NULL);

        delete(rayFFmpeg);
        rayFFmpeg = NULL;

        if (rayCallJava != NULL) {
            delete(rayCallJava);
            rayCallJava = NULL;
        }

        if (playStatus != NULL) {
            delete(playStatus);
            playStatus = NULL;
        }

    }

    nativeStopping = false;
    env->CallVoidMethod(instance, jmethodIDNext);

}extern "C"
JNIEXPORT void JNICALL
Java_com_ray_player_RayPlayer_native_1seek(JNIEnv *env, jobject instance, jint seconds) {

    if (rayFFmpeg != NULL) {
        rayFFmpeg->seek(seconds);
    }

}extern "C"
JNIEXPORT jint JNICALL
Java_com_ray_player_RayPlayer_native_1getDuration(JNIEnv *env, jobject instance) {

    if (rayFFmpeg != NULL) {
        return rayFFmpeg->duration;
    }
    return 0;
}extern "C"
JNIEXPORT void JNICALL
Java_com_ray_player_RayPlayer_native_1setVolume(JNIEnv *env, jobject instance, jint volumePercent) {

    if (rayFFmpeg != NULL) {
        rayFFmpeg->setVolume(volumePercent);
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_ray_player_RayPlayer_native_1setChannelType(JNIEnv *env, jobject instance, jint type) {

    if (rayFFmpeg != NULL) {
        rayFFmpeg->setMute(type);
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_ray_player_RayPlayer_native_1setPitch(JNIEnv *env, jobject instance, jfloat pitch) {

    if (rayFFmpeg != NULL) {
        rayFFmpeg->setPitch(pitch);
    }

}extern "C"
JNIEXPORT void JNICALL
Java_com_ray_player_RayPlayer_native_1setSpeed(JNIEnv *env, jobject instance, jfloat speed) {

    if (rayFFmpeg != NULL) {
        rayFFmpeg->setSpeed(speed);
    }

}extern "C"
JNIEXPORT jint JNICALL
Java_com_ray_player_RayPlayer_native_1getSampleRate(JNIEnv *env, jobject instance) {

    if (rayFFmpeg != NULL) {
        return rayFFmpeg->getSampleRate();
    }

    return 0;
}extern "C"
JNIEXPORT void JNICALL
Java_com_ray_player_RayPlayer_native_1startStopRecord(JNIEnv *env, jobject instance,
                                                      jboolean start) {

    if (rayFFmpeg != NULL) {
        rayFFmpeg->startStopRecord(start);
    }

}extern "C"
JNIEXPORT jboolean JNICALL
Java_com_ray_player_RayPlayer_native_1cutAudioPlay(JNIEnv *env, jobject instance, jint startTime,
                                                   jint endTime, jboolean showPcm) {

    if (rayFFmpeg != NULL) {
        return rayFFmpeg->cutAudioPlay(startTime, endTime, showPcm);
    }
    return false;

}