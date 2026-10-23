#include <jni.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "lib/common.h"
#include "lib/state.h"
#include "lib/debug_chr.h"

#define AUDIO_SAMPLE_RATE 48000

JNIEXPORT jint JNICALL Java_com_brruham_smb_Core_init(JNIEnv *env, jclass c, jstring path) {
    char buf[512];
    const char *p = (*env)->GetStringUTFChars(env, path, NULL);
    snprintf(buf, sizeof buf, "%s", p);
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

JNIEXPORT void JNICALL Java_com_brruham_smb_Core_input(JNIEnv *env, jclass c, jint m) {
    update_controller1((uint8_t)m);
}

JNIEXPORT void JNICALL Java_com_brruham_smb_Core_frame(JNIEnv *env, jclass c, jintArray out) {
    static jint buf[256 * 240];
    next_frame();
    render_frame();
    sound_end_frame();
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
