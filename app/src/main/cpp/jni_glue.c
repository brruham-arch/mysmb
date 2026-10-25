#include <jni.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "lib/common.h"
#include "lib/state.h"
#include "lib/debug_chr.h"

#define AUDIO_SAMPLE_RATE 48000
void lua_set_dir(const char *p);
void extra_run(void);
void extra_test(void);
void extra_overlay(uint8_t *o);
void extra_restore(uint8_t *o);
int net_on(void);
int net_begin(uint32_t, uint8_t *);
void net_end(void);

JNIEXPORT jint JNICALL Java_com_brruham_smb_Core_init(JNIEnv *env, jclass c, jstring path) {
    char buf[512];
    const char *p = (*env)->GetStringUTFChars(env, path, NULL);
    snprintf(buf, sizeof buf, "%s", p);
    lua_set_dir(buf);
    (*env)->ReleaseStringUTFChars(env, path, p);
    cpu_init();
    apu_init(AUDIO_SAMPLE_RATE);
    int bad = read_chr_rom(buf);
    if (bad) {
        uint8_t chr[DEBUG_CHR_SIZE];
        debug_chr_generate(chr);
        video_init(chr);
        render_init();
    }
    cpu_reset_entry();
    return bad ? 1 : 0;
}

JNIEXPORT jint JNICALL Java_com_brruham_smb_Core_size(JNIEnv *env, jclass c) {
    return (jint)((SCREEN_WIDTH << 16) | SCREEN_HEIGHT);
}

extern uint8_t lua_input(uint32_t);
extern uint8_t det_in(uint8_t);
extern void det_after(const uint8_t *);
static volatile uint32_t raw_in;
JNIEXPORT void JNICALL Java_com_brruham_smb_Core_input(JNIEnv *env, jclass c, jint m) {
    raw_in = (uint32_t)m;
}

JNIEXPORT void JNICALL Java_com_brruham_smb_Core_frame(JNIEnv *env, jclass c, jintArray out) {
    static jint buf[256 * 240];
    if (net_on()) {
        uint8_t m1;
        if (!net_begin((uint32_t)raw_in, &m1)) goto present;
        update_controller1(m1);
    } else update_controller1(det_in(lua_input(raw_in)));
    next_frame();
    det_after(video.oam);
    if (net_on()) net_end();
    //extra_test();
    //extra_run();
    extra_overlay(video.oam);
    render_frame();
    extra_restore(video.oam);
    sound_end_frame();
present: ;
    const uint8_t *f = (const uint8_t *)frame;
    int n = SCREEN_WIDTH * SCREEN_HEIGHT;
    if (n > 256 * 240) n = 256 * 240;
    for (int i = 0; i < n; i++) {
        buf[i] = (jint)(0xFF000000u | ((uint32_t)f[i * 3] << 16) | ((uint32_t)f[i * 3 + 1] << 8) | f[i * 3 + 2]);
    }
    (*env)->SetIntArrayRegion(env, out, 0, n, buf);
}

JNIEXPORT void JNICALL Java_com_brruham_smb_Core_audio(JNIEnv *env, jclass c, jfloatArray out, jint n) {
    static float tmp[4096];
    if (n > 4096) n = 4096;
    apu_render(tmp, (size_t)n);
    (*env)->SetFloatArrayRegion(env, out, 0, n, tmp);
}
int extra_count(void);
JNIEXPORT jintArray JNICALL Java_com_brruham_smb_Core_stats(JNIEnv *env, jclass c) {
    jint s[2]; s[0] = extra_count(); s[1] = 0;
    for (int i = 0; i < 64; i++) if (video.oam[i * 4] < 0xef) s[1]++;
    jintArray r = (*env)->NewIntArray(env, 2);
    (*env)->SetIntArrayRegion(env, r, 0, 2, s);
    return r;
}
