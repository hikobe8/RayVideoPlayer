package com.ray.listener;

/**
 * Author : hikobe8@github.com
 * Time : 2018/9/3 下午11:19
 * Description :
 */
public interface OnPcmCutInfoListener {

    void getPcmCutInfo(byte[] buffer, int bufferSize);

    void onGetSampleRate(int sampleRate, int bit, int channel);
}
