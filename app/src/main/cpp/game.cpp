// Android glue: JNI entry points around the shared game core (game_core.h).
#include <jni.h>
constexpr int VK_SHIFT=16, VK_ESCAPE=27, VK_SPACE=32, VK_LEFT=37, VK_UP=38, VK_RIGHT=39, VK_DOWN=40, VK_RETURN=13;
#define EV_TOUCH 1
#include "game_core.h"

static int pending_sound=0;
static void sound(int id) { if(!muted) pending_sound=id+1; }   // Java plays it; 0 means none
static void apply_mute() {}

#define JNI(ret,name) extern "C" JNIEXPORT ret JNICALL Java_com_echovault_game_GameView_##name

// `save` holds SAVE_INTS ints previously written by nativeTakeSave (all zero on first launch).
JNI(void,nativeInit)(JNIEnv* env, jclass, jintArray save) {
    int data[SAVE_INTS]={0};
    if(save && env->GetArrayLength(save)>=SAVE_INTS) env->GetIntArrayRegion(save,0,SAVE_INTS,data);
    save_unpack(data);   // ignored when the data is empty or from an unknown version
    reset_game(); scene=TITLE; save_dirty=false;
}
JNI(void,nativeSuspend)(JNIEnv*, jclass) { auto_pause(); }
JNI(jint,nativeMenuState)(JNIEnv*, jclass) {return scene==PAUSED?1:scene==HELP?2:scene==TITLE?3:scene==SETTINGS?4:0;}
JNI(jint,nativeMaxBytes)(JNIEnv*, jclass) {return MAX_VERTICES*int(sizeof(GpuVertex));}
// Copies the settings and best scores out when they changed since the last call.
JNI(jboolean,nativeTakeSave)(JNIEnv* env, jclass, jintArray out) {
    if(!save_dirty || !out || env->GetArrayLength(out)<SAVE_INTS) return JNI_FALSE;
    int data[SAVE_INTS]; save_pack(data); env->SetIntArrayRegion(out,0,SAVE_INTS,data); save_dirty=false; return JNI_TRUE;
}
// Tap at logical 320x240 coordinates; true when it hit a menu control.
JNI(jboolean,nativeClick)(JNIEnv*, jclass, jint x, jint y) {return ui_click(x,y)?JNI_TRUE:JNI_FALSE;}
JNI(jint,nativeTick)(JNIEnv*, jclass, jint held, jint pulse) {
    static const int codes[]={VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,VK_SPACE,'F',VK_SHIFT,'E','P',VK_RETURN,'M','H','C'};
    for(int i=0;i<13;i++) {bool down=(held & (1<<i))!=0; pressed[codes[i]]=(down&&!keys[codes[i]]) || (pulse & (1<<i));keys[codes[i]]=down;}
    step();
    memset(pressed,0,sizeof(pressed)); int s=pending_sound;pending_sound=0;return s;
}
JNI(jint,nativeDraw)(JNIEnv* env, jclass, jbyteArray bytes) {
    draw(); int count=vertex_count; int size=count*int(sizeof(GpuVertex));
    if(env->GetArrayLength(bytes)<size) return 0;
    env->SetByteArrayRegion(bytes,0,size,reinterpret_cast<const jbyte*>(vertices)); return count;
}
