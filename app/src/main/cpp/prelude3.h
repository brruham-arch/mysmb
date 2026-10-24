static const char *PRELUDE3 = STR(
world = {}
local function s8(n) if n > 127 then return n - 256 end return n end
local EN = {flag=0x0f, id=0x16, state=0x1e, dir=0x46, xs=0x58, page=0x6e, x=0x87, ys=0xa0, y=0xcf}
local tasks = {}
function task(fn, ...) tasks[#tasks+1] = {co=coroutine.create(fn), w=0, a={...}} end
function wait(n) coroutine.yield(n or 1) end
function __tick()
  local i = 1
  while i <= #tasks do
    local t = tasks[i]
    if t.w > 0 then t.w = t.w - 1 end
    if t.w > 0 then i = i + 1 else
      local ok, r = coroutine.resume(t.co, table.unpack(t.a))
      t.a = {}
      if not ok then log("task ERR " .. tostring(r)) table.remove(tasks, i)
      elseif coroutine.status(t.co) == "dead" then table.remove(tasks, i)
      else t.w = r or 1 i = i + 1 end
    end
  end
end
function enemy.get(i) local e = {i=i} for k, a in pairs(EN) do e[k] = mem.peek(a + i) end e.xs = s8(e.xs) e.ys = s8(e.ys) e.wx = e.page * 256 + e.x return e end
function enemy.set(i, t) for k, v in pairs(t) do if EN[k] then mem.poke(EN[k] + i, v % 256) end end end
function enemy.kill(i) mem.poke(0x0f + i, 0) end
function enemy.stop(i) mem.poke(0x58 + i, 0) mem.poke(0xa0 + i, 0) end
function enemy.push(i, xs, ys) mem.poke(0x58 + i, xs % 256) mem.poke(0xa0 + i, (ys or 0) % 256) end
function enemy.list() local r = {} for i = 0, 4 do if mem.peek(0x0f + i) ~= 0 then r[#r+1] = enemy.get(i) end end return r end
function world.x() return V.screen_left_page * 256 + V.screen_left_x end
function world.number() return V.world_number + 1, V.level_number + 1 end
function world.lock_scroll(on) V.scroll_locked = on and 1 or 0 end
function hud.time(n) mem.poke(0x7f8, math.floor(n / 100) % 10) mem.poke(0x7f9, math.floor(n / 10) % 10) mem.poke(0x7fa, n % 10) end
function hud.get_time() return mem.peek(0x7f8) * 100 + mem.peek(0x7f9) * 10 + mem.peek(0x7fa) end
function player.big() player.powerup(1) end
function player.small() player.powerup(0) end
function player.fire() player.powerup(2) end
function player.status() return mem.peek(0x756) end
item = {}
local KIND = {mushroom=0, flower=1, star=2, ["1up"]=3}
function item.spawn(kind, wx, y, force)
  local t = KIND[kind] or kind
  wx = math.floor(wx or (player.x() + 80))
  if t < 2 and not force then t = math.min(mem.peek(0x756), 1) end
  mem.poke(0x16 + 5, 0x2e)
  mem.poke(0x6e + 5, math.floor(wx / 256) % 256)
  mem.poke(0x87 + 5, wx % 256)
  mem.poke(0xb6 + 5, 1)
  mem.poke(0xcf + 5, ((y or 0xb0) + 16) % 256)
  mem.poke(0x1e + 5, 1)
  mem.poke(0x0f + 5, 1)
  mem.poke(0x49a + 5, 3)
  mem.poke(0x39, t)
  mem.poke(0x3c5 + 5, 0x20)
  mem.poke(0xfe, 2)
end
function item.give(kind)
  if kind == "mushroom" then if player.status() == 0 then player.big() end
  elseif kind == "flower" then player.fire()
  elseif kind == "star" then player.star(12)
  elseif kind == "1up" then V.lives = V.lives + 1 end
end
);
