package com.ray.rayvideoplayer;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

import com.ray.TimeUtil;
import com.ray.entity.TimeInfo;
import com.ray.listener.DbChangeListener;
import com.ray.listener.OnCompleteListener;
import com.ray.listener.OnErrorListener;
import com.ray.listener.OnLoadListener;
import com.ray.listener.OnPauseResumeListener;
import com.ray.listener.OnRecordTimeChangeListener;
import com.ray.listener.PlayTimeListener;
import com.ray.listener.PlayerPrepareListener;
import com.ray.listener.YUVDataListener;
import com.ray.log.MyLog;
import com.ray.opengl.RayGLSurfaceView;
import com.ray.player.RayPlayer;
import com.ray.type.ChannelType;

import java.io.File;
import java.lang.ref.WeakReference;

public class MainActivity extends AppCompatActivity {


    private static final boolean TEST_NET_SWITCH = true;
    private static final int KEY_UPDATE_PLAY_TIME = 1;

    private static final String TEST_FILE = "output.mp3";
    private RayPlayer mPlayer;
    private TextView mTvTime;
    private TextView mTvRecordTime;
    private SeekBar mSeekBar;
    private SeekBar mSeekBarVol;
    private MHandler mMHandler;
    private int mPlayPosition;
    private boolean doSeek;
    private RayGLSurfaceView mRayGLSurfaceView;

    static class MHandler extends Handler {
        WeakReference<MainActivity> mContextWeakReference;

