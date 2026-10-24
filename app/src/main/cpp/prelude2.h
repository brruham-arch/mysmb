#define STR(...) #__VA_ARGS__
static const char *PRELUDE2 = STR(
player = {}
hud = {}
local function s8(n) if n > 127 then return n - 256 end return n end
function player.x(v) if v then V.player_x = v % 256 V.player_page = math.floor(v / 256) else return V.player_page * 256 + V.player_x end end
function player.y(v) if v then V.player_y = v else return V.player_y end end
function player.xspeed(v) if v then V.player_x_speed = v % 256 else return s8(V.player_x_speed) end end
function player.yspeed(v) if v then V.player_y_speed = v % 256 else return s8(V.player_y_speed) end end
function player.lives(v) if v then V.lives = v else return V.lives end end
function player.coins(v) if v then V.coins = v else return V.coins end end
function player.star(f) V.star_timer = f or 255 end
function player.shield(f) V.injury_timer = f or 255 end
function player.jump(v) V.player_y_speed = (v or -5) % 256 V.player_state = 1 end
function player.powerup(n) mem.poke(0x756, n) mem.poke(0x754, n == 0 and 1 or 0) end
function player.kill() mem.poke(0x0e, 11) end
function hud.score(n) for i = 5, 0, -1 do V.player_score[i] = n % 10 n = math.floor(n / 10) end end
function enemy.kill_all() for i = 0, 4 do mem.poke(0x0f + i, 0) end enemy.reset() end
function player.star(sec) V.star_timer = math.min(255, math.ceil((sec or 12) * 60 / 21)) mem.poke(0xfb, 0x40) end
function player.powerup(n) mem.poke(0x756, n) mem.poke(0x754, n == 0 and 1 or 0) local ok = pcall(function() R.queue_player_colors{} end) if not ok then log("no queue_player_colors") end end
function player.kill() mem.poke(0x57, 0) mem.poke(0xfc, 1) mem.poke(0x9f, 0xfc) mem.poke(0x0e, 11) mem.poke(0x1d, 1) mem.poke(0x747, 255) mem.poke(0x775, 0) end
);
