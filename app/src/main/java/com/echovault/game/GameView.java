package com.echovault.game;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.HapticFeedbackConstants;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

public final class GameView extends View {
    static { System.loadLibrary("echovault"); }
    private static native void nativeInit(int[] save);
    private static native void nativeSuspend();
    private static native int nativeTick(int held, int pulse);
    private static native int nativeDraw(byte[] vertices);
    private static native int nativeMenuState();
    private static native int nativeMaxBytes();
    private static native boolean nativeTakeSave(int[] out);
    private static native boolean nativeClick(int x, int y);

    private static final int SAVE_INTS = 13;      // must equal SAVE_INTS in game_core.h
    private static final int BIT_PAUSE = 1 << 8;
    private final GpuSurface gpu;
    private final byte[] vertices = new byte[GpuSurface.MAX_BYTES];
    private final Paint paint = new Paint();
    private final String[] labels={"","","","","FIRE","SWORD","DASH","GUN","PAUSE","START","SOUND","HELP","SET"};
    private final float[] cx={0,0,0,0,.91f,.80f,.91f,.80f,.95f,.82f,.69f,.55f,.42f};
    private final float[] cy={0,0,0,0,.63f,.76f,.88f,.53f,.12f,.12f,.12f,.12f,.12f};
    // Input sources are kept apart and merged into `held`: on-screen touch, gamepad/keyboard buttons, gamepad sticks.
    private int held = 0, pending = 0, touchHeld = 0, padKeys = 0, padAxis = 0;
    private int joystickPointer = -1, menuPointer = -1;
    private float stickX = 0, stickY = 0;
    private long previous = 0;
    private double accumulator = 0;
    private final Sfx sfx = new Sfx();
    private final SharedPreferences prefs;
    private final int[] saveBuffer = new int[SAVE_INTS];

    GameView(Context context, GpuSurface surface) {
        super(context);
        gpu = surface;
        if (nativeMaxBytes() != GpuSurface.MAX_BYTES) throw new IllegalStateException("Vertex buffer size mismatch between Java and C++");
        setFocusable(true);
        setFocusableInTouchMode(true);
        prefs = context.getSharedPreferences("echovault", Context.MODE_PRIVATE);
        int[] saved = new int[SAVE_INTS];
        for (int i = 0; i < SAVE_INTS; i++) saved[i] = prefs.getInt("s" + i, 0);
        nativeInit(saved);
        requestFocus();
    }

