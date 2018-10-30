package com.ray.opengl;

import android.content.Context;
import android.opengl.GLSurfaceView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import static android.opengl.GLES20.GL_COLOR_BUFFER_BIT;
import static android.opengl.GLES20.GL_FLOAT;
import static android.opengl.GLES20.GL_LINEAR;
import static android.opengl.GLES20.GL_LUMINANCE;
import static android.opengl.GLES20.GL_REPEAT;
import static android.opengl.GLES20.GL_TEXTURE0;
import static android.opengl.GLES20.GL_TEXTURE1;
import static android.opengl.GLES20.GL_TEXTURE2;
import static android.opengl.GLES20.GL_TEXTURE_2D;
import static android.opengl.GLES20.GL_TEXTURE_MAG_FILTER;
import static android.opengl.GLES20.GL_TEXTURE_MIN_FILTER;
import static android.opengl.GLES20.GL_TEXTURE_WRAP_S;
import static android.opengl.GLES20.GL_TEXTURE_WRAP_T;
import static android.opengl.GLES20.GL_TRIANGLE_STRIP;
import static android.opengl.GLES20.GL_UNSIGNED_BYTE;
import static android.opengl.GLES20.glActiveTexture;
import static android.opengl.GLES20.glBindTexture;
import static android.opengl.GLES20.glClear;
import static android.opengl.GLES20.glClearColor;
import static android.opengl.GLES20.glDisableVertexAttribArray;
import static android.opengl.GLES20.glDrawArrays;
import static android.opengl.GLES20.glEnableVertexAttribArray;
import static android.opengl.GLES20.glGenTextures;
import static android.opengl.GLES20.glGetAttribLocation;
import static android.opengl.GLES20.glGetUniformLocation;
import static android.opengl.GLES20.glTexImage2D;
import static android.opengl.GLES20.glTexParameteri;
import static android.opengl.GLES20.glUniform1i;
import static android.opengl.GLES20.glUseProgram;
import static android.opengl.GLES20.glVertexAttribPointer;
import static android.opengl.GLES20.glViewport;

/***
 *  Author : ryu18356@gmail.com
 *  Create at 2018-10-30 14:59
 *  description : 视频渲染使用的renderer
 */
public class VideoRenderer implements GLSurfaceView.Renderer {

    private Context mContext;

    private static final float[] VERTEX_DATA = {
            -1f, -1f,
            1f, -1f,
            -1f, 1f,
            1f, 1f
    };

    private static final float[] TEXTURE_DATA = {
            0f, 1f,
            1f, 1f,
            0f, 0f,
            1f, 0f
    };

    private FloatBuffer mVertexBuffer;
    private FloatBuffer mTextureBuffer;
    private int mProgramYUV;
    private int mAvPositionYUV;
    private int mAfPositionYUV;
    private int mSamplerY;
    private int mSamplerU;
    private int mSamplerV;
    private int[] mTextureIdYUV;
    private int mWidthYUV;
    private int mHeightYUV;
    private ByteBuffer mYBuffer;
    private ByteBuffer mUBuffer;
    private ByteBuffer mVBuffer;

    public VideoRenderer(Context context) {
        mContext = context;
        mVertexBuffer = ByteBuffer
                .allocateDirect(VERTEX_DATA.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer()
                .put(VERTEX_DATA);
        mVertexBuffer.position(0);
        mTextureBuffer = ByteBuffer
                .allocateDirect(TEXTURE_DATA.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer()
                .put(TEXTURE_DATA);
        mTextureBuffer.position(0);
    }

    public void setYUVRenderData(int width, int height, byte[] y, byte[] u, byte[] v){
        mWidthYUV = width;
        mHeightYUV = height;
        mYBuffer = ByteBuffer.wrap(y);
        mUBuffer = ByteBuffer.wrap(u);
        mVBuffer = ByteBuffer.wrap(v);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        initRenderYUV();
    }

    private void initRenderYUV() {
        String vertexSource = ShaderUtils.readAssetsShader(mContext, "shader/vertex_shader.glsl");
        String fragmentSource = ShaderUtils.readAssetsShader(mContext, "shader/fragment_shader.glsl");
        mProgramYUV = ShaderUtils.createProgram(vertexSource, fragmentSource);
        mAvPositionYUV = glGetAttribLocation(mProgramYUV, "av_Position");
        mAfPositionYUV = glGetAttribLocation(mProgramYUV, "af_Position");
        mSamplerY = glGetUniformLocation(mProgramYUV, "sampler_y");
        mSamplerU = glGetUniformLocation(mProgramYUV, "sampler_u");
        mSamplerV = glGetUniformLocation(mProgramYUV, "sampler_v");
        mTextureIdYUV = new int[3];
        glGenTextures(3, mTextureIdYUV, 0);
        for (int i = 0; i < 3; i++) {
            glBindTexture(GL_TEXTURE_2D, mTextureIdYUV[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        glViewport(0, 0, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0f, 0f, 0f, 1f);
        renderYUV();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisableVertexAttribArray(mAvPositionYUV);
        glDisableVertexAttribArray(mAfPositionYUV);
    }

    private void renderYUV(){
        if (mWidthYUV > 0 && mHeightYUV > 0 && mYBuffer != null && mUBuffer != null && mVBuffer != null) {
            glUseProgram(mProgramYUV);

            glEnableVertexAttribArray(mAvPositionYUV);
            glVertexAttribPointer(mAvPositionYUV, 2, GL_FLOAT, false, 8, mVertexBuffer);

            glEnableVertexAttribArray(mAfPositionYUV);
            glVertexAttribPointer(mAfPositionYUV, 2, GL_FLOAT, false, 8, mTextureBuffer);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mTextureIdYUV[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mWidthYUV, mHeightYUV, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mYBuffer);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mTextureIdYUV[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mWidthYUV/2, mHeightYUV/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mUBuffer);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, mTextureIdYUV[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mWidthYUV/2, mHeightYUV/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mVBuffer);

            glUniform1i(mSamplerY, 0);
            glUniform1i(mSamplerU, 1);
            glUniform1i(mSamplerV, 2);

            mYBuffer.clear();
            mUBuffer.clear();
            mVBuffer.clear();

            mYBuffer = null;
            mUBuffer = null;
            mVBuffer = null;
        }
    }

}
