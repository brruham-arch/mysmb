static const char *BN[10] = {"a","b","select","start","up","down","left","right","ta","tb"};
static int ipr[8], ibl[8];
static uint8_t itb, iphys;
static uint16_t iprev;
static unsigned ifc;
static int bidx(lua_State *s, int i) {
    const char *n = luaL_checkstring(s, i);
    for (int k = 0; k < 8; k++) if (!strcmp(BN[k], n)) return k;
    return luaL_error(s, "unknown button: %s", n);
}
static int l_ipress(lua_State *s) { ipr[bidx(s, 1)] = (int)luaL_optinteger(s, 2, 3); return 0; }
static int l_iblock(lua_State *s) { ibl[bidx(s, 1)] = (int)luaL_optinteger(s, 2, 3); return 0; }
static int l_iturbo(lua_State *s) {
    int k = bidx(s, 1);
    if (lua_toboolean(s, 2)) itb |= (uint8_t)(1 << k); else itb &= (uint8_t)~(1 << k);
    return 0;
}
static int l_iget(lua_State *s) {
    lua_newtable(s);
    for (int k = 0; k < 8; k++) { lua_pushboolean(s, (iphys >> k) & 1); lua_setfield(s, -2, BN[k]); }
    return 1;
}
static const luaL_Reg inputlib[] = {
    {"press", l_ipress}, {"block", l_iblock}, {"turbo", l_iturbo}, {"get", l_iget}, {NULL, NULL}};
uint8_t lua_input(uint32_t raw) {
    uint16_t p = (uint16_t)(raw & 0x3ff);
    uint8_t m = (uint8_t)(p & 0xff);
    ifc++;
    iphys = m;
    if (ok && L) {
        uint16_t ch = p ^ iprev;
        for (int k = 0; k < 10 && ok; k++) if ((ch >> k) & 1) {
            lua_getglobal(L, "on_button");
            if (!lua_isfunction(L, -1)) { lua_pop(L, 1); break; }
            lua_pushstring(L, BN[k]);
            lua_pushboolean(L, (p >> k) & 1);
            hticks = 0;
            if (lua_pcall(L, 2, 0, 0)) {
                const char *e = lua_tostring(L, -1);
                snprintf(status, sizeof status, "lua:ERR %.80s", e ? e : "?");
                lua_pop(L, 1);
                ok = 0;
            }
        }
    }
    iprev = p;
    uint8_t tb = itb | ((p & 0x100) ? 1 : 0) | ((p & 0x200) ? 2 : 0);
    for (int k = 0; k < 2; k++) if ((tb >> k) & 1) {
        if ((ifc / 3) & 1) m &= (uint8_t)~(1 << k); else m |= (uint8_t)(1 << k);
    }
    for (int k = 0; k < 8; k++) if (ibl[k] > 0) { m &= (uint8_t)~(1 << k); ibl[k]--; }
    for (int k = 0; k < 8; k++) if (ipr[k] > 0) { m |= (uint8_t)(1 << k); ipr[k]--; }
    return m;
}
