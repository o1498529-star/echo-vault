package com.echovault.game;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;

/** The five game sounds, synthesized at start-up (same waveforms as the Windows build) into tiny static AudioTracks. */
final class Sfx {
    private static final int RATE = 16000, SAMPLES = 3200;
    private final AudioTrack[] tracks = new AudioTrack[5];

    Sfx() {
        for (int k = 0; k < tracks.length; k++) {
            try { tracks[k] = build(k); } catch (RuntimeException e) { tracks[k] = null; }
        }
    }

    private static AudioTrack build(int k) {
        short[] pcm = new short[SAMPLES];
        float phase = 0;
        for (int i = 0; i < SAMPLES; i++) {
            float t = (float) i / SAMPLES;
            float hz = k == 0 ? 700 - 450 * t : k == 1 ? 400 + 600 * t : k == 2 ? 180 - 100 * t : k == 3 ? 260 + 460 * t : 120 + 100 * t;
            phase += hz / RATE;
            if (phase >= 1) phase -= 1;
            float amp = 45 * (1 - t);
            pcm[i] = (short) ((phase < 0.5f ? amp : -amp) * 256);
        }
        AudioTrack track = new AudioTrack.Builder()
            .setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build())
            .setAudioFormat(new AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(RATE).setChannelMask(AudioFormat.CHANNEL_OUT_MONO).build())
            .setBufferSizeInBytes(SAMPLES * 2)
            .setTransferMode(AudioTrack.MODE_STATIC)
            .build();
        if (track.write(pcm, 0, SAMPLES) != SAMPLES) { track.release(); return null; }
        return track;
    }

    void play(int id) {
        if (id < 0 || id >= tracks.length || tracks[id] == null) return;
        try {
            AudioTrack t = tracks[id];
            t.stop();
            t.reloadStaticData();
            t.play();
        } catch (IllegalStateException ignored) { }
    }

    void release() {
        for (int k = 0; k < tracks.length; k++) {
            if (tracks[k] != null) { try { tracks[k].release(); } catch (RuntimeException ignored) { } tracks[k] = null; }
        }
    }
}
