# Echo Vault Android

Android port of the 24-stage C++ game. Use the virtual joystick on the left to move in any direction, including diagonally. START begins, confirms upgrades, and retries; moving the joystick up or down selects upgrades. FIRE, SWORD, DASH, GUN, PAUSE and SOUND control their respective actions. Pillars retain their square appearance while their collision shape is round.

The original C++ game simulation and software pixel renderer are compiled using the Android NDK. Android displays the framebuffer and provides touch controls and simple sound effects. Progress is not saved after closing the app.

GitHub Actions builds a debug APK; download the Echo-Vault-Android-APK artifact from the successful run and extract the APK. Debug APKs are not Play Store release builds.

The desktop Windows Job Object CPU and memory restrictions are not applied on Android; GPU memory use is not limited or measured.
