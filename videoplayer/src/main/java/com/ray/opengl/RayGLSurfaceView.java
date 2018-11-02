package com.ray.opengl;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

/***
 *  Author : ryu18356@gmail.com
 *  Create at 2018-10-30 16:08
 *  description : 
 */
public class RayGLSurfaceView extends GLSurfaceView {

    private VideoRenderer mVideoRenderer;

    public RayGLSurfaceView(Context context) {
        this(context, null);
    }

    public RayGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        mVideoRenderer = new VideoRenderer(context);
        setRenderer(mVideoRenderer);
        setRenderMode(RENDERMODE_WHEN_DIRTY);
        mVideoRenderer.setOnRenderListener(new VideoRenderer.OnRenderListener() {
            @Override
            public void onRender() {
                requestRender();
            }
        });
    }

    public void setYUVData(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (mVideoRenderer != null) {
            mVideoRenderer.setYUVRenderData(width, height, y, u, v);
            requestRender();
        }
    }

    public VideoRenderer getVideoRenderer() {
        return mVideoRenderer;
    }
}
