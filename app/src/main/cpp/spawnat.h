static int l_spawn_at(lua_State *s) {
    int k = (int)luaL_checkinteger(s, 1);
    int wx = (int)luaL_checkinteger(s, 2);
    int py = (int)luaL_optinteger(s, 3, 0xb0);
    if (wx < 0) wx = 0;
    lua_pushinteger(s, extra_spawn((uint8_t)k, (uint8_t)((wx >> 8) & 255), (uint8_t)(wx & 255), (uint8_t)py));
    return 1;
}
