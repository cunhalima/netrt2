#ifndef INC_DEFS_H
#define INC_DEFS_H

#include <stdbool.h>

#define CMD_ORDER       1
#define CMD_ROUTER      10
#define CMD_RELIABLE    20

#define DIST_EXISTS(x) ((x) >= 0)
#define DIST_LESS(a, b) (DIST_EXISTS(a) && (((a) < (b)) || !DIST_EXISTS(b)))

#define max(a, b) (((a)>=(b))?(a):(b))
#define min(a, b) (((a)<=(b))?(a):(b))

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
void szb_clear(szb_t *buf);

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
int sys_getMilli(void);
void sys_sleep(int milli);
void sys_init(void);

/* net.c */
bool net_secretsend(int node, szb_t *msg);
bool net_send(int node, szb_t *msg);
bool net_recv(int *node, szb_t *msg);
bool net_init(void);
void net_cleanup(void);
int net_lastTime(int node);
int net_nextNode(int node);
bool net_loadLinks(void);

/* options.c */
struct options_s {
    int errRate;
    int id;
    int showtime;
    bool stalled;
    bool sleeping;
    bool show_dropadd;
    int heartbeat;
    int heartcheck;
    int infinity;
    int rtimeout;
    int rattempts;
    bool rdebug;
};
typedef struct options_s options_t;
extern options_t options;
void opt_readArgs(int argc, char **argv);

/* tables.c */
typedef struct table_s table_t;

int tab_rows(table_t *tab);
int tab_cols(table_t *tab);
table_t *tab_create(void *def, int itemsize);
void tab_free(table_t *tab);
void tab_resize(table_t *tab, int i, int j);
const void *tab_get(table_t *tab, int i, int j);
void tab_getCopy(table_t *tab, int i, int j, void *data);
void tab_set(table_t *tab, int i, int j, const void *item);

/* alltabs.c */
void tabs_init(void);
void tabs_cleanup(void);
bool is_neighbour(int node);
void drop_neighbour(int node);
int next_neighbour(int node);
void set_neighbour_cost(int node, int cost);
int get_neighbour_cost(int node);
void set_node_inseqno(int node, int seqno);
int get_node_inseqno(int node);
void set_node_outseqno(int node, int seqno);
int get_node_outseqno(int node);
void set_rt_distance(int node, int dist);
int get_rt_distance(int node);
void set_rt_via(int node, int via);
int get_rt_via(int node);
void set_dv_distance(int via, int to, int dist);
int get_dv_distance(int via, int to);
int num_destinations(void);
bool add_neighbour(int node);
void print_tabs(void);
void tabs_resetconn(void);

/* router.c */
void router_init(void);
void router_cleanup(void);
void router_logic(void);
void router_readmessage(int cmd, szb_t *msg);
void router_heartbeat(void);
void router_dropcheck(void);
void router_sendmessage(int dst, szb_t *msg);
void router_restart(void);

/* reliable.c */
void rbl_init(void);
void rbl_cleanup(void);
void rbl_readmessage(int cmd, szb_t *msg);
void rbl_sendmessage(int dst, const char *text);
void rbl_logic(void);

/* command.c */
void cmd_init(void);
void cmd_cleanup(void);
void cmd_order(const char *text);
void cmd_tabs(void);


#endif

