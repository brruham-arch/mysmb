#include <stdint.h>
#include <string.h>
#include "cpu.h"
extern void update_enemies_and_loops(void);
extern void init_enemy(void);
#define SLOT 4
#define NF 20
#define MAXE 32
static const uint16_t FA[NF] = {0x0f,0x16,0x1e,0x46,0x58,0x6e,0x87,0xa0,0xb6,0xcf,
  0x3c5,0x401,0x417,0x434,0x491,0x49a,0x78a,0x796,0x6e5,0x3c};
#define BOX (0x4b0 + 4 * SLOT)
typedef struct { uint8_t used, v[NF], box[4]; } Ex;
static Ex ex[MAXE];
static void put(const Ex *e) { for (int i = 0; i < NF; i++) ram[FA[i] + SLOT] = e->v[i]; memcpy(&ram[BOX], e->box, 4); }
static void get(Ex *e) { for (int i = 0; i < NF; i++) e->v[i] = ram[FA[i] + SLOT]; memcpy(e->box, &ram[BOX], 4); }
static uint8_t oam_pool = 0xe8;
static uint8_t oam_alloc(void) { uint8_t o = oam_pool; oam_pool = (oam_pool < 0x40 + 24) ? 0xe8 : (uint8_t)(oam_pool - 24); return o; }
static void call(Ex *e, int init) {
    Ex keep; get(&keep);
    uint8_t s8 = ram[8], xr = x;
    put(e); ram[8] = SLOT; x = SLOT;
    if (init) init_enemy(); else update_enemies_and_loops();
    get(e);
    put(&keep); ram[8] = s8; x = xr;
}
static int extra_spawn_raw(uint8_t kind, uint8_t page, uint8_t px, uint8_t py) {
    for (int i = 0; i < MAXE; i++) if (!ex[i].used) {
        Ex *e = &ex[i]; memset(e, 0, sizeof *e); e->used = 1;
        e->v[0] = 1; e->v[1] = kind; e->v[5] = page; e->v[6] = px; e->v[8] = 1; e->v[9] = py;
        e->v[18] = 0xe8;
        call(e, 1); e->v[18] = 0xe8;
        return i;
    }
    return -1;
}
int extra_count(void) { int n = 0; for (int i = 0; i < MAXE; i++) n += ex[i].used; return n; }
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
void extra_test(void) {
    static unsigned n; if (++n % 90) return;
    unsigned xx = ram[0x86] + 120;
    extra_spawn(0x06, (uint8_t)(ram[0x6d] + (xx >> 8)), (uint8_t)xx, 0xb0);
}
static uint8_t prev[MAXE];
static int last_n;
void extra_reset(void) { memset(ex, 0, sizeof ex); last_n = 0; }
static int free_block(void) {
    for (int o = 0xe8; o >= 0; o -= 4) {
        int ok = 1;
        for (int s = 0; s < 6; s++) if (ram[0x200 + o + 4 * s] < 0xef) { ok = 0; break; }
        if (ok) return o;
    }
    return -1;
}
void extra_run(void) {
    static unsigned tick; tick++;
    for (int k = 0; k < last_n; k++)
        for (int s = 0; s < 6; s++) ram[0x200 + prev[k] + 4 * s] = 0xf8;
    int n = 0;
    for (int i = 0; i < MAXE; i++) if (ex[i].used) {
        int o = free_block();
        if (o < 0) continue;
        ex[i].v[18] = (uint8_t)o;
        if (ex[i].v[16]) ex[i].v[16]--;
        if (tick % 21 == 0 && ex[i].v[17]) ex[i].v[17]--;
        call(&ex[i], 0);
        if (!ex[i].v[0]) ex[i].used = 0;
        prev[n++] = (uint8_t)o;
    }
    last_n = n;
}
