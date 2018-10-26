//
// Created by Administrator on 2018/9/4.
//

#include "RayPcmBean.h"

RayPcmBean::RayPcmBean(SAMPLETYPE *buffer, int buffer_size) {
    this->buffer = static_cast<char *>(malloc(buffer_size));
    this->buffer_size = buffer_size;
    memcpy(this->buffer, buffer, buffer_size);
}

RayPcmBean::~RayPcmBean() {
    free(this->buffer);
    this->buffer = NULL;
}
