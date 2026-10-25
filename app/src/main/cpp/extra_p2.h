// extra_p2.h - Pemain 2 lokal (3a tampil, 3b tabrakan+power-up, 3c mati/respawn, 3d kamera)
#include <sys/stat.h>
#include <stdio.h>
#define P2LOG(...) do { FILE *lg = fopen("/sdcard/SMB/p2log.txt", "a"); if (lg) { fprintf(lg, __VA_ARGS__); fclose(lg); } } while (0)
static uint8_t p2_netin;
static int p2_netmode;
extern void control_player(void);
extern void draw_player(void);
extern void player_enemy_collision(void);
extern void scroll_screen(void);
static void put(const Ex *e, int sl);
static void get(Ex *e, int sl);
#define P2S 0xe0
#define P2_DELAY 20
#define P2_PAL 1
#define P2_RTIME 120
#define P2_INV 8
#define P2_LEFT 0x14
#define P2_STUCK 240
static const uint16_t PA[] = {
  0x0a,0x0b,0x0c,0x0d,0x0e,0x1d,0x33,0x45,0x57,0x6d,0x86,0x9f,0xb5,0xce,
  0x3ad,0x3b8,0x3c4,0x3d0,0x416,0x433,0x450,0x456,0x490,0x499,
  0x4ac,0x4ad,0x4ae,0x4af,
  0x6d5,0x6db,0x6ff,0x700,0x701,0x702,0x703,0x704,0x705,0x706,0x707,0x708,
  0x709,0x70a,0x70b,0x70c,0x70d,0x70e,0x711,0x714,0x754,0x755,0x756,
  0x781,0x782,0x783,0x785,0x786,0x789,0x79e,0x79f };
