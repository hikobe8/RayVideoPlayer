//
// Created by Administrator on 2018/8/7.
//

#include "RayCallJava.h"

RayCallJava::RayCallJava(JavaVM *javaVM, JNIEnv *env, jobject obj) {
    this->javaVM = javaVM;
    this->jniEnv = env;
    this->jobj = obj;
    this->jobj = env->NewGlobalRef(jobj);

    jclass jclz = jniEnv->GetObjectClass(jobj);
    if (!jclz) {
        if (LOG_DEBUG) {
            LOGE("get jClass error!");
        }
        return;
    }
    jMIDPrepare = jniEnv->GetMethodID(jclz, "onCallPrepared", "()V");
    jMIDLoad = jniEnv->GetMethodID(jclz, "onResourceLoaded", "(Z)V");
    jMIDTime = jniEnv->GetMethodID(jclz, "onPlayTimeChanged", "(II)V");
    jMIDCallError = jniEnv->GetMethodID(jclz, "onErrorCall", "(ILjava/lang/String;)V");
    jMIDCallComplete = jniEnv->GetMethodID(jclz, "onCallComplete", "()V");
    jMIDCallDbValueChanged = jniEnv->GetMethodID(jclz, "onDbValueChanged", "(I)V");
    jMIDEncodePcm2Aac = jniEnv->GetMethodID(jclz, "encodePcm2Aac", "(I[B)V");
    jMIDGetPcmCutInfo = jniEnv->GetMethodID(jclz, "getPcmCutInfo", "([BI)V");
    jMIDGetPcmCutInfoSampleRate = jniEnv->GetMethodID(jclz, "onGetSampleRate", "(I)V");
}

RayCallJava::~RayCallJava() {
}

void RayCallJava::onCallPrepared(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jMIDPrepare);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jniEnv error!");
            }
            return;
        }
        jniEnv->CallVoidMethod(jobj, jMIDPrepare);
        javaVM->DetachCurrentThread();
    }
}

void RayCallJava::onLoad(int type, bool isLoading) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jMIDLoad, isLoading);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jniEnv error!");
            }
            return;
        }
        jniEnv->CallVoidMethod(jobj, jMIDLoad, isLoading);
        javaVM->DetachCurrentThread();
    }
}

void RayCallJava::onTimeChanged(int type, int now_time, int duration) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jMIDTime, now_time, duration);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jniEnv error!");
            }
            return;
        }
        jniEnv->CallVoidMethod(jobj, jMIDTime, now_time, duration);
        javaVM->DetachCurrentThread();
    }
}

void RayCallJava::onCallError(int type, int code, const char *msg) {
    if (type == MAIN_THREAD) {
        jstring msg_ = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jobj, jMIDCallError, code, msg_);
        jniEnv->DeleteLocalRef(msg_);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jniEnv error!");
            }
            return;
        }
        jstring msg_ = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jobj, jMIDCallError, code, msg_);
        jniEnv->DeleteLocalRef(msg_);
        javaVM->DetachCurrentThread();
    }
}

void RayCallJava::onCallComplete(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jMIDCallComplete);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jniEnv error!");
            }
            return;
        }
        jniEnv->CallVoidMethod(jobj, jMIDCallComplete);
        javaVM->DetachCurrentThread();
    }
}

void RayCallJava::onDbValueChanged(int type, int db) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jMIDCallDbValueChanged, db);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jniEnv error!");
            }
            return;
        }
        jniEnv->CallVoidMethod(jobj, jMIDCallDbValueChanged, db);
        javaVM->DetachCurrentThread();
    }
}

void RayCallJava::onCallRecord(int type, int size, void *buffer) {
    if (type == MAIN_THREAD) {
        jbyteArray jBuffer = jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(jBuffer, 0, size, static_cast<const jbyte *>(buffer));
        jniEnv->CallVoidMethod(jobj, jMIDEncodePcm2Aac, size, jBuffer);
        jniEnv->DeleteLocalRef(jBuffer);
    } else if (type == CHILD_THREAD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jniEnv error!");
            }
            return;
        }
        jbyteArray jBuffer = jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(jBuffer, 0, size, static_cast<const jbyte *>(buffer));
        jniEnv->CallVoidMethod(jobj, jMIDEncodePcm2Aac, size, jBuffer);
        jniEnv->DeleteLocalRef(jBuffer);
        javaVM->DetachCurrentThread();
    }
}

void RayCallJava::onGetPcmCutInfo(void *buffer, int size) {
    JNIEnv *jniEnv;
    if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        if (LOG_DEBUG) {
            LOGE("get child thread jniEnv error!");
        }
        return;
    }
    jbyteArray jBuffer = jniEnv->NewByteArray(size);
    jniEnv->SetByteArrayRegion(jBuffer, 0, size, static_cast<const jbyte *>(buffer));
    jniEnv->CallVoidMethod(jobj, jMIDGetPcmCutInfo, jBuffer, size);
    jniEnv->DeleteLocalRef(jBuffer);
    javaVM->DetachCurrentThread();
}

void RayCallJava::onGetPcmCutInfoSampleRate(int sampleRate) {
    JNIEnv *jniEnv;
    if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        if (LOG_DEBUG) {
            LOGE("get child thread jniEnv error!");
        }
        return;
    }
    jniEnv->CallVoidMethod(jobj, jMIDGetPcmCutInfoSampleRate, sampleRate);
    javaVM->DetachCurrentThread();
}
