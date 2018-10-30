package com.ray.player;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.text.TextUtils;

import com.ray.entity.TimeInfo;
import com.ray.listener.DbChangeListener;
import com.ray.listener.OnCompleteListener;
import com.ray.listener.OnErrorListener;
import com.ray.listener.OnLoadListener;
import com.ray.listener.OnPauseResumeListener;
import com.ray.listener.OnPcmCutInfoListener;
import com.ray.listener.OnRecordTimeChangeListener;
import com.ray.listener.PlayTimeListener;
import com.ray.listener.PlayerPrepareListener;
import com.ray.listener.YUVDataListener;
import com.ray.log.MyLog;
import com.ray.type.ChannelType;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

/***
 *  Author : ryu18356@gmail.com
 *  Create at 2018-08-07 18:44
 *  description : 
 */
public class RayPlayer {

    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avutil-55");
        System.loadLibrary("swresample-2");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("avformat-57");
        System.loadLibrary("swscale-4");
        System.loadLibrary("postproc-54");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("avdevice-57");
    }

    private String mSource;
    private PlayerPrepareListener mPlayerPrepareListener;
    private OnLoadListener mOnLoadListener;
    private OnPauseResumeListener mOnPauseResumeListener;
    private PlayTimeListener mPlayTimeListener;
    private OnErrorListener mOnErrorListener;
    private OnCompleteListener mOnCompleteListener;
    private DbChangeListener mDbChangeListener;
    private OnRecordTimeChangeListener mOnRecordTimeChangeListener;
    private OnPcmCutInfoListener mOnPcmCutInfoListener;
    private YUVDataListener mYUVDataListener;

    private static TimeInfo sTimeInfo;
    private static boolean sPlayNext;
    private static int sDuration = -1;
    private static int sVolumePercent = 50;
    private boolean mInitMediaCodec;
    private static float sPitch = 1.0f;
    private static float sSpeed = 1.0f;
    private double mRecordTime;
    private int mSampleRate;

    public void setPlayerPrepareListener(PlayerPrepareListener playerPrepareListener) {
        mPlayerPrepareListener = playerPrepareListener;
    }

    public void setOnLoadListener(OnLoadListener onLoadListener) {
        mOnLoadListener = onLoadListener;
    }

    public void setOnPauseResumeListener(OnPauseResumeListener onPauseResumeListener) {
        mOnPauseResumeListener = onPauseResumeListener;
    }

    public void setPlayTimeListener(PlayTimeListener playTimeListener) {
        mPlayTimeListener = playTimeListener;
    }

    public void setOnErrorListener(OnErrorListener onErrorListener) {
        mOnErrorListener = onErrorListener;
    }

    public void setOnCompleteListener(OnCompleteListener onCompleteListener) {
        mOnCompleteListener = onCompleteListener;
    }

    public void setDbChangeListener(DbChangeListener dbChangeListener) {
        mDbChangeListener = dbChangeListener;
    }

    public void setOnRecordTimeChangeListener(OnRecordTimeChangeListener onRecordTimeChangeListener) {
        mOnRecordTimeChangeListener = onRecordTimeChangeListener;
    }

    public void setOnPcmCutInfoListener(OnPcmCutInfoListener onPcmCutInfoListener) {
        mOnPcmCutInfoListener = onPcmCutInfoListener;
    }

    public void setYUVDataListener(YUVDataListener YUVDataListener) {
        mYUVDataListener = YUVDataListener;
    }

    public void setSource(String source) {
        mSource = source;
    }

    public void prepare() {
        if (TextUtils.isEmpty(mSource)) {
            MyLog.e("play source can't be empty!");
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                native_prepare(mSource);
            }
        }).start();
    }

    public void start() {
        if (TextUtils.isEmpty(mSource)) {
            MyLog.e("play source can't be empty!");
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                native_start();
            }
        }).start();
    }

    public void pause() {
        if (mOnPauseResumeListener != null) {
            mOnPauseResumeListener.onPause();
        }
        native_pause();
    }

    public void resume() {
        if (mOnPauseResumeListener != null) {
            mOnPauseResumeListener.onResume();
        }
        native_resume();
    }

    public void stop() {
        sTimeInfo = null;
        new Thread(new Runnable() {
            @Override
            public void run() {
                native_stop();
            }
        }).start();
    }

    public void seek(int seconds) {
        native_seek(seconds);
    }

    public void playNext(String url) {
        sPlayNext = true;
        setSource(url);
        stop();
    }

    public int getDuration() {
        if (sDuration < 0) {
            sDuration = native_getDuration();
        }
        return sDuration;
    }

    public int getVolume() {
        return sVolumePercent;
    }

    public void setVolume(int volumePercent) {
        if (volumePercent >= 0 && volumePercent <= 100) {
            sVolumePercent = volumePercent;
            native_setVolume(volumePercent);
        }
    }

    public void setChannelType(@ChannelType int type) {
        native_setChannelType(type);
    }

    public void setPitch(float pitch) {
        sPitch = pitch;
        native_setPitch(pitch);
    }

    public void setSpeed(float speed) {
        sSpeed = speed;
        native_setSpeed(speed);
    }

    public void startRecord(File outFile) {
        if (mInitMediaCodec)
            return;
        mSampleRate = native_getSampleRate();
        if (mSampleRate > 0) {
            initMediaCodec(outFile, mSampleRate);
        }
    }

    public void stopRecord() {
        mRecordTime = 0;
        native_startStopRecord(false);
        releaseMediaCodec();
    }

    public void pauseRecord() {
        native_startStopRecord(false);
    }

    public void resumeRecord() {
        native_startStopRecord(true);
    }

    private void releaseMediaCodec() {
        if (mEncoder == null)
            return;
        try {
            if (mAACOutputStream != null) {
                mAACOutputStream.close();
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            mAACOutputStream = null;
            mEncoder.stop();
            mEncoder.release();
            mEncoder = null;
            mMediaFormat = null;
            mInfo = null;
            mInitMediaCodec = false;
            MyLog.d("录制完成");
        }
        mAACOutputStream = null;
    }

    public void cutAudioPlay(int startTime, int endTime, boolean showPcm) {
        if (native_cutAudioPlay(startTime, endTime, showPcm)) {
            start();
        } else {
            stop();
            onErrorCall(2001, "cutaudio params is wrong");
        }
    }

    public void onCallPrepared() {
        if (mPlayerPrepareListener != null)
            mPlayerPrepareListener.onPrepared();
    }

    public void onResourceLoaded(boolean isLoading) {
        if (mOnLoadListener != null) {
            mOnLoadListener.onLoad(isLoading);
        }
    }

    public void onPlayTimeChanged(int nowTime, int duration) {
        if (mPlayTimeListener != null) {
            if (sTimeInfo == null) {
                sTimeInfo = new TimeInfo(nowTime, duration);
            }
            sTimeInfo.nowTime = nowTime;
            sTimeInfo.duration = duration;
            mPlayTimeListener.onPlayTimeChanged(sTimeInfo);
        }
    }

    public void onErrorCall(int code, String msg) {
        stop();
        if (mOnErrorListener != null) {
            mOnErrorListener.onError(code, msg);
        }
    }

    public void onCallComplete() {
        stop();
        if (mOnCompleteListener != null) {
            mOnCompleteListener.onComplete();
        }
    }

    public void onCallNext() {
        if (sPlayNext) {
            sPlayNext = false;
            prepare();
        }
    }

    public void onDbValueChanged(int db) {
        if (mDbChangeListener != null) {
            mDbChangeListener.onDbChanged(db);
        }
    }

    public void getPcmCutInfo(byte[] buffer, int size) {
        if (mOnPcmCutInfoListener != null) {
            mOnPcmCutInfoListener.getPcmCutInfo(buffer, size);
        }
    }

    public void onGetSampleRate(int sampleRate) {
        if (mOnPcmCutInfoListener != null) {
            mOnPcmCutInfoListener.onGetSampleRate(sampleRate, 16, 2);
        }
    }

    public void onRenderYUVData(int width, int height, byte[] fy, byte[] fu, byte[] fv) {
        if (mYUVDataListener != null) {
            mYUVDataListener.onGetYUVData(width, height, fy, fu, fv);
        }
    }

    private native void native_prepare(String source);

    private native void native_start();

    private native void native_pause();

    private native void native_resume();

    private native void native_stop();

    private native void native_seek(int seconds);

    private native int native_getDuration();

    private native void native_setVolume(int volumePercent);

    private native void native_setChannelType(int type);

    private native void native_setPitch(float pitch);

    private native void native_setSpeed(float speed);

    private native int native_getSampleRate();

    private native void native_startStopRecord(boolean start);

    private native boolean native_cutAudioPlay(int startTime, int endTime, boolean showPcm);

    //MediaCodec
    private MediaFormat mMediaFormat;
    private MediaCodec mEncoder;
    private FileOutputStream mAACOutputStream;
    private MediaCodec.BufferInfo mInfo;
    private int mPerPcmSize;
    private byte[] mOutByteBuffer;
    private int mAacSampleRate = 4;

    private void initMediaCodec(File outFile, int sampleRate) {
        try {
            mAacSampleRate = getADTSsamplerate(sampleRate);
            mMediaFormat = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, sampleRate, 2);
            mMediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, 96000);
            mMediaFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
// java.lang.IllegalStateException native_dequeueOutputBuffer
// mMediaFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 4096);
            mMediaFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 12000);
            mEncoder = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);
            if (mEncoder == null) {
                MyLog.d("create encoder wrong");
                return;
            }
            mInfo = new MediaCodec.BufferInfo();
            mEncoder.configure(mMediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mAACOutputStream = new FileOutputStream(outFile);
            mEncoder.start();
            native_startStopRecord(true);
            mInitMediaCodec = true;
        } catch (IOException e) {
            MyLog.e("create MediaCodec Encoder wrong!");
            e.printStackTrace();
        }
    }

    private void encodePcm2Aac(int size, byte[] buffer) {
        if (buffer != null && mEncoder != null) {
            mRecordTime += size * 1.0f / (mSampleRate * 2 * 2);
            if (mOnRecordTimeChangeListener != null) {
                mOnRecordTimeChangeListener.onRecordTimeChanged((int) mRecordTime);
            }
            int inputBufferIndex = mEncoder.dequeueInputBuffer(0);
            if (inputBufferIndex >= 0) {
                ByteBuffer byteBuffer = mEncoder.getInputBuffers()[inputBufferIndex];
                if (byteBuffer == null) {
                    MyLog.e("fetch ByteBuffer wrong!");
                    return;
                }
                byteBuffer.clear();
                byteBuffer.put(buffer);
                mEncoder.queueInputBuffer(inputBufferIndex, 0, size, 0, 0);
            }
            int index = mEncoder.dequeueOutputBuffer(mInfo, 0);
            while (index >= 0) {
                try {
                    mPerPcmSize = mInfo.size + 7;
                    mOutByteBuffer = new byte[mPerPcmSize];
                    ByteBuffer byteBuffer = mEncoder.getOutputBuffers()[index];
                    if (byteBuffer == null) {
                        MyLog.e("fetch ByteBuffer wrong!");
                        return;
                    }
                    byteBuffer.position(mInfo.offset);
                    byteBuffer.limit(mInfo.offset + mInfo.size);

                    addADtsHeader(mOutByteBuffer, mPerPcmSize, mAacSampleRate);

                    byteBuffer.get(mOutByteBuffer, 7, mInfo.size);
                    byteBuffer.position(mInfo.offset);
                    mAACOutputStream.write(mOutByteBuffer, 0, mPerPcmSize);

                    mEncoder.releaseOutputBuffer(index, false);
                    index = mEncoder.dequeueOutputBuffer(mInfo, 0);
                    mOutByteBuffer = null;
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

    }

    private void addADtsHeader(byte[] packet, int packetLen, int samplerate) {
        int profile = 2; // AAC LC
        int freqIdx = samplerate; // samplerate
        int chanCfg = 2; // CPE

        packet[0] = (byte) 0xFF; // 0xFFF(12bit) 这里只取了8位，所以还差4位放到下一个里面
        packet[1] = (byte) 0xF9; // 第一个t位放F
        packet[2] = (byte) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
        packet[3] = (byte) (((chanCfg & 3) << 6) + (packetLen >> 11));
        packet[4] = (byte) ((packetLen & 0x7FF) >> 3);
        packet[5] = (byte) (((packetLen & 7) << 5) + 0x1F);
        packet[6] = (byte) 0xFC;
    }

    private int getADTSsamplerate(int samplerate) {
        int rate = 4;
        switch (samplerate) {
            case 96000:
                rate = 0;
                break;
            case 88200:
                rate = 1;
                break;
            case 64000:
                rate = 2;
                break;
            case 48000:
                rate = 3;
                break;
            case 44100:
                rate = 4;
                break;
            case 32000:
                rate = 5;
                break;
            case 24000:
                rate = 6;
                break;
            case 22050:
                rate = 7;
                break;
            case 16000:
                rate = 8;
                break;
            case 12000:
                rate = 9;
                break;
            case 11025:
                rate = 10;
                break;
            case 8000:
                rate = 11;
                break;
            case 7350:
                rate = 12;
                break;
        }
        return rate;
    }
}
