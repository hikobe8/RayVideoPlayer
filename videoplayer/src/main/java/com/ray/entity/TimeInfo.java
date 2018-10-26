package com.ray.entity;

/***
 *  Author : ryu18356@gmail.com
 *  Create at 2018-08-14 15:24
 *  description : 
 */
public class TimeInfo {

    public int nowTime;
    public int duration;

    public TimeInfo(int nowTime, int duration) {
        this.nowTime = nowTime;
        this.duration = duration;
    }

    @Override
    public String toString() {
        return "TimeInfo{" +
                "nowTime=" + nowTime +
                ", duration=" + duration +
                '}';
    }
}
