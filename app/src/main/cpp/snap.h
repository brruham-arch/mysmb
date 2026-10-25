#include <stdbool.h>
#include "video.h"
extern bool controller1_strobe, controller2_strobe;
extern uint8_t controller1_btn_index, controller2_btn_index;
// snap.h - savestate lengkap (dipakai tes round-trip & resync)
#include <string.h>
#include <stdint.h>
extern int  extra_snap_size(void);
extern int  extra_snap_save(uint8_t *p);
extern void extra_snap_load(const uint8_t *p);

#define SP_(v) do{ memcpy(p,&(v),sizeof(v)); p+=sizeof(v);}while(0)
#define SG_(v) do{ memcpy(&(v),p,sizeof(v)); p+=sizeof(v);}while(0)

static int snap_size(void){
  int n = 2048 + (int)sizeof(video)
    + (int)(sizeof(a)+sizeof(x)+sizeof(y)+sizeof(sp))
    + (int)(sizeof(carry_flag)+sizeof(zero_flag)+sizeof(neg_flag)
           +sizeof(overflow_flag)+sizeof(interrupt_disabled)+sizeof(decimal_flag))
    + (int)(sizeof(cpu_resume_pc)+sizeof(cpu_dispatch_target))
    + (int)(3*sizeof(controller1_state)) * 2
    + (int)(sizeof(ibl)+sizeof(ipr)+sizeof(itb)+sizeof(iphys)+sizeof(iprev)+sizeof(ifc));
  return n + extra_snap_size();
}
static int snap_save(uint8_t *buf){
  uint8_t *p = buf;
  memcpy(p, ram, 2048); p += 2048;
  SP_(video);
  SP_(a); SP_(x); SP_(y); SP_(sp);
  SP_(carry_flag); SP_(zero_flag); SP_(neg_flag);
  SP_(overflow_flag); SP_(interrupt_disabled); SP_(decimal_flag);
  SP_(cpu_resume_pc); SP_(cpu_dispatch_target);
  SP_(controller1_state); SP_(controller1_strobe); SP_(controller1_btn_index);
  SP_(controller2_state); SP_(controller2_strobe); SP_(controller2_btn_index);
  SP_(ibl); SP_(ipr); SP_(itb); SP_(iphys); SP_(iprev); SP_(ifc);
  p += extra_snap_save(p);
  return (int)(p - buf);
}
static void snap_load(const uint8_t *buf){
  const uint8_t *p = buf;
  memcpy(ram, p, 2048); p += 2048;
  SG_(video);
  SG_(a); SG_(x); SG_(y); SG_(sp);
  SG_(carry_flag); SG_(zero_flag); SG_(neg_flag);
  SG_(overflow_flag); SG_(interrupt_disabled); SG_(decimal_flag);
  SG_(cpu_resume_pc); SG_(cpu_dispatch_target);
  SG_(controller1_state); SG_(controller1_strobe); SG_(controller1_btn_index);
  SG_(controller2_state); SG_(controller2_strobe); SG_(controller2_btn_index);
  SG_(ibl); SG_(ipr); SG_(itb); SG_(iphys); SG_(iprev); SG_(ifc);
  extra_snap_load(p);
}

#include <errno.h>
static int serr;
static uint8_t sbuf[16384];
static uint32_t hp[600], sdh, sH1;
static int stest = -1, sphase, sdiv, ssz;
static void snap_test(void) {
    if (stest < 0) {
        struct stat st;
        stest = (stat("/sdcard/SMB/snaptest", &st) == 0) ? 1 : 0;
        if (!stest) serr = errno;
    }
    if (dcount == 100) { FILE *g=fopen("/sdcard/SMB/snapdbg.txt","w"); if(g){fprintf(g,"stest=%d dmode=%d dcount=%d size=%d err=%d\n",stest,dmode,dcount,snap_size(),serr); fclose(g);} }
    if (!stest) return;
    if (sphase == 0 && dmode == 0 && dcount == 600) {
        ssz = snap_size();
        if (ssz > (int)sizeof(sbuf)) { stest = 0; return; }
        snap_save(sbuf); sdh = dh; sphase = 1;
    } else if (sphase == 1) {
        if (dcount > 600 && dcount <= 1200) hp[dcount - 601] = dh;
        if (dcount == 1200) {
            sH1 = dh; dlen = 1200;
            snap_load(sbuf); dcount = 600; dh = sdh; dmode = 1; sphase = 2;
        }
    } else if (sphase == 2) {
        if (dcount > 600 && dcount <= 1200 && !sdiv && dh != hp[dcount - 601]) sdiv = dcount;
        if (dcount == 1200) {
            FILE *f = fopen("/sdcard/SMB/snap.txt", "w");
            if (f) {
                fprintf(f, "size=%d H1=%08x H2=%08x diverge_frame=%d %s\n",
                        ssz, sH1, (unsigned)dh, sdiv, dh == sH1 ? "OK" : "GAGAL");
                fclose(f);
            }
            dmode = 0; sphase = 3;
        }
    }
}
