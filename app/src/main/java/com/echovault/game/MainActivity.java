package com.echovault.game;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;

public final class MainActivity extends Activity {
    private GameView game;
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        game = new GameView(this);
        setContentView(game);
    }
    @Override protected void onPause() { if (game != null) game.suspend(); super.onPause(); }
    @Override public void onBackPressed() { if (game != null) game.pauseGame(); }
}