    void pauseGame() { pending |= BIT_PAUSE; invalidate(); }
    void suspend() {
        held = pending = touchHeld = padKeys = padAxis = 0; joystickPointer = menuPointer = -1; stickX = stickY = 0;
        nativeSuspend(); previous = 0; accumulator = 0; flushSave();
    }
    private void flushSave() {
        if (!nativeTakeSave(saveBuffer)) return;
        SharedPreferences.Editor edit = prefs.edit();
        for (int i = 0; i < SAVE_INTS; i++) edit.putInt("s" + i, saveBuffer[i]);
        edit.apply();
    }
    private void syncHeld() { int next = touchHeld | padKeys | padAxis; pending |= next & ~held; held = next; }
    @Override protected void onDetachedFromWindow() { sfx.release(); super.onDetachedFromWindow(); }

    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas); long now = System.nanoTime();
        if (previous == 0) previous = now;
        accumulator += Math.min((now - previous) / 1e9, 0.1); previous = now;
        boolean first = true;
        while (accumulator >= 1.0 / 60.0) {
            int s = nativeTick(held, first ? pending : 0); if (first) pending = 0; first = false;
            if (s != 0) {
                sfx.play(s - 1);
                if (s == 3) performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);   // damage taken
            }
            accumulator -= 1.0 / 60.0;
        }
        flushSave();
        int count = nativeDraw(vertices); gpu.submit(vertices, count);
        int menu = nativeMenuState();
        float radius = Math.min(getHeight() * .071f, getWidth() * .036f);
        paint.setTextAlign(Paint.Align.CENTER); paint.setTextSize(Math.max(12, radius * .43f)); paint.setFakeBoldText(true);
        float joyX = getWidth() * .13f, joyY = getHeight() * .72f, joyR = getHeight() * .18f;
        paint.setColor(0x99142130); canvas.drawCircle(joyX, joyY, joyR, paint);
        paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(2); paint.setColor(0xbb93d8dd); canvas.drawCircle(joyX, joyY, joyR, paint); paint.setStyle(Paint.Style.FILL);
        paint.setColor(joystickPointer < 0 ? 0xaa93d8dd : 0xdd65dbd3); canvas.drawCircle(joyX + stickX * joyR * .65f, joyY + stickY * joyR * .65f, joyR * .37f, paint);
        for (int i = 4; i < labels.length; i++) {
            if (!visible(i, menu)) continue;
            float x = cx[i] * getWidth(), y = cy[i] * getHeight();
            paint.setColor((held & (1 << i)) != 0 ? 0xbb65dbd3 : 0x99142130); canvas.drawCircle(x, y, radius, paint);
            paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(2); paint.setColor(0xbb93d8dd); canvas.drawCircle(x, y, radius, paint); paint.setStyle(Paint.Style.FILL);
            paint.setColor(Color.WHITE); canvas.drawText((i == 11 && menu == 2) || (i == 12 && menu == 4) ? "BACK" : labels[i], x, y - (paint.ascent() + paint.descent()) / 2, paint);
        }
        postInvalidateOnAnimation();
    }
    private static boolean visible(int button, int menu) {
        if (button == 11) return menu == 1 || menu == 2;
        if (button == 12) return menu == 1 || menu == 3 || menu == 4;
        return true;
    }

    // ---- gamepad and keyboard ----
    private static int keyBits(int code) {
        switch (code) {
            case KeyEvent.KEYCODE_DPAD_UP: case KeyEvent.KEYCODE_W: return 1;
            case KeyEvent.KEYCODE_DPAD_DOWN: case KeyEvent.KEYCODE_S: return 2;
            case KeyEvent.KEYCODE_DPAD_LEFT: case KeyEvent.KEYCODE_A: return 4;
            case KeyEvent.KEYCODE_DPAD_RIGHT: case KeyEvent.KEYCODE_D: return 8;
            case KeyEvent.KEYCODE_SPACE: return 1 << 4;
            case KeyEvent.KEYCODE_BUTTON_A: return (1 << 4) | (1 << 9);     // fire, and confirm in menus
            case KeyEvent.KEYCODE_F: case KeyEvent.KEYCODE_BUTTON_X: return 1 << 5;
            case KeyEvent.KEYCODE_SHIFT_LEFT: case KeyEvent.KEYCODE_SHIFT_RIGHT: case KeyEvent.KEYCODE_BUTTON_B:
            case KeyEvent.KEYCODE_BUTTON_R1: case KeyEvent.KEYCODE_BUTTON_L1: return 1 << 6;
            case KeyEvent.KEYCODE_E: case KeyEvent.KEYCODE_BUTTON_Y: return 1 << 7;
            case KeyEvent.KEYCODE_P: case KeyEvent.KEYCODE_ESCAPE: case KeyEvent.KEYCODE_BUTTON_START: return BIT_PAUSE;
            case KeyEvent.KEYCODE_ENTER: case KeyEvent.KEYCODE_NUMPAD_ENTER: case KeyEvent.KEYCODE_DPAD_CENTER: return 1 << 9;
            case KeyEvent.KEYCODE_M: return 1 << 10;
            case KeyEvent.KEYCODE_H: return 1 << 11;
            case KeyEvent.KEYCODE_C: return 1 << 12;
            default: return 0;
        }
    }
    @Override public boolean onKeyDown(int code, KeyEvent event) {
        int bits = keyBits(code);
        if (bits == 0) return super.onKeyDown(code, event);
        padKeys |= bits; syncHeld(); return true;
    }
    @Override public boolean onKeyUp(int code, KeyEvent event) {
        int bits = keyBits(code);
        if (bits == 0) return super.onKeyUp(code, event);
        padKeys &= ~bits; syncHeld(); return true;
    }
    @Override public boolean onGenericMotionEvent(MotionEvent event) {
        if ((event.getSource() & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK || event.getActionMasked() != MotionEvent.ACTION_MOVE)
            return super.onGenericMotionEvent(event);
        float x = event.getAxisValue(MotionEvent.AXIS_X) + event.getAxisValue(MotionEvent.AXIS_HAT_X);
        float y = event.getAxisValue(MotionEvent.AXIS_Y) + event.getAxisValue(MotionEvent.AXIS_HAT_Y);
        float trigger = Math.max(event.getAxisValue(MotionEvent.AXIS_RTRIGGER), event.getAxisValue(MotionEvent.AXIS_GAS));
        int axis = 0;
        if (y < -.4f) axis |= 1; if (y > .4f) axis |= 2; if (x < -.4f) axis |= 4; if (x > .4f) axis |= 8;
        if (trigger > .5f) axis |= 1 << 4;
        padAxis = axis; syncHeld(); return true;
    }

    // ---- touch ----
    @Override public boolean onTouchEvent(MotionEvent event) {
        int next = 0, action = event.getActionMasked(), changed = event.getActionIndex(); float radius = Math.min(getHeight() * .085f, getWidth() * .043f);
        boolean down = action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN;
        boolean up = action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP;
        if (down) {
            // A tap that lands on a menu row is consumed by the menu and never starts the joystick or a button.
            float scale = Math.min(getWidth() / 320f, getHeight() / 240f);
            int gx = (int) ((event.getX(changed) - (getWidth() - 320 * scale) / 2) / scale), gy = (int) ((event.getY(changed) - (getHeight() - 240 * scale) / 2) / scale);
            if (nativeClick(gx, gy)) menuPointer = event.getPointerId(changed);
        }
        if (down && menuPointer != event.getPointerId(changed) && joystickPointer < 0 && event.getX(changed) < getWidth() * .27f && event.getY(changed) > getHeight() * .38f) joystickPointer = event.getPointerId(changed);
        if (action == MotionEvent.ACTION_CANCEL || (up && event.getPointerId(changed) == joystickPointer)) { joystickPointer = -1; stickX = stickY = 0; }
        if (action == MotionEvent.ACTION_CANCEL || (up && event.getPointerId(changed) == menuPointer)) menuPointer = -1;
        if (action != MotionEvent.ACTION_CANCEL) for (int p = 0; p < event.getPointerCount(); p++) {
            if (up && p == changed) continue;
            int id = event.getPointerId(p);
            if (id == menuPointer) continue;
            float x = event.getX(p), y = event.getY(p);
            if (id == joystickPointer) {
                float joyR = getHeight() * .18f, dx = (x - getWidth() * .13f) / joyR, dy = (y - getHeight() * .72f) / joyR;
                float mag = (float) Math.sqrt(dx * dx + dy * dy); if (mag > 1) { dx /= mag; dy /= mag; } stickX = dx; stickY = dy;
                if (dy < -.3f) next |= 1; if (dy > .3f) next |= 2; if (dx < -.3f) next |= 4; if (dx > .3f) next |= 8;
            } else {
                int menu = nativeMenuState(), nearest = -1; float best = radius * radius;
                for (int i = 4; i < labels.length; i++) {
                    if (!visible(i, menu)) continue;
                    float dx = x - cx[i] * getWidth(), dy = y - cy[i] * getHeight(), dist = dx * dx + dy * dy;
                    if (dist < best) { best = dist; nearest = i; }
                }
                if (nearest >= 0) next |= 1 << nearest;
            }
        }
        touchHeld = next; syncHeld(); return true;
    }
}
