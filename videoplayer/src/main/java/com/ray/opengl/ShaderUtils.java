package com.ray.opengl;

import android.content.Context;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import static android.opengl.GLES20.GL_COMPILE_STATUS;
import static android.opengl.GLES20.GL_FRAGMENT_SHADER;
import static android.opengl.GLES20.GL_LINK_STATUS;
import static android.opengl.GLES20.GL_TRUE;
import static android.opengl.GLES20.GL_VERTEX_SHADER;
import static android.opengl.GLES20.glAttachShader;
import static android.opengl.GLES20.glCompileShader;
import static android.opengl.GLES20.glCreateProgram;
import static android.opengl.GLES20.glCreateShader;
import static android.opengl.GLES20.glDeleteProgram;
import static android.opengl.GLES20.glDeleteShader;
import static android.opengl.GLES20.glGetProgramiv;
import static android.opengl.GLES20.glGetShaderiv;
import static android.opengl.GLES20.glLinkProgram;
import static android.opengl.GLES20.glShaderSource;

/***
 *  Author : ryu18356@gmail.com
 *  Create at 2018-10-30 13:45
 *  description : OpenGLES shader 工具类，读取，编译等
 */
public class ShaderUtils {

    private static final String TAG = "ShaderUtils";

    public static String readAssetsShader(Context context, String assetsPath){
        try {
            InputStream inputStream = context.getResources().getAssets().open(assetsPath);
            InputStreamReader inputStreamReader = new InputStreamReader(inputStream);
            BufferedReader bufferedReader = new BufferedReader(inputStreamReader);
            StringBuilder stringBuilder = new StringBuilder();
            String line;
            while ((line = bufferedReader.readLine()) != null) {
                stringBuilder.append(line).append("\n");
            }
            bufferedReader.close();
            return stringBuilder.toString();
        } catch (IOException e) {
            e.printStackTrace();
            Log.e(TAG, "read shader : " + assetsPath + " failed!" );
        }
        return null;
    }

    public static int loadShader(int type, String code) {
        int shader = glCreateShader(type);
        if (shader != 0) {
            glShaderSource(shader, code);
            glCompileShader(shader);
            int[] compile = new int[1];
            glGetShaderiv(shader, GL_COMPILE_STATUS, compile, 0);
            if (compile[0] != GL_TRUE) {
                Log.e(TAG, "shader compilation error!");
                glDeleteShader(shader);
                shader = 0;
            }
        }
        return shader;
    }

    public static int createProgram(String vertextCode, String fragmentCode){
        int vertexShader = loadShader(GL_VERTEX_SHADER, vertextCode);
        if (vertexShader == 0) {
            return 0;
        }
        int fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentCode);
        if (fragmentShader == 0) {
            return 0;
        }
        int program = glCreateProgram();
        if (program != 0) {
            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);
            glLinkProgram(program);
            int [] linkStatus = new int[1];
            glGetProgramiv(program, GL_LINK_STATUS, linkStatus, 0);
            if (linkStatus[0] != GL_TRUE) {
                Log.e(TAG, "link program error!");
                glDeleteProgram(program);
                program = 0;
            }
        }
        return program;
    }

}
