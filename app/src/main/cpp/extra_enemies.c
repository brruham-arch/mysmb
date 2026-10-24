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
int extra_spawn(uint8_t kind, uint8_t page, uint8_t px, uint8_t py) {
    for (int i = 0; i < MAXE; i++) if (!ex[i].used) {
        Ex *e = &ex[i]; memset(e, 0, sizeof *e); e->used = 1;
        e->v[0] = 1; e->v[1] = kind; e->v[5] = page; e->v[6] = px; e->v[8] = 1; e->v[9] = py;
        e->v[18] = 0xe8;
        call(e, 1); e->v[18] = 0xe8;
        return i;
    }
    return -1;
}
void extra_test(void) {
    static unsigned n; if (++n % 90) return;
    unsigned xx = ram[0x86] + 120;
    extra_spawn(0x06, (uint8_t)(ram[0x6d] + (xx >> 8)), (uint8_t)xx, 0xb0);
}
static int last_n;
void extra_run(void) {
    for (int k = 0; k < last_n; k++)
        for (int j = 0; j < 24; j += 4) ram[0x200 + 0xe8 - 24 * k + j] = 0xf8;
    int n = 0;
    for (int i = 0; i < MAXE; i++) if (ex[i].used) {
        ex[i].v[18] = (uint8_t)(0xe8 - 24 * (n < 8 ? n : 7)); n++;
        call(&ex[i], 0);
        if (!ex[i].v[0]) ex[i].used = 0;
    }
    last_n = n < 8 ? n : 8;
}
int extra_count(void) { int n = 0; for (int i = 0; i < MAXE; i++) n += ex[i].used; return n; }
