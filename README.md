# Echo Vault Android

Android port of the 24-stage C++ game. Use the virtual joystick on the left to move in any direction, including diagonally. START begins, confirms upgrades, and retries; moving the joystick up or down selects upgrades. FIRE, SWORD, DASH, GUN, PAUSE and SOUND control their respective actions. Pillars have 14-by-14 square collision shapes inside their 16-by-16 visual tiles.

Pause and tap HELP to open the game guide. Move the joystick left or right to change pages; tap the on-screen back button to return to the pause menu.

The original C++ game simulation and software pixel renderer are compiled using the Android NDK. Android displays the framebuffer and provides touch controls and simple sound effects. Progress is not saved after closing the app.

GitHub Actions builds a debug APK; download the Echo-Vault-Android-APK artifact from the successful run and extract the APK. Debug APKs are not Play Store release builds.

The desktop Windows Job Object CPU and memory restrictions are not applied on Android; GPU memory use is not limited or measured.
