#include <jni.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "cpu.h"
#include "prelude2.h"
#include "prelude3.h"

typedef struct { const char *n; uint16_t a; uint16_t sz; } VarEnt;
typedef struct { const char *n; void (*f)(void); } RtnEnt;
extern const VarEnt api_vars[];
extern const RtnEnt api_rtns[];
extern int extra_spawn(uint8_t k, uint8_t pg, uint8_t px, uint8_t py);
extern int extra_count(void);
extern void extra_reset(void);
extern void extra_name(int, const char *);
extern void extra_name_orig(int, const char *);
extern int extra_item_spawn(uint8_t, uint8_t, uint8_t, uint8_t);
extern int extra_item_count(void);
#include "spawnat.h"

static lua_State *L;
static int ok;
static char status[96] = "lua:menunggu script";
static char sfile[300];
static char cur[300];
static time_t mt;
static unsigned fcount;
static pthread_mutex_t qm = PTHREAD_MUTEX_INITIALIZER;
static char q[16][128];
static int qn;

static const char *PRELUDE =
"V = setmetatable({}, {\n"
" __index = function(_, k)\n"
"  if mem.size(k) > 1 then\n"
"   return setmetatable({}, {\n"
"    __index = function(_, i) return mem.get(k, i) end,\n"
"    __newindex = function(_, i, v) mem.set(k, i, v) end })\n"
"  end\n"
"  return mem.get(k)\n"
" end,\n"
" __newindex = function(_, k, v) mem.set(k, v) end })\n"
"R = setmetatable({}, { __index = function(_, n)\n"
"  return function(regs) return call(n, regs) end end })\n";

static const VarEnt *findv(const char *n) {
    for (const VarEnt *v = api_vars; v->n; v++) if (!strcmp(v->n, n)) return v;
    return NULL;
}
static const VarEnt *needv(lua_State *s, int i) {
    const char *n = luaL_checkstring(s, i);
    const VarEnt *v = findv(n);
    if (!v) luaL_error(s, "unknown var: %s", n);
    return v;
}
static int l_get(lua_State *s) {
    const VarEnt *v = needv(s, 1);
    int i = (int)luaL_optinteger(s, 2, 0);
    if (i < 0 || i >= v->sz) return luaL_error(s, "index %d out of range for %s", i, v->n);
    lua_pushinteger(s, ram[v->a + i]);
    return 1;
}
static int l_set(lua_State *s) {
    const VarEnt *v = needv(s, 1);
    int i = 0, k = 2;
    if (lua_gettop(s) >= 3) { i = (int)luaL_checkinteger(s, 2); k = 3; }
    if (i < 0 || i >= v->sz) return luaL_error(s, "index %d out of range for %s", i, v->n);
    ram[v->a + i] = (uint8_t)luaL_checkinteger(s, k);
    return 0;
}
static int l_size(lua_State *s) {
    const VarEnt *v = findv(luaL_checkstring(s, 1));
    lua_pushinteger(s, v ? v->sz : 0);
    return 1;
}
static int l_peek(lua_State *s) {
    int ad = (int)luaL_checkinteger(s, 1);
    if (ad < 0 || ad >= RAM_SIZE) return luaL_error(s, "bad addr");
    lua_pushinteger(s, ram[ad]);
    return 1;
}
static int l_poke(lua_State *s) {
    int ad = (int)luaL_checkinteger(s, 1);
    if (ad < 0 || ad >= RAM_SIZE) return luaL_error(s, "bad addr");
    ram[ad] = (uint8_t)luaL_checkinteger(s, 2);
    return 0;
}
static void rd_reg(lua_State *s, const char *k, uint8_t *dst) {
    lua_getfield(s, 2, k);
    if (!lua_isnil(s, -1)) *dst = (uint8_t)lua_tointeger(s, -1);
    lua_pop(s, 1);
}
static void wr_reg(lua_State *s, const char *k, int v) {
    lua_pushinteger(s, v);
    lua_setfield(s, -2, k);
}
static int l_call(lua_State *s) {
    const char *n = luaL_checkstring(s, 1);
    const RtnEnt *r = api_rtns;
    while (r->n && strcmp(r->n, n)) r++;
    if (!r->n) return luaL_error(s, "routine not callable: %s", n);
    if (lua_istable(s, 2)) {
        rd_reg(s, "a", &a); rd_reg(s, "x", &x); rd_reg(s, "y", &y);
        rd_reg(s, "slot", &ram[8]);
    }
    r->f();
    lua_newtable(s);
    wr_reg(s, "a", a); wr_reg(s, "x", x); wr_reg(s, "y", y);
    lua_pushboolean(s, carry_flag);
    lua_setfield(s, -2, "carry");
    return 1;
}
static int l_log(lua_State *s) {
    snprintf(status, sizeof status, "lua:%.80s", luaL_tolstring(s, 1, NULL));
    return 0;
}
static int l_spawn(lua_State *s) {
    int kind = (int)luaL_checkinteger(s, 1);
    int off = (int)luaL_optinteger(s, 2, 40);
    int py = (int)luaL_optinteger(s, 3, 0xb0);
    const VarEnt *vx = findv("screen_right_x"), *vp = findv("screen_right_page");
    if (!vx || !vp) return luaL_error(s, "screen_right_x/page not in map");
    int xr = ram[vx->a] + off;
    int page = ram[vp->a] + (xr >> 8);
    lua_pushinteger(s, extra_spawn((uint8_t)kind, (uint8_t)page, (uint8_t)(xr & 0xff), (uint8_t)py));
    return 1;
}
static int l_ecount(lua_State *s) { lua_pushinteger(s, extra_count()); return 1; }
static int l_ereset(lua_State *s) { extra_reset(); return 0; }
extern void p2_setfire(int);
static int l_p2fire(lua_State *s) { p2_setfire(lua_toboolean(s, 1)); return 0; }

