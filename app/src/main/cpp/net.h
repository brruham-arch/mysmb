// net.h - lockstep TCP: host = P1 (Mario), client = P2 (Luigi). Di-include dari extra_enemies.c
#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#define NET_D 3
#define NET_PORT 7777
static volatile int net_st, net_runf;
static int net_role, net_fd = -1, net_lfd = -1, net_bad;
static uint32_t net_f, net_sent = 0xffffffffu, nrnext;
static uint8_t nlin[1024], nrin[1024], nrbuf[64];
static int nrlen;
static uint32_t nlh[32], nlhf[32], nrh[32], nrhf[32];

static void net_log(const char *m, uint32_t f) {
    FILE *g = fopen("/sdcard/SMB/net.txt", "a");
    if (g) { fprintf(g, "%s f=%u\n", m, f); fclose(g); }
}
static uint8_t rev8(uint8_t v) {
    uint8_t r = 0;
    for (int i = 0; i < 8; i++) if (v & (1 << i)) r |= (uint8_t)(0x80 >> i);
    return r;
}
static void net_send(const uint8_t *p, int n) {
    if (net_fd >= 0 && send(net_fd, p, (size_t)n, MSG_NOSIGNAL) != n) net_st = 3;
}
static void net_finish(int fd) {
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    struct timeval tv; tv.tv_sec = 4; tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    char m[4] = {'S','M','B','1'}, g[4] = {0, 0, 0, 0};
    int ok = send(fd, m, 4, MSG_NOSIGNAL) == 4 && recv(fd, g, 4, MSG_WAITALL) == 4 && memcmp(m, g, 4) == 0;
    if (!ok) { close(fd); net_st = 3; return; }
    tv.tv_sec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    net_fd = fd; net_st = 2;
}
static void *net_host_th(void *arg) {
    (void)arg;
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    struct sockaddr_in sa; memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET; sa.sin_port = htons(NET_PORT); sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(lfd, (struct sockaddr *)&sa, sizeof sa) < 0 || listen(lfd, 1) < 0) { close(lfd); net_st = 3; return NULL; }
    net_lfd = lfd;
    int fd = accept(lfd, NULL, NULL);
    if (net_lfd == lfd) { close(lfd); net_lfd = -1; }
    if (fd < 0) { if (net_st == 1) net_st = 3; return NULL; }
    net_finish(fd);
    return NULL;
}
static void *net_join_th(void *arg) {
    char ip[64];
    snprintf(ip, sizeof ip, "%s", (const char *)arg);
    free(arg);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval tv; tv.tv_sec = 6; tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
    struct sockaddr_in sa; memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET; sa.sin_port = htons(NET_PORT);
    if (inet_pton(AF_INET, ip, &sa.sin_addr) <= 0 || connect(fd, (struct sockaddr *)&sa, sizeof sa) < 0) {
        close(fd); net_st = 3; return NULL;
    }
    net_finish(fd);
    return NULL;
}
JNIEXPORT void JNICALL Java_com_brruham_smb_Core_netHost(JNIEnv *e, jclass c) {
    (void)e; (void)c;
    net_role = 1; net_st = 1;
    pthread_t t; pthread_create(&t, NULL, net_host_th, NULL); pthread_detach(t);
}
JNIEXPORT void JNICALL Java_com_brruham_smb_Core_netJoin(JNIEnv *e, jclass c, jstring ip) {
    (void)c;
    const char *p = (*e)->GetStringUTFChars(e, ip, NULL);
    char *d = strdup(p);
    (*e)->ReleaseStringUTFChars(e, ip, p);
    net_role = 2; net_st = 1;
    pthread_t t; pthread_create(&t, NULL, net_join_th, d); pthread_detach(t);
}
JNIEXPORT jint JNICALL Java_com_brruham_smb_Core_netState(JNIEnv *e, jclass c) { (void)e; (void)c; return net_st; }
JNIEXPORT void JNICALL Java_com_brruham_smb_Core_netStop(JNIEnv *e, jclass c) {
    (void)e; (void)c;
    if (net_lfd >= 0) { shutdown(net_lfd, SHUT_RDWR); close(net_lfd); net_lfd = -1; }
    if (!net_runf) { if (net_fd >= 0) { close(net_fd); net_fd = -1; } net_st = 0; }
}
JNIEXPORT void JNICALL Java_com_brruham_smb_Core_netStart(JNIEnv *e, jclass c) {
    (void)e; (void)c;
    net_f = 0; net_sent = 0xffffffffu; nrnext = NET_D; nrlen = 0; net_bad = 0;
    memset(nlin, 0, sizeof nlin); memset(nrin, 0, sizeof nrin);
    p2_netmode = 1; net_runf = 1;
}
int net_on(void) { return net_runf; }
static void net_hash_in(uint32_t f, uint32_t h) {
    int i = (int)((f / 60) & 31);
    nrh[i] = h; nrhf[i] = f;
    if (nlhf[i] == f && nlh[i] != h && !net_bad) { net_bad = 1; net_log("DESYNC", f); }
}
static void net_pump(void) {
    if (net_fd < 0) return;
    for (;;) {
        ssize_t r = recv(net_fd, nrbuf + nrlen, sizeof nrbuf - (size_t)nrlen, MSG_DONTWAIT);
        if (r == 0) { if (net_st != 3) net_log("DISCONNECT", net_f); net_st = 3; break; }
        if (r < 0) { if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) net_st = 3; break; }
        nrlen += (int)r;
        int o = 0;
        while (o < nrlen) {
            if (nrbuf[o] == 'I') {
                if (nrlen - o < 6) break;
                uint32_t f; memcpy(&f, nrbuf + o + 1, 4);
                nrin[f & 1023] = nrbuf[o + 5];
                if (f + 1 > nrnext) nrnext = f + 1;
                o += 6;
            } else if (nrbuf[o] == 'H') {
                if (nrlen - o < 9) break;
                uint32_t f, h; memcpy(&f, nrbuf + o + 1, 4); memcpy(&h, nrbuf + o + 5, 4);
                net_hash_in(f, h);
                o += 9;
            } else o = nrlen;
        }
        memmove(nrbuf, nrbuf + o, (size_t)(nrlen - o));
        nrlen -= o;
    }
}
int net_begin(uint32_t raw, uint8_t *m1out) {
    net_pump();
    if (net_st != 2) return 0;
    uint32_t f = net_f;
    if (net_sent != f) {
        uint8_t l = (uint8_t)(raw & 0xff);
        if ((raw & 0x100) && ((f / 3) & 1) == 0) l |= 1;
        if ((raw & 0x200) && ((f / 3) & 1) == 0) l |= 2;
        nlin[(f + NET_D) & 1023] = l;
        uint8_t pk[6]; uint32_t ff = f + NET_D;
        pk[0] = 'I'; memcpy(pk + 1, &ff, 4); pk[5] = l;
        net_send(pk, 6);
        net_sent = f;
    }
    if (nrnext <= f) return 0;
    uint8_t mh = (net_role == 1) ? nlin[f & 1023] : nrin[f & 1023];
    uint8_t mc = (net_role == 1) ? nrin[f & 1023] : nlin[f & 1023];
    if (ram[0x770] == 0 && f >= 90 && (f % 48) < 3) mh |= 0x08;
    mc &= (uint8_t)~0x0c;
    p2_netin = rev8(mc);
    *m1out = mh;
    return 1;
}
void net_end(void) {
    uint32_t f = net_f++;
    if (f % 60 == 59) {
        uint32_t h = 2166136261u;
        for (int k = 0; k < 2048; k++) h = (h ^ ram[k]) * 16777619u;
        int i = (int)((f / 60) & 31);
        nlh[i] = h; nlhf[i] = f;
        uint8_t pk[9];
        pk[0] = 'H'; memcpy(pk + 1, &f, 4); memcpy(pk + 5, &h, 4);
        net_send(pk, 9);
        if (nrhf[i] == f && nrh[i] != h && !net_bad) { net_bad = 1; net_log("DESYNC", f); }
    }
}
