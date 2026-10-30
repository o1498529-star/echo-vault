# Echo Vault Android

Android port of the 36-stage C++ game. Use the virtual joystick on the left to move in any direction, including diagonally. START begins, confirms upgrades, and retries; moving the joystick up or down selects upgrades (you can also tap an upgrade row, and tap the selected row again to confirm). FIRE, SWORD, DASH, GUN, PAUSE and SOUND control their respective actions. Pillars have 14-by-14 square collision shapes inside their 16-by-16 visual tiles.

Pause and tap HELP to open the game guide. Move the joystick left or right to change pages; tap BACK (or the on-screen back button) to return to the pause menu. Pause-menu rows can be tapped directly.

Tap SET on the title or pause screen to choose difficulty, player color, starting gun and sound. Move the joystick up or down to select a row, and left or right to change it, or tap a row to change it. Tap BACK to leave settings. Starting gun applies to a new game. Four difficulty levels change player speed, enemy speed, spawn rate and the maximum number of active enemies. Six bosses use three attack patterns.

## What's new in 2.0

- **Saved data:** settings and the best score / furthest stage per difficulty are kept between launches (SharedPreferences, 13 integers).
- **Score chains:** consecutive kills raise a score multiplier up to x4; taking damage or 3 s without a kill ends the chain. Sword kills are worth more but cut yellow enemies cleanly (no ring of bullets).
- **New upgrade:** REPAIR restores two HP.
- **Sound:** the same five synthesized effects as the Windows build, played from small in-memory `AudioTrack`s (no sound files). Taking damage gives a haptic tick.
- **Controllers and keyboards:** gamepad (D-pad or left stick, A fire/confirm, X sword, B/R1/L1 dash, Y gun, Start pause, right trigger fire) and a physical keyboard (WASD, arrows, Space, F, E, Shift, P, Enter, M, H, C) work alongside touch.
- **Polish:** screen shake on hits, boss hit flash, sensor-landscape orientation, notch-aware immersive mode, screen kept awake, taps on menus never also steer the joystick.
- **Build:** native code built with `-fno-exceptions -fno-rtti`, section GC and 16 KB page alignment; ABIs limited to arm64-v8a, armeabi-v7a and x86_64.

## Resource use

The C++ game simulation produces a batch of colored rectangles (at most 6144 per frame, in a fixed 442 KB buffer; the busiest measured frame used about 3,960). OpenGL ES 2.0 draws the pixel art on the graphics renderer, while Android overlays touch controls. The CPU still runs game logic and prepares geometry. A graphics driver may use software rendering. There is no per-frame allocation in the game core: all state lives in fixed static arrays.

The desktop Windows Job Object CPU and memory restrictions are not applied on Android; GPU memory use is not limited or measured.

## Code layout

`game_core.h` (all gameplay, drawing and menus) and `campaign.h` are shared with the Windows project and must stay identical to `pixel_game/`; run `sync_core.sh` from the parent folder after editing them there. `game.cpp` is only the JNI glue.

## Building

GitHub Actions builds a debug APK with the `Build Android Debug APK` workflow; download the `Echo-Vault-Android-Debug-APK` artifact from a successful run. A separate workflow builds an unsigned release APK in the `Echo-Vault-Android-unsigned-release` artifact; sign it before installing.
