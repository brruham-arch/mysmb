#define DETMAX 36000
#define DETN 3600
static uint8_t drec[DETMAX];
static int dmode, dcount, dlen;
static uint32_t dh = 2166136261u;
static uint32_t hk[12];
uint8_t det_in(uint8_t m) {
    if (dmode == 1) m = (dcount < dlen) ? drec[dcount] : 0;
    else if (dcount < DETMAX) drec[dcount] = m;
    return m;
}
void det_after(const uint8_t *oam) {
    for (int i = 0; i < 2048; i++) dh = (dh ^ ram[i]) * 16777619u;
    for (int i = 0; i < 256; i++) dh = (dh ^ oam[i]) * 16777619u;
    dcount++;
    if (dcount % 300 == 0 && dcount / 300 <= 12) hk[dcount / 300 - 1] = dh;
}
JNIEXPORT void JNICALL Java_com_brruham_smb_Core_detSet(JNIEnv *env, jobject c, jbyteArray a) {
    jsize n = (*env)->GetArrayLength(env, a);
    if (n > DETMAX) n = DETMAX;
    (*env)->GetByteArrayRegion(env, a, 0, n, (jbyte *)drec);
    dlen = n; dmode = 1;
}
JNIEXPORT jbyteArray JNICALL Java_com_brruham_smb_Core_detGet(JNIEnv *env, jobject c) {
    int n = dcount < DETN ? dcount : DETN;
    jbyteArray a = (*env)->NewByteArray(env, n);
    (*env)->SetByteArrayRegion(env, a, 0, n, (const jbyte *)drec);
    return a;
}
JNIEXPORT jintArray JNICALL Java_com_brruham_smb_Core_detInfo(JNIEnv *env, jobject c) {
    jint v[14];
    v[0] = dmode; v[1] = dcount;
    for (int k = 0; k < 12; k++) v[2 + k] = (jint)hk[k];
    jintArray a = (*env)->NewIntArray(env, 14);
    (*env)->SetIntArrayRegion(env, a, 0, 14, v);
    return a;
}
