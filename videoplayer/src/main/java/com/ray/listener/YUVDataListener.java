package com.ray.listener;

/**
 * Author : hikobe8@github.com
 * Time : 2018/8/22 下午3:14
 * Description :
 */
public interface YUVDataListener {
    void onGetYUVData(int width, int height, byte[] y, byte[] u, byte[] v);
}
