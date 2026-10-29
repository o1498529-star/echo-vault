package com.echovault.game;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.view.MotionEvent;
import android.view.View;

public final class GameView extends View {
    static { System.loadLibrary("echovault"); }
    private static native void nativeInit();
    private static native void nativeSuspend();
    private static native int nativeTick(int held, int pulse);
    private static native void nativeDraw(int[] pixels);
    private final Bitmap image = Bitmap.createBitmap(320,240,Bitmap.Config.ARGB_8888);
    private final int[] pixels = new int[320*240];
    private final Paint paint = new Paint();
    private final String[] labels={"UP","DOWN","LEFT","RIGHT","FIRE","SWORD","DASH","GUN","PAUSE","START","SOUND"};
    private final float[] cx={.12f,.12f,.055f,.185f,.91f,.81f,.91f,.81f,.95f,.78f,.68f};
    private final float[] cy={.58f,.88f,.73f,.73f,.67f,.79f,.90f,.58f,.12f,.12f,.12f};
    private int held=0, pending=0;
    private long previous=0;
    private double accumulator=0;
    private ToneGenerator tones;

    GameView(Context context) {
        super(context); setFocusable(true); nativeInit();
        try { tones = new ToneGenerator(AudioManager.STREAM_MUSIC, 45); } catch (RuntimeException ignored) { tones=null; }
    }
    void pauseGame() { pending |= 1<<8; invalidate(); }
    void suspend() { held=0;pending=0;nativeSuspend();previous=0;accumulator=0; }
    @Override protected void onDetachedFromWindow() { if(tones!=null) {tones.release();tones=null;} super.onDetachedFromWindow(); }
    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas); long now=System.nanoTime();
        if(previous==0) previous=now;
        accumulator+=Math.min((now-previous)/1e9,0.1); previous=now;
        boolean first=true;
        while(accumulator>=1.0/60.0) {
            int s=nativeTick(held,first?pending:0); if(first) pending=0; first=false;
            if(s!=0 && tones!=null) tones.startTone(s==1?ToneGenerator.TONE_PROP_BEEP:s==3?ToneGenerator.TONE_PROP_NACK:ToneGenerator.TONE_PROP_ACK,55);
            accumulator-=1.0/60.0;
        }
        nativeDraw(pixels); image.setPixels(pixels,0,320,0,0,320,240);
        canvas.drawColor(Color.rgb(7,12,20));
        float scale=Math.min(getWidth()/320f,getHeight()/240f), w=320*scale,h=240*scale;
        paint.setFilterBitmap(false);paint.setAlpha(255);
        canvas.drawBitmap(image,null,new RectF((getWidth()-w)/2,(getHeight()-h)/2,(getWidth()+w)/2,(getHeight()+h)/2),paint);
        float radius=Math.min(getHeight()*.071f,getWidth()*.036f);
        paint.setTextAlign(Paint.Align.CENTER);paint.setTextSize(Math.max(12,radius*.43f));paint.setFakeBoldText(true);
        for(int i=0;i<labels.length;i++) {
            float x=cx[i]*getWidth(), y=cy[i]*getHeight();
            paint.setColor((held&(1<<i))!=0?0xbb65dbd3:0x99142130);canvas.drawCircle(x,y,radius,paint);
            paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(2);paint.setColor(0xbb93d8dd);canvas.drawCircle(x,y,radius,paint);paint.setStyle(Paint.Style.FILL);
            paint.setColor(Color.WHITE); canvas.drawText(labels[i],x,y-(paint.ascent()+paint.descent())/2,paint);
        }
        postInvalidateOnAnimation();
    }
    @Override public boolean onTouchEvent(MotionEvent event) {
        int next=0, action=event.getActionMasked(); float radius=Math.min(getHeight()*.085f,getWidth()*.043f);
        if(action!=MotionEvent.ACTION_CANCEL) for(int p=0;p<event.getPointerCount();p++) {
            if((action==MotionEvent.ACTION_UP || action==MotionEvent.ACTION_POINTER_UP) && p==event.getActionIndex()) continue;
            float x=event.getX(p),y=event.getY(p);
            for(int i=0;i<labels.length;i++) {float dx=x-cx[i]*getWidth(),dy=y-cy[i]*getHeight();if(dx*dx+dy*dy<=radius*radius) next|=1<<i;}
        }
        pending |= next & ~held;held=next;return true;
    }
}
