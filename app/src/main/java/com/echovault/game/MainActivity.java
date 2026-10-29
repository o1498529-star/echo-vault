package com.echovault.game;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.FrameLayout;

public final class MainActivity extends Activity {
    private GameView game;
    private GpuSurface gpu;
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        gpu = new GpuSurface(this);
        game = new GameView(this,gpu);
        FrameLayout layout = new FrameLayout(this);
        layout.addView(gpu);layout.addView(game);
        setContentView(layout);
    }
    @Override protected void onResume() {super.onResume();if(gpu!=null) gpu.onResume();}
    @Override protected void onPause() { if (game != null) game.suspend();if(gpu!=null) gpu.onPause(); super.onPause(); }
    @Override public void onBackPressed() { if (game != null) game.pauseGame(); }
}
