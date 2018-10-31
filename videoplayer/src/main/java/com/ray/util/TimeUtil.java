package com.ray.util;

import com.ray.log.MyLog;

import java.text.SimpleDateFormat;
import java.util.Locale;

/***
 *  Author : ryu18356@gmail.com
 *  Create at 2018-08-14 16:18
 *  description : 时间转换工具类
 */
public class TimeUtil {

    /**
     * 获取hh:ss格式的时间
     * @param seconds 秒数
     * @return hh:ss
     */
    public static String getMMssTime(long seconds) {
        MyLog.e(seconds + " s");
        //get seconds
        int sec = (int) (seconds % 60);
        int min = (int) (seconds/60);
        return formatTimeString(min) + ":" + formatTimeString(sec);
    }

    public static String formatTimeString(int time){
        if (time > 9) {
            return String.valueOf(time);
        } else {
            return "0" + time;
        }
    }

}