#define NPA ((int)(sizeof(PA) / sizeof(PA[0])))
typedef char p2_chk[(sizeof(PA) / sizeof(PA[0])) <= 64 ? 1 : -1];
typedef struct { uint8_t s[64], ring[32], spr[32], rp, tk, en, ok, rs, vis, dead, dy, rt, go, slk, sv, lk, sa, pend; uint16_t sg; } P2;
static P2 p2;
static uint16_t p2_sig(void) {
    return (uint16_t)(ram[0x750]*31u + ram[0x74f]*17u + ram[0x74e]*7u + ram[0x75c]*131u + ram[0x75f]*257u + ram[0x760]*13u);
}
static int pai(uint16_t a) { for (int i = 0; i < NPA; i++) if (PA[i] == a) return i; return 0; }
static void p2_reset(void) { uint8_t en = p2.en; memset(&p2, 0, sizeof p2); p2.en = en; }
static void p2_paste(uint8_t *oam, int *cur) {
    if (!p2.en || !p2.vis) return;
    for (int s = 0; s < 8; s++) {
        if (p2.spr[s * 4] >= 0xef) continue;
        while (*cur < 64 && oam[*cur * 4] < 0xef) (*cur)++;
        if (*cur >= 64) return;
        memcpy(&oam[*cur * 4], &p2.spr[s * 4], 4);
        (*cur)++;
    }
}
// dipanggil sebelum run_player_task P1
int p2_pre(void) {
    if (!p2.en || !p2.ok) return 0;
    { uint16_t sg = p2_sig(); if (sg != p2.sg) { p2.sg = sg; p2.pend = 1; } }
    if (p2.pend) {
        if (ram[0x0e] == 8 && ram[0x770] == 1 && ram[0x772] == 3) { p2.pend = 0; if (!p2.dead) p2.rs = 4; }
        else return 0;
    }
    int lockp1 = 0, p2sc = 0;
    int sl = ram[0x71a] * 256 + ram[0x71c];
    int x1 = ram[0x6d] * 256 + ram[0x86];
    if (!p2.dead && ram[0x0e] == 8 && ram[0x723] == 0) {
        int x2 = p2.s[pai(0x6d)] * 256 + p2.s[pai(0x86)];
        if (x1 - sl <= P2_LEFT || x2 - sl <= P2_LEFT) lockp1 = 1;
        else if (x2 > x1) { lockp1 = 1; p2sc = 1; }
    }
    if (p2sc) {
        uint8_t pos = p2.s[pai(0x755)], sct = p2.s[pai(0x785)], psx = p2.s[pai(0x6ff)];
        if (pos >= 0x50 && sct == 0) {
            uint8_t yy = (uint8_t)(psx - 1);
            if (!(yy & 0x80)) {
                yy++; if (yy >= 2) yy--;
                if (pos >= 0x70) yy = psx;
                uint8_t ra = a, rx = x, ry = y;
                y = yy; scroll_screen(); p2.sa = ram[0x775];
                a = ra; x = rx; y = ry;
            }
        }
    }
    if (lockp1) { p2.sv = ram[0x723]; ram[0x723] = 1; p2.slk = 1; }
    if (p2.dead || ram[0x0e] != 6) return 0;
    uint8_t lv = ram[0x75a];
    if (lv == 0 || (lv & 0x80)) return 0;
    ram[0x75a] = (uint8_t)(lv - 1);
    ram[0x86] = p2.s[pai(0x86)]; ram[0x6d] = p2.s[pai(0x6d)];
    ram[0xce] = p2.s[pai(0xce)]; ram[0xb5] = p2.s[pai(0xb5)];
    ram[0x0e] = 8; ram[0x1d] = 2; ram[0x57] = 0; ram[0x9f] = 0;
    ram[0x433] = 0; ram[0x416] = 0; ram[0x754] = 1; ram[0x756] = 0;
    ram[0x79e] = P2_INV; ram[0x723] = 0; ram[0x712] = 0; ram[0x747] = 0;
    ram[0x70b] = 0; ram[0x3c4] = 0;
    p2.slk = 0;
    return 1;
}
static void p2_collide(void) {
    uint8_t s8 = ram[8], xr = x;
    for (int i = 0; i < 5 && ram[0x0e] == 8; i++) if (ram[0x0f + i]) {
        ram[8] = (uint8_t)i; x = (uint8_t)i; player_enemy_collision();
    }
    if (ram[0x0e] == 8 && ram[0x0f + 5] && ram[0x1e + 5] >= 6) {   // power-up bawaan (slot 5)
        ram[8] = 5; x = 5; player_enemy_collision();
    }
    uint8_t m4 = ram[0x3d8 + 4], c4 = ram[0x125 + 4], m5 = ram[0x3d8 + 5], p39 = ram[0x39];
    for (int i = 0; i < MAXE && ram[0x0e] == 8; i++) {
        Ex *e = &ex[i];
        if (!e->used || !e->v[0]) continue;
        if (e->item) {
            if (e->v[2] < 6) continue;
            Ex k5; get(&k5, 5);
            put(e, 5); ram[0x3d8 + 5] = 0; ram[0x39] = e->ptype; ram[8] = 5; x = 5;
            player_enemy_collision();
            get(e, 5); put(&k5, 5);
        } else {
            Ex k4; get(&k4, 4);
            put(e, 4); ram[0x3d8 + 4] = 0; ram[8] = 4; x = 4;
            player_enemy_collision();
            get(e, 4); put(&k4, 4);
        }
    }
    ram[0x3d8 + 4] = m4; ram[0x125 + 4] = c4; ram[0x3d8 + 5] = m5; ram[0x39] = p39;
    ram[8] = s8; x = xr;
}
static void p2_run(void) {
    static int chk;
    if (p2_netmode) { chk = 1; p2.en = 1; }
    if (!chk) { chk = 1; struct stat st; p2.en = (stat("/sdcard/SMB/p2test", &st) == 0); }
    if (!p2.en) return;
    if (p2.slk) {   // pulihkan kunci kamera, batasi P1 di sisi kanan layar
        ram[0x723] = p2.sv; p2.slk = 0; if (p2.sa) { ram[0x775] = p2.sa; p2.sa = 0; }
        unsigned wx = ram[0x6d] * 256u + ram[0x86], sl = ram[0x71a] * 256u + ram[0x71c];
        if (wx > sl + 0xd0) {
            wx = sl + 0xd0; ram[0x86] = (uint8_t)wx; ram[0x6d] = (uint8_t)(wx >> 8);
            if (!(ram[0x57] & 0x80)) ram[0x57] = 0;
        }
    }
    if (p2.pend || ram[0x0e] == 2 || ram[0x0e] == 3) { memset(p2.spr, 0xf8, 32); p2.vis = 0; return; }
    p2.ring[p2.rp & 31] = ram[0x6fc]; p2.rp++;
    uint8_t inp = p2_netmode ? p2_netin : p2.ring[(uint8_t)(p2.rp - 1 - P2_DELAY) & 31];
    memset(p2.spr, 0xf8, 32); p2.vis = 0;
    int anim = 0;
    if (p2.dead) {
        if (p2.rt) p2.rt--;
        else if (ram[0x0e] == 8) { p2.dead = 0; p2.dy = 0; p2.rs = 2; }
        if (p2.dead) { if (!p2.dy) return; anim = 1; }
    }
    if (!p2.ok) {
        if (!(ram[0x0e] == 8 && ram[0x770] == 1 && ram[0x772] == 3)) return;
        p2.ok = 1; p2.rs = 1; p2.sg = p2_sig();
    }
    uint8_t p1s[64], tmp[16], bak[32];
    for (int i = 0; i < NPA; i++) p1s[i] = ram[PA[i]];
    memcpy(tmp, ram, 16);
    memcpy(bak, &ram[0x200 + P2S], 32);
    uint8_t g775 = ram[0x775], g723 = ram[0x723], gfc = ram[0xfc], gfb = ram[0xfb];
    uint8_t g712 = ram[0x712], g758 = ram[0x758], g6fc = ram[0x6fc], g6e4 = ram[0x6e4];
    uint8_t g747 = ram[0x747], g6de = ram[0x6de];
    uint8_t ra = a, rx = x, ry = y;
    for (int i = 0; i < NPA; i++) ram[PA[i]] = p2.s[i];
    if (p2.rs) {
        uint8_t k754 = ram[0x754], k756 = ram[0x756];
        if (p2.rs == 3) {   // macet: pindah saja ke dekat P1
            unsigned px = p1s[pai(0x6d)] * 256u + p1s[pai(0x86)] + 24u;
            ram[0x86] = (uint8_t)px; ram[0x6d] = (uint8_t)(px >> 8);
            ram[0xce] = p1s[pai(0xce)]; ram[0xb5] = p1s[pai(0xb5)];
            ram[0x57] = 0; ram[0x9f] = 0;
        } else {
            for (int i = 0; i < NPA; i++) ram[PA[i]] = p1s[i];
            unsigned px = ram[0x6d] * 256u + ram[0x86] + 24u;
            ram[0x86] = (uint8_t)px; ram[0x6d] = (uint8_t)(px >> 8);
            ram[0x57] = 0; ram[0x9f] = 0; ram[0x1d] = 2; ram[0x0e] = 8;
            if (p2.rs == 2) { ram[0x754] = 1; ram[0x756] = 0; ram[0x79e] = P2_INV; }
            if (p2.rs == 4) { ram[0x754] = k754; ram[0x756] = k756; }
        }
        P2LOG("SPAWN rs=%d lx=%d ly=%d size=%d task=%d\n", p2.rs, ram[0x6d]*256+ram[0x86], ram[0xce], ram[0x754], ram[0x772]);
        p2.rs = 0;
    }
    ram[0x6fc] = inp; ram[0x6e4] = P2S; ram[0x723] = 1;
    { static const uint16_t TF[6] = {0x781,0x782,0x783,0x785,0x786,0x789};
      for (int i = 0; i < 6; i++) if (ram[TF[i]]) ram[TF[i]]--; }
    control_player();
    ram[0x0d] = ram[0x0a];
    uint8_t tk = ram[0x0e];
    if (!anim && tk == 8) { p2_collide(); tk = ram[0x0e]; }
    if (tk == 0x0a) { ram[0x0e] = 8; ram[0x754] = 1; ram[0x756] = 0; ram[0x70b] = 0; tk = 8; }
    if (tk == 0x09) { ram[0x0e] = 8; ram[0x754] = 0; ram[0x70b] = 0; tk = 8; }
    if (tk == 0x0c) { ram[0x0e] = 8; ram[0x3c4] &= 0xfc; tk = 8; }
    if (tk == 3 || tk == 2) { ram[0x0e] = 8; tk = 8; }   // P2 tidak boleh masuk pipa sendiri
    int fell = (ram[0xb5] >= 2 && ram[0xb5] < 0x80);
    if (!anim && (tk == 6 || tk == 0x0b || fell)) {
        P2LOG("DIE tk=%d fell=%d ly=%d:%d lx=%d p1x=%d p1y=%d inj=%d small=%d w=%d a=%d k=%d t=%d\n", tk, fell, ram[0xb5], ram[0xce], ram[0x6d]*256+ram[0x86], p1s[pai(0x6d)]*256+p1s[pai(0x86)], p1s[pai(0xce)], ram[0x79e], ram[0x754], ram[0x75f], ram[0x760], ram[0x74e], ram[0x772]);
        p2.dead = 1; p2.rt = P2_RTIME; p2.dy = (tk == 0x0b && !fell);
        uint8_t lv = ram[0x75a];
        if (lv == 0 || (lv & 0x80)) p2.go = 1; else ram[0x75a] = (uint8_t)(lv - 1);
    } else if (anim && (tk == 6 || fell)) p2.dy = 0;
    if (!p2.dead || p2.dy) {
        memset(&ram[0x200 + P2S], 0xf8, 32);
        draw_player();
        memcpy(p2.spr, &ram[0x200 + P2S], 32);
        for (int s = 0; s < 8; s++) p2.spr[s * 4 + 2] = (uint8_t)((p2.spr[s * 4 + 2] & 0xfc) | P2_PAL);
        p2.vis = 1;
    }
    if (++p2.tk >= 21) { p2.tk = 0; if (ram[0x79e]) ram[0x79e]--; if (ram[0x79f]) ram[0x79f]--; }
    for (int i = 0; i < NPA; i++) p2.s[i] = ram[PA[i]];
    for (int i = 0; i < NPA; i++) ram[PA[i]] = p1s[i];
    memcpy(ram, tmp, 16);
    ram[0x775] = g775; ram[0x723] = g723; ram[0xfc] = gfc; ram[0xfb] = gfb;
    ram[0x712] = g712; ram[0x758] = g758; ram[0x6fc] = g6fc; ram[0x6e4] = g6e4;
    ram[0x747] = g747; ram[0x6de] = g6de;
    memcpy(&ram[0x200 + P2S], bak, 32);
    a = ra; x = rx; y = ry;
    if (p2.go) { p2.go = 0; ram[0x0e] = 6; }
}