static const luaL_Reg memlib[] = {
    {"get", l_get}, {"set", l_set}, {"size", l_size},
    {"peek", l_peek}, {"poke", l_poke}, {NULL, NULL}};
static const luaL_Reg enemylib[] = {
    {"spawn", l_spawn}, {"spawn_at", l_spawn_at}, {"item_at", l_item_at}, {"item_count", l_item_count}, {"name", l_ename}, {"name_orig", l_oname}, {"count", l_ecount}, {"reset", l_ereset}, {"p2fire", l_p2fire}, {NULL, NULL}};

static char *pending;
static pthread_mutex_t pm = PTHREAD_MUTEX_INITIALIZER;
static int hticks;
#include "inputapi.h"
static void hookfn(lua_State *s, lua_Debug *d) {
    if (++hticks >= 2) luaL_error(s, "script terlalu lama");
}
static void load(const char *src) {
    if (L) lua_close(L);
    ok = 0;
    L = luaL_newstate();
    luaL_openlibs(L);
    luaL_newlib(L, memlib);   lua_setglobal(L, "mem");
    luaL_newlib(L, enemylib); lua_setglobal(L, "enemy");
    luaL_newlib(L, inputlib); lua_setglobal(L, "input");
    lua_pushcfunction(L, l_call); lua_setglobal(L, "call");
    lua_pushcfunction(L, l_log);  lua_setglobal(L, "log");
    if (luaL_dostring(L, PRELUDE) || luaL_dostring(L, PRELUDE2) || luaL_dostring(L, PRELUDE3)) {
        const char *m = lua_tostring(L, -1);
        snprintf(status, sizeof status, "lua:prelude %.70s", m ? m : "?");
        lua_pop(L, 1);
        return;
    }
    hticks = 0;
    lua_sethook(L, hookfn, LUA_MASKCOUNT, 1000000);
    if (luaL_dostring(L, src)) {
        const char *m = lua_tostring(L, -1);
        snprintf(status, sizeof status, "lua:ERR %.80s", m ? m : "?");
        lua_pop(L, 1);
        return;
    }
    ok = 1;
    snprintf(status, sizeof status, "lua:ok");
}
static void run_hook(const char *name, const char *arg) {
    lua_getglobal(L, name);
    if (!lua_isfunction(L, -1)) { lua_pop(L, 1); return; }
    int n = 0;
    hticks = 0;
    if (arg) { lua_pushstring(L, arg); n = 1; }
    if (lua_pcall(L, n, 0, 0)) {
        const char *m = lua_tostring(L, -1);
        snprintf(status, sizeof status, "lua:ERR %.80s", m ? m : "?");
        lua_pop(L, 1);
        ok = 0;
    }
}
void lua_set_dir(const char *rom) { (void)rom; }
void lua_frame(void) {
    uint8_t sa = a, sx = x, sy = y;
    char *src;
    pthread_mutex_lock(&pm);
    src = pending; pending = NULL;
    pthread_mutex_unlock(&pm);
    if (src) { extra_reset(); load(src); free(src); }
    if (ok) {
        char ev[16][128]; int n;
        pthread_mutex_lock(&qm);
        n = qn; memcpy(ev, q, sizeof q); qn = 0;
        pthread_mutex_unlock(&qm);
        for (int i = 0; i < n && ok; i++) run_hook("on_event", ev[i]);
        if (ok) { run_hook("__tick", NULL); if (ok) run_hook("on_frame", NULL); }
    }
    a = sa; x = sx; y = sy;
}
JNIEXPORT void JNICALL Java_com_brruham_smb_Core_luaLoad(JNIEnv *env, jobject c, jstring js) {
    const char *t = (*env)->GetStringUTFChars(env, js, NULL);
    char *cp = strdup(t);
    (*env)->ReleaseStringUTFChars(env, js, t);
    pthread_mutex_lock(&pm);
    free(pending); pending = cp;
    pthread_mutex_unlock(&pm);
}
JNIEXPORT void JNICALL Java_com_brruham_smb_Core_pushEvent(JNIEnv *env, jobject c, jstring js) {
    const char *t = (*env)->GetStringUTFChars(env, js, NULL);
    pthread_mutex_lock(&qm);
    if (qn < 16) snprintf(q[qn++], 128, "%s", t);
    pthread_mutex_unlock(&qm);
    (*env)->ReleaseStringUTFChars(env, js, t);
}
JNIEXPORT jstring JNICALL Java_com_brruham_smb_Core_luaStatus(JNIEnv *env, jobject c) {
    return (*env)->NewStringUTF(env, status);
}
