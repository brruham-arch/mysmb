static int l_spawn_at(lua_State *s) {
    int k = (int)luaL_checkinteger(s, 1);
    int wx = (int)luaL_checkinteger(s, 2);
    int py = (int)luaL_optinteger(s, 3, 0xb0);
    if (wx < 0) wx = 0;
    lua_pushinteger(s, extra_spawn((uint8_t)k, (uint8_t)((wx >> 8) & 255), (uint8_t)(wx & 255), (uint8_t)py));
    return 1;
}
static int l_item_at(lua_State *s) {
    int t = (int)luaL_checkinteger(s, 1);
    int wx = (int)luaL_checkinteger(s, 2);
    int py = (int)luaL_optinteger(s, 3, 0xb0);
    if (wx < 0) wx = 0;
    lua_pushinteger(s, extra_item_spawn((uint8_t)t, (uint8_t)((wx >> 8) & 255), (uint8_t)(wx & 255), (uint8_t)py));
    return 1;
}
static int l_item_count(lua_State *s) { lua_pushinteger(s, extra_item_count()); return 1; }
extern int extra_tags(char *out, int cap);
static int clean_name(lua_State *s, char *b) {
    const char *t = luaL_checkstring(s, 2); int j = 0;
    for (int k = 0; t[k] && j < 23; k++) if ((unsigned char)t[k] >= 32 && (unsigned char)t[k] < 127) b[j++] = t[k];
    b[j] = 0; return j;
}
static int l_ename(lua_State *s) { char b[24]; clean_name(s, b); extra_name((int)luaL_checkinteger(s, 1), b); return 0; }
static int l_oname(lua_State *s) { char b[24]; clean_name(s, b); extra_name_orig((int)luaL_checkinteger(s, 1), b); return 0; }
JNIEXPORT jstring JNICALL Java_com_brruham_smb_Core_tags(JNIEnv *env, jobject c) {
    static char buf[2048];
    extra_tags(buf, sizeof buf);
    return (*env)->NewStringUTF(env, buf);
}