        MHandler(MainActivity context) {
            mContextWeakReference = new WeakReference<>(context);
        }

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            MainActivity mainActivity = mContextWeakReference.get();
            if (mainActivity != null && msg.what == KEY_UPDATE_PLAY_TIME) {
                if (!mainActivity.doSeek) {
                    TimeInfo timeInfo = (TimeInfo) msg.obj;
                    mainActivity.mSeekBar.setProgress((int) (timeInfo.nowTime * 1f / timeInfo.duration * 100 + 0.5f));
                    String nowTime = TimeUtil.getMMssTime(timeInfo.nowTime);
                    String duration = TimeUtil.getMMssTime(timeInfo.duration);
                    mainActivity.mTvTime.setText(mainActivity.getString(R.string.play_time, nowTime, duration));
                }
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mRayGLSurfaceView = findViewById(R.id.gl_surface);
        mTvTime = findViewById(R.id.tv_time);
        mSeekBar = findViewById(R.id.seek_bar);
        mSeekBarVol = findViewById(R.id.seek_bar_volume);
        mTvRecordTime = findViewById(R.id.tv_record_time);
        mMHandler = new MHandler(this);
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (mPlayer.getDuration() > 0 && fromUser) {
                    mPlayPosition = (int) (progress / 100f * mPlayer.getDuration() + 0.5f);
                    String nowTime = TimeUtil.getMMssTime(mPlayPosition);
                    String duration = TimeUtil.getMMssTime(mPlayer.getDuration());
                    mTvTime.setText(getString(R.string.play_time, nowTime, duration));
                }

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                doSeek = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                doSeek = false;
                mPlayer.seek(mPlayPosition);
            }
        });
        mSeekBarVol.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                mPlayer.setVolume(progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
    }

    public void start(View view) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            int permission = checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE);
            if (permission == PackageManager.PERMISSION_GRANTED) {
                prepare();
            } else {
                requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
            }
        } else {
            prepare();
        }

    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        prepare();
    }

    private void prepare() {
        mPlayer = new RayPlayer();
        mSeekBarVol.setProgress(mPlayer.getVolume());
        mPlayer.setPlayerPrepareListener(new PlayerPrepareListener() {
            @Override
            public void onPrepared() {
                MyLog.d("准备完成 on Thread " + (Looper.myLooper() == Looper.getMainLooper() ? "main" : "child"));
                mPlayer.start();
            }
        });
        mPlayer.setOnLoadListener(new OnLoadListener() {
            @Override
            public void onLoad(boolean isLoading) {
                MyLog.d(isLoading ? " 加载中 " : " 播放中 ");
            }
        });
        mPlayer.setOnPauseResumeListener(new OnPauseResumeListener() {
            @Override
            public void onPause() {
                MyLog.d("暂停中...");
            }

            @Override
            public void onResume() {
                MyLog.d("播放中...");
            }

        });
        mPlayer.setPlayTimeListener(new PlayTimeListener() {
            @Override
            public void onPlayTimeChanged(TimeInfo timeInfo) {
                mMHandler.obtainMessage(KEY_UPDATE_PLAY_TIME, timeInfo).sendToTarget();
            }
        });
        mPlayer.setOnErrorListener(new OnErrorListener() {
            @Override
            public void onError(int code, String msg) {
                MyLog.e("error ! code : " + code + ", msg : " + msg);
            }
        });
        mPlayer.setOnCompleteListener(new OnCompleteListener() {
            @Override
            public void onComplete() {
                MyLog.w("播放完成");
                mMHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        mSeekBar.setProgress(0);
                        mTvTime.setText("");
                    }
                });
            }
        });
        mPlayer.setDbChangeListener(new DbChangeListener() {
            @Override
            public void onDbChanged(int db) {
                MyLog.w("db value = " + db);
            }
        });
        if (TEST_NET_SWITCH) {
//            mPlayer.setSource("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
            mPlayer.setSource("http://vfx.mtime.cn/Video/2018/10/26/mp4/181026140242572417.mp4");
//            mPlayer.setSource("http://live.hkstv.hk.lxdns.com/live/hks/playlist.m3u8");
        } else {
            mPlayer.setSource(Environment.getExternalStorageDirectory() + File.separator + TEST_FILE);
        }
        mPlayer.setOnRecordTimeChangeListener(new OnRecordTimeChangeListener() {
            @Override
            public void onRecordTimeChanged(final int seconds) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mTvRecordTime.setText(TimeUtil.getMMssTime(seconds));
                    }
                });
            }
        });
        mPlayer.setYUVDataListener(new YUVDataListener() {
            @Override
            public void onGetYUVData(int width, int height, byte[] y, byte[] u, byte[] v) {
                mRayGLSurfaceView.setYUVData(width, height, y, u, v);
            }
        });
        mPlayer.prepare();
    }

    public void pause(View view) {
        mPlayer.pause();
    }

    public void resume(View view) {
        mPlayer.resume();
    }

    public void stop(View view) {
        mPlayer.stop();
    }

    public void next(View view) {
        mPlayer.playNext("http://138.128.206.175:8080/test.ape");
//        mPlayer.playNext("http://live.hkstv.hk.lxdns.com/live/hks/playlist.m3u8");
    }

    public void leftClick(View view) {
        mPlayer.setChannelType(ChannelType.LEFT);
    }

    public void rightClick(View view) {
        mPlayer.setChannelType(ChannelType.RIGHT);
    }

    public void allClick(View view) {
        mPlayer.setChannelType(ChannelType.ALL);
    }

    public void speed(View view) {
        mPlayer.setSpeed(1.5f);
        mPlayer.setPitch(1.0f);
    }

    public void pitch(View view) {
        mPlayer.setSpeed(1.0f);
        mPlayer.setPitch(1.5f);
    }

    public void speedPitch(View view) {
        mPlayer.setSpeed(1.5f);
        mPlayer.setPitch(1.5f);
    }

    public void normalSpeedPitch(View view) {
        mPlayer.setSpeed(1.0f);
        mPlayer.setPitch(1.0f);
    }

    public void record(View view) {
        if (mPlayer != null)
            mPlayer.startRecord(new File(Environment.getExternalStorageDirectory() + File.separator + "test.aac"));
    }

    public void stopRecord(View view) {
        if (mPlayer != null)
            mPlayer.stopRecord();
    }

    public void resumeRecord(View view) {
        if (mPlayer != null) {
            mPlayer.resumeRecord();
        }
    }

    public void pauseRecord(View view) {
        if (mPlayer != null) {
            mPlayer.pauseRecord();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mMHandler.removeMessages(KEY_UPDATE_PLAY_TIME);
    }
}
