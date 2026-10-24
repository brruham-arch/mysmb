#include <stdint.h>
#include <string.h>
#include "cpu.h"
extern void update_enemies_and_loops(void);
extern void init_enemy(void);
extern void update_powerup(void);
#define SLOT 4
#define ISLOT 5
#define NF 20
#define MAXE 48
#define SCR 0xe8
static const uint16_t FA[NF] = {0x0f,0x16,0x1e,0x46,0x58,0x6e,0x87,0xa0,0xb6,0xcf,
  0x3c5,0x401,0x417,0x434,0x491,0x49a,0x78a,0x796,0x6e5,0x3c};
typedef struct { uint8_t used, item, ptype, v[NF], box[4], spr[24]; } Ex;
static Ex ex[MAXE];

static void put(const Ex *e, int sl) { for (int i = 0; i < NF; i++) ram[FA[i] + sl] = e->v[i]; memcpy(&ram[0x4b0 + 4 * sl], e->box, 4); }
static void get(Ex *e, int sl) { for (int i = 0; i < NF; i++) e->v[i] = ram[FA[i] + sl]; memcpy(e->box, &ram[0x4b0 + 4 * sl], 4); }
static void call(Ex *e, int init) {
    int sl = e->item ? ISLOT : SLOT;
    uint8_t bak[24]; memcpy(bak, &ram[0x200 + SCR], 24);
    Ex keep; get(&keep, sl);
    uint8_t s8 = ram[8], xr = x, pt = ram[0x39];
    e->v[18] = SCR;
    put(e, sl); ram[8] = (uint8_t)sl; x = (uint8_t)sl;
    if (e->item) { ram[0x39] = e->ptype; if (!init) update_powerup(); }
    else if (init) init_enemy(); else update_enemies_and_loops();
    if (!init) memcpy(e->spr, &ram[0x200 + SCR], 24);
    get(e, sl);
    put(&keep, sl); ram[8] = s8; x = xr; ram[0x39] = pt;
    memcpy(&ram[0x200 + SCR], bak, 24);
}
static int extra_spawn_raw(uint8_t kind, uint8_t page, uint8_t px, uint8_t py) {
    for (int i = 0; i < MAXE; i++) if (!ex[i].used) {
        Ex *e = &ex[i]; memset(e, 0, sizeof *e); e->used = 1;
        memset(e->spr, 0xf8, 24);
        e->v[0] = 1; e->v[1] = kind; e->v[5] = page; e->v[6] = px; e->v[8] = 1; e->v[9] = py;
        call(e, 1);
        return i;
    }
    return -1;
}
int extra_item_spawn(uint8_t type, uint8_t page, uint8_t px, uint8_t py) {
    for (int i = 0; i < MAXE; i++) if (!ex[i].used) {
        Ex *e = &ex[i]; memset(e, 0, sizeof *e);
        e->used = 1; e->item = 1; e->ptype = type;
        memset(e->spr, 0xf8, 24);
        e->v[0] = 1; e->v[1] = 0x2e; e->v[2] = 1; e->v[5] = page; e->v[6] = px;
        e->v[8] = 1; e->v[9] = (uint8_t)(py + 16); e->v[10] = 0x20; e->v[15] = 3;
        ram[0xfe] = 2;
        return i;
    }
    return -1;
}
static int mt(uint8_t page, uint8_t px, int row) {
    if (row < 0 || row > 12) return 0;
    return ram[((page & 1) ? 0x5d0 : 0x500) + row * 16 + (px >> 4)];
}
static int solid(uint8_t v) {
    static const uint8_t thr[4] = {0x10, 0x61, 0x88, 0xc4};
    return v >= thr[v >> 6];
}
static int blocked(unsigned page, unsigned px, uint8_t py) {
    int row = ((int)py - 0x10) >> 4;
    unsigned x2 = px + 15;
    if (solid((uint8_t)mt((uint8_t)(page + (px >> 8)), (uint8_t)px, row))) return 1;
    if (solid((uint8_t)mt((uint8_t)(page + (x2 >> 8)), (uint8_t)x2, row))) return 1;
    return 0;
}
int extra_spawn(uint8_t kind, uint8_t page, uint8_t px, uint8_t py) {
    for (int k = 0; k < 10; k++) {
        unsigned xx = px + 16u * k;
        unsigned pg = page + (xx >> 8);
        if (!blocked(pg, xx & 0xff, py)) return extra_spawn_raw(kind, (uint8_t)pg, (uint8_t)xx, py);
    }
    return -1;
}
int extra_count(void) { int n = 0; for (int i = 0; i < MAXE; i++) n += (ex[i].used && !ex[i].item); return n; }
int extra_item_count(void) { int n = 0; for (int i = 0; i < MAXE; i++) n += (ex[i].used && ex[i].item); return n; }
void extra_test(void) {}
void extra_reset(void) { memset(ex, 0, sizeof ex); }
void extra_run(void) {
    static unsigned tick; tick++;
    for (int i = 0; i < MAXE; i++) if (ex[i].used) {
        Ex *e = &ex[i];
        if (e->v[16]) e->v[16]--;
        if (tick % 21 == 0 && e->v[17]) e->v[17]--;
        call(e, 0);
        if (!e->v[0]) e->used = 0;
    }
}
static uint8_t oam_bak[256];
void extra_overlay(uint8_t *oam) {
    memcpy(oam_bak, oam, 256);
    int cur = 0;
    for (int i = 0; i < MAXE; i++) if (ex[i].used)
        for (int s = 0; s < 6; s++) {
            if (ex[i].spr[s * 4] >= 0xef) continue;
            while (cur < 64 && oam[cur * 4] < 0xef) cur++;
            if (cur >= 64) return;
            memcpy(&oam[cur * 4], &ex[i].spr[s * 4], 4);
            cur++;
        }
}
void extra_restore(uint8_t *oam) { memcpy(oam, oam_bak, 256); }
