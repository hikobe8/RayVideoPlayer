package com.ray.log;

import android.util.Log;

import com.ray.musicplayer.BuildConfig;

/***
 *  Author : ryu18356@gmail.com
 *  Create at 2018-08-07 18:49
 *  description : 
 */
public class MyLog {

    private static final String TAG = "RayVideo";

    public static void i(String msg) {
        Log.i(TAG, msg);
    }

    public static void w(String msg) {
        Log.w(TAG, msg);
    }

    public static void d(String msg) {
        Log.d(TAG, msg);
    }

    public static void e(String msg) {
        Log.e(TAG, msg);
    }


}
