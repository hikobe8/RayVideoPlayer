//
// Created by Administrator on 2018/9/4.
//

#ifndef RAYMUSIC_RAYPCMBEAN_H
#define RAYMUSIC_RAYPCMBEAN_H

#include "SoundTouch.h"
using namespace soundtouch;

class RayPcmBean {

public:
    char* buffer;
    int buffer_size;

public:
    RayPcmBean(SAMPLETYPE *buffer, int buffer_size);
    ~RayPcmBean();

};


#endif //RAYMUSIC_RAYPCMBEAN_H
