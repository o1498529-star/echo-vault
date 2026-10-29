package com.echovault.game;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

final class GpuSurface extends GLSurfaceView implements GLSurfaceView.Renderer {
    static final int MAX_BYTES=16000*6*12;
    private final Object lock=new Object();
    private final ByteBuffer frame=ByteBuffer.allocateDirect(MAX_BYTES).order(ByteOrder.nativeOrder());
    private int program,position,vertexColor,width,height,vertexCount;

    GpuSurface(Context context) {super(context);setEGLContextClientVersion(2);setRenderer(this);setRenderMode(RENDERMODE_WHEN_DIRTY);}
    void submit(byte[] vertices,int count) {
        synchronized(lock) {frame.position(0);frame.put(vertices,0,count*12);frame.position(0);vertexCount=count;}
        requestRender();
    }
    private static int shader(int type,String source) {
        int id=GLES20.glCreateShader(type);GLES20.glShaderSource(id,source);GLES20.glCompileShader(id);
        int[] ok=new int[1];GLES20.glGetShaderiv(id,GLES20.GL_COMPILE_STATUS,ok,0);
        if(ok[0]==0) throw new RuntimeException("Shader: "+GLES20.glGetShaderInfoLog(id));
        return id;
    }
    @Override public void onSurfaceCreated(GL10 ignored,EGLConfig config) {
        int vs=shader(GLES20.GL_VERTEX_SHADER,"attribute vec2 p; attribute vec4 color; varying vec4 vColor; void main(){gl_Position=vec4(p,0.0,1.0);vColor=color;}");
        int fs=shader(GLES20.GL_FRAGMENT_SHADER,"precision mediump float; varying vec4 vColor; void main(){gl_FragColor=vColor;}");
        program=GLES20.glCreateProgram();GLES20.glAttachShader(program,vs);GLES20.glAttachShader(program,fs);GLES20.glLinkProgram(program);
        int[] ok=new int[1];GLES20.glGetProgramiv(program,GLES20.GL_LINK_STATUS,ok,0);if(ok[0]==0) throw new RuntimeException("OpenGL link: "+GLES20.glGetProgramInfoLog(program));
        GLES20.glDeleteShader(vs);GLES20.glDeleteShader(fs);
        position=GLES20.glGetAttribLocation(program,"p");vertexColor=GLES20.glGetAttribLocation(program,"color");
    }
    @Override public void onSurfaceChanged(GL10 ignored,int w,int h) {width=w;height=h;}
    @Override public void onDrawFrame(GL10 ignored) {
        GLES20.glViewport(0,0,width,height);GLES20.glClearColor(0,0,0,1);GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        float scale=Math.min(width/320f,height/240f);int w=(int)(320*scale),h=(int)(240*scale);
        GLES20.glViewport((width-w)/2,(height-h)/2,w,h);GLES20.glUseProgram(program);
        synchronized(lock) {
            frame.position(0);GLES20.glVertexAttribPointer(position,2,GLES20.GL_FLOAT,false,12,frame);GLES20.glEnableVertexAttribArray(position);
            frame.position(8);GLES20.glVertexAttribPointer(vertexColor,4,GLES20.GL_UNSIGNED_BYTE,true,12,frame);GLES20.glEnableVertexAttribArray(vertexColor);
            GLES20.glDrawArrays(GLES20.GL_TRIANGLES,0,vertexCount);
        }
    }
}
