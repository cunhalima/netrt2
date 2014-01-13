#ifndef INC_DEFS_H
#define INC_DEFS_H

#include <stdbool.h>

#define DIST_EXISTS(x) ((x) >= 0)

/* szb.c */
struct szb_s;
typedef struct szb_s szb_t;
szb_t *szb_create();
void szb_free(szb_t *buf);
void szb_write8(szb_t *buf, int v);
void szb_write16(szb_t *buf, int v);
void szb_write32(szb_t *buf, int v);
void szb_writeCStr(szb_t *buf, const char *str);
int szb_read8(szb_t *buf);
int szb_read16(szb_t *buf);
int szb_read32(szb_t *buf);
void szb_print(szb_t *buf);
void szb_rewind(szb_t *buf);
void szb_setCapacity(szb_t *buf, int capacity);
void szb_resize(szb_t *buf, int size);
void *szb_getPtr(szb_t *buf);
void *szb_getPosPtr(szb_t *buf);
int szb_getSize(szb_t *buf);
bool szb_eof(szb_t *buf);

/* console.c */
typedef void (*cmdparserfn_t)(const char *);
bool con_init();
bool con_cleanup();
void con_print(const char *msg);
void con_printf(const char *fmt, ...);
void con_error(const char *msg);
void con_errorf(const char *fmt, ...);
void con_checkInput();
bool con_isRunning();
void con_register(cmdparserfn_t fn);
void con_runfile(const char *path);

/* timer.c */
int sys_getMilli();
void sys_sleep(int milli);

/* net.c */
bool net_secretsend(int node, szb_t *msg);
bool net_send(int node, szb_t *msg);
bool net_recv(int *node, szb_t *msg);
bool net_init(void);
void net_cleanup(void);
int net_lastTime(int node);
int net_nextNode(int node);

/* options.c */
struct options_s {
    int errRate;
    int id;
    int showtime;
    bool stalled;
    bool sleeping;
};
typedef struct options_s options_t;
extern options_t options;
void opt_readArgs(int argc, char **argv);

/* tables.c */
void tab_init(void);
void tab_cleanup(void);
int tab_dvget(int to);
void tab_dvset(int to, int value);
void tab_dvprint(void);
void tab_dvvalidate(void);
bool tab_dvchanged(void);
int tab_dvsize(void);
int tab_nhget(int to);
void tab_nhset(int to, int value);
void tab_nhprint(void);
int tab_nhsize(void);
bool is_neighbour(int node);
void tab_dropnode(int node);
int next_neighbour(int node);

int tab_fdget(int to);
void tab_fdset(int to, int value);
void tab_fdprint(void);
int tab_fdsize(void);

bool tab_dnget(int to);
void tab_dnset(int to, bool value);
void tab_dnprint(void);
int tab_dnsize(void);

void tab_allprint(void);

#endif
