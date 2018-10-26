package com.ray.listener;

import com.ray.entity.TimeInfo;

/***
 *  Author : ryu18356@gmail.com
 *  Create at 2018-08-14 15:26
 *  description : 
 */
public interface PlayTimeListener {

    void onPlayTimeChanged(TimeInfo timeInfo);

}
