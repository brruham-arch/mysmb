// extra_p2.h - Pemain 2 lokal (tahap 3a), di-include dari extra_enemies.c
#include <sys/stat.h>
extern void control_player(void);
extern void draw_player(void);
#define P2S 0xe0      // scratch OAM 32 byte
#define P2_DELAY 20   // tes: P2 meniru input P1 dengan jeda frame
#define P2_PAL 1      // palet sprite P2 (0 = sama dengan P1)
static const uint16_t PA[] = {
  0x0a,0x0b,0x0c,0x0d,0x0e,0x1d,0x33,0x45,0x57,0x6d,0x86,0x9f,0xb5,0xce,
  0x3ad,0x3b8,0x3c4,0x3d0,0x416,0x433,0x450,0x456,0x490,0x499,
  0x4ac,0x4ad,0x4ae,0x4af,
  0x6d5,0x6db,0x6ff,0x700,0x701,0x702,0x703,0x704,0x705,0x706,0x707,0x708,
  0x709,0x70a,0x70b,0x70c,0x70d,0x70e,0x711,0x714,0x754,0x755,0x756,
  0x781,0x782,0x783,0x785,0x786,0x789,0x79e,0x79f };
#define NPA ((int)(sizeof(PA) / sizeof(PA[0])))
typedef char p2_chk[(sizeof(PA) / sizeof(PA[0])) <= 64 ? 1 : -1];
typedef struct { uint8_t s[64], ring[32], spr[32], rp, tk, en, ok, rs, vis; } P2;
static P2 p2;
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
static void p2_run(void) {
    static int chk;
    if (!chk) { chk = 1; struct stat st; p2.en = (stat("/sdcard/SMB/p2test", &st) == 0); }
    if (!p2.en) return;
    uint8_t p1s[64], tmp[16], bak[32];
    for (int i = 0; i < NPA; i++) p1s[i] = ram[PA[i]];
    memcpy(tmp, ram, 16);
    memcpy(bak, &ram[0x200 + P2S], 32);
    uint8_t g775 = ram[0x775], g723 = ram[0x723], gfc = ram[0xfc], gfb = ram[0xfb];
    uint8_t g712 = ram[0x712], g758 = ram[0x758], g6fc = ram[0x6fc], g6e4 = ram[0x6e4];
    uint8_t ra = a, rx = x, ry = y;
    p2.ring[p2.rp & 31] = g6fc; p2.rp++;
    uint8_t inp = p2.ring[(uint8_t)(p2.rp - 1 - P2_DELAY) & 31];
    if (!p2.ok) { p2.ok = 1; p2.rs = 1; }
    for (int i = 0; i < NPA; i++) ram[PA[i]] = p2.s[i];
    if (p2.rs) {
        p2.rs = 0;
        for (int i = 0; i < NPA; i++) ram[PA[i]] = p1s[i];
        unsigned px = ram[0x6d] * 256u + ram[0x86] + 24u;
        ram[0x86] = (uint8_t)px; ram[0x6d] = (uint8_t)(px >> 8);
        ram[0x57] = 0; ram[0x9f] = 0; ram[0x1d] = 2; ram[0x0e] = 8;
    }
    { static const uint16_t TF[6] = {0x781,0x782,0x783,0x785,0x786,0x789};
      for (int i = 0; i < 6; i++) if (ram[TF[i]]) ram[TF[i]]--; }
    ram[0x6fc] = inp; ram[0x6e4] = P2S; ram[0x723] = 1;
    control_player();
    int dead = (ram[0x0e] == 6 || ram[0x0e] == 0xb || (ram[0xb5] >= 2 && ram[0xb5] < 0x80));
    memset(p2.spr, 0xf8, 32);
    if (!dead) {
        memset(&ram[0x200 + P2S], 0xf8, 32);
        draw_player();
        memcpy(p2.spr, &ram[0x200 + P2S], 32);
        for (int s = 0; s < 8; s++) p2.spr[s * 4 + 2] = (uint8_t)((p2.spr[s * 4 + 2] & 0xfc) | P2_PAL);
    } else p2.rs = 1;
    p2.vis = !dead;
    if (++p2.tk >= 21) { p2.tk = 0; if (ram[0x79e]) ram[0x79e]--; if (ram[0x79f]) ram[0x79f]--; }
    for (int i = 0; i < NPA; i++) p2.s[i] = ram[PA[i]];
    for (int i = 0; i < NPA; i++) ram[PA[i]] = p1s[i];
    memcpy(ram, tmp, 16);
    ram[0x775] = g775; ram[0x723] = g723; ram[0xfc] = gfc; ram[0xfb] = gfb;
    ram[0x712] = g712; ram[0x758] = g758; ram[0x6fc] = g6fc; ram[0x6e4] = g6e4;
    memcpy(&ram[0x200 + P2S], bak, 32);
    a = ra; x = rx; y = ry;
}
