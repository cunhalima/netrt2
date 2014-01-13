#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

#define CMD_DV      100
#define CMD_CHAT    101

// DEBUG
#define CMD_ORDER   102

#define HEARTBEAT   2000
#define HEARTCHECK  (2*HEARTBEAT)

#define INFINITY    100

static int startTime;
static int time;

void check_dropped(void) {
    int i;
    //bool dropped = false;

    if (options.sleeping) {
        return;
    }
    if (options.stalled) {          /* the stalled algorithm is different */
        int largest_time = -1;
        int base_time = -1;
        for (i = next_neighbour(-1); i >= 0; i = next_neighbour(i)) {
            int thistime = net_lastTime(i);
            if (thistime > largest_time) {
                base_time = largest_time;
                largest_time = thistime;
            } else if (thistime > base_time) {
                base_time = thistime;
            }
        }
        // base_time deve ser o segundo maior tempo
        base_time = largest_time;
        base_time -= 1000;              /* one second behind */
        for (i = next_neighbour(-1); i >= 0; i = next_neighbour(i)) {
            if (net_lastTime(i) < base_time) {
                tab_dropnode(i);
            }
        }
        return;
    }
    
    for (i = next_neighbour(-1); i >= 0; i = next_neighbour(i)) {
        if (net_lastTime(i) + HEARTCHECK < time) {
            tab_dropnode(i);
        }
    }
    //if (dropped) {
    //    tab_nhcalc();
    // }
}

void send_dv(void) {
    szb_t *msg;
    int to;
    int tabsize;
    int i;

    if (options.sleeping) {
        return;
    }
    tabsize = tab_dvsize();
    msg = szb_create();
    szb_write8(msg, CMD_DV);
    szb_write32(msg, options.id);
    for (i = 0; i < tabsize; i++) {
        int dist = tab_dvget(i);
        //if (dist < 0) {
        //    continue;
        //}
        szb_write32(msg, i);
        szb_write32(msg, dist);
    }
    szb_write32(msg, -1);
    for (to = next_neighbour(-1); to >= 0; to = next_neighbour(to)) {
        net_send(to, msg);
    }
    //net_send(
    szb_free(msg);
    //tab_dvvalidate();
}

void add_neighbour(int n) {
    int dist;

    dist = tab_fdget(n);
    if (DIST_EXISTS(dist)) {
        tab_dnset(n, false);
        int currdist = tab_dvget(n);
        if (!DIST_EXISTS(currdist) || (dist < currdist)) {
            tab_dvset(n, dist);
            tab_nhset(n, n);
        }
    }
}

void recv_dv(szb_t *msg) {
    int via;
    int viadist;
    int nh;

    assert(msg != NULL);
    via = szb_read32(msg);              /* identifica o vizinho q manda a msg */
    assert(via >= 0);                   /* certifica-se de que eh um numero valido */
    add_neighbour(via);                 /* adiciona-o como vizinho se ja nao for */
    viadist = tab_dvget(via);           /* pega a atual menor distancia ateh esse vizinho */
    nh = tab_nhget(via);                /* pega o atual "proximo nodo" na menor jornada ateh o vizinho (pode ser que nao seja diretamente por ele */
    for (;;) {
        int to, dist, curdist;

        to = szb_read32(msg);           /* atual destino */
        if (to < 0) {                   /* condicao de parada - final da mensagem */
            break;
        }
        dist = szb_read32(msg);         /* distancia do meu vizinho ateh esse destino  */
        if (to == options.id) {         /* nao da bola qdo destinatario sou eu */
            continue;
        }
        if (to == via) {                /* nao da bola qdo destinatario eh ele mesmo */
            continue;
        }
        int currdist = tab_dvget(to);   /* pega a atual menor distancia entre eu e o destino */
        if (DIST_EXISTS(dist)) {        /* se a distancia q o vizinho ta mandando nao eh infinita, */
            dist += viadist;            /*  entao acrescenta a ela a menor distancia entre eu e o vizinho */
        }
        if (dist >= INFINITY) {
            dist = -1;
        }
        // minha atual maneira de chegar a to eh por via? entao atualiza sempre
        if (tab_nhget(to) == via) {
            tab_dvset(to, dist);
            if (!DIST_EXISTS(dist)) {
                tab_nhset(to, -1);
            }
        } else
        if (DIST_EXISTS(dist)) {
            if (!DIST_EXISTS(currdist) || dist < currdist) {
                tab_dvset(to, dist);
                tab_nhset(to, nh);
            }
        }
    }
}

static void cmd_set(const char *args) {
    int to, via, value;

    if (sscanf(args, "%d %d %d", &to, &via, &value) != 3) {
        con_error("usage: set <to> <via> <value>\n");
        return;
    }
    //tab_tvset(to, via, value);
}

static void cmd_showtime(const char *args) {
    if (sscanf(args, "%d", &options.showtime) != 1) {
        con_error("usage: showtime <milliseconds>\n");
    }
}

static void cmd_drop(const char *args) {
    int node;

    if (sscanf(args, "%d", &node) != 1) {
        con_error("usage: drop <node>\n");
        return;
    }
    tab_dropnode(node);
    //tab_nhcalc();
}

static void cmd_tabs(void) {
    //tab_tvprint();
    //tab_dvprint();
    //tab_nhprint();
    tab_allprint();
}

static void cmd_stall(void) {
    options.stalled = true;
    con_printf("algorithm stalled\n");
}

static void cmd_all(const char *args);

static void cmd_heartbeat(void) {
    send_dv();
    if (!options.sleeping) {
        con_printf("heartbeat sent\n");
    }
}

static void cmd_heartcheck(void) {
    check_dropped();
    if (!options.sleeping) {
        con_printf("heartcheck done\n");
    }
}

static void cmd_sleep(void) {
    options.sleeping = true;
    con_printf("sleeping\n");
}

static void cmd_wake(void) {
    options.sleeping = false;
    con_printf("waking up\n");
}

static void cmdparser(const char *text) {
    if (text[0] == '/') {
        text++;
        if ((strcmp(text, "tabs") == 0) ||
            (strcmp(text, "t") == 0)) {
            cmd_tabs();
        } else if (strncmp(text, "set ", 4) == 0) {
            cmd_set(&text[4]);
        } else if (strncmp(text, "drop ", 5) == 0) {
            cmd_drop(&text[5]);
        } else if (strncmp(text, "showtime ", 9) == 0) {
            cmd_showtime(&text[9]);
        } else if (strncmp(text, "all ", 4) == 0) {
            cmd_all(&text[4]);
        } else if (strcmp(text, "stall") == 0) {
            cmd_stall();
        } else if (strcmp(text, "hb") == 0) {
            cmd_heartbeat();
        } else if (strcmp(text, "hc") == 0) {
            cmd_heartcheck();
        } else if (strcmp(text, "sleep") == 0) {
            cmd_sleep();
        } else if (strcmp(text, "wake") == 0) {
            cmd_wake();
        } else {
            con_errorf("unknown command: %s\n", text);
        }
    } else {
        szb_t *msg;
        int to;

        con_printf("you say: <%s>\n", text);
        
        msg = szb_create();
        szb_write8(msg, CMD_CHAT);
        szb_writeCStr(msg, text);
        for (to = next_neighbour(-1); to >= 0; to = next_neighbour(to)) {
            net_send(to, msg);
        }
        //net_send(
        szb_free(msg);
    }
}

static void cmd_order(const char *text) {
    cmdparser(text);
}

void cmd_all(const char *args) {
    szb_t *msg;
    int to;

    msg = szb_create();
    szb_write8(msg, CMD_ORDER);
    szb_writeCStr(msg, args);
    for (to = net_nextNode(-1); to >= 0; to = net_nextNode(to)) {
        //if (to == options.id) {
        //    continue;
        //}
        net_secretsend(to, msg);
    }
    szb_free(msg);
    //cmd_order(args);
}

int main(int argc, char **argv) {
    szb_t *msg;

    startTime = time = sys_getMilli();                     /* Inicia o cronometro */
    opt_readArgs(argc, argv);
    tab_init();
    con_init();
    con_register(cmdparser);
    con_printf("id=%d\n", options.id);
    if (!net_init()) {
        con_cleanup();
        tab_cleanup();
        return -1;
    }
    msg = szb_create();
    con_runfile("autoexec.script");
    int nextHeartBeat = startTime + HEARTBEAT;
    int nextHeartCheck = startTime + HEARTCHECK;
    int nextShow = startTime + options.showtime;
    while (con_isRunning()) {
        int sender;

        while (net_recv(&sender, msg)) {
            int cmd = szb_read8(msg);
            switch(cmd) {
            case CMD_DV:
                //if (sender == 4) {
                //    con_printf("got DV from %d\n", sender);
                //}
                recv_dv(msg);
                break;
            case CMD_CHAT:
                con_printf("got from %d <%s>\n", sender, szb_getPosPtr(msg));
                break;
            case CMD_ORDER:
                cmd_order(szb_getPosPtr(msg));
                //con_printf("got order!!!\n");
                break;
            default:
                con_errorf("got invalid CMD=%d\n", cmd);
                break;
            }
        }
        con_checkInput();
        time = sys_getMilli();
        if (!options.stalled) {
            if (options.showtime > 0 && time >= nextShow) {
                nextShow = time + options.showtime;
                cmd_tabs();
            }
            if (time >= nextHeartBeat) {
                nextHeartBeat = time + HEARTBEAT;
                //con_printf("sending hb\n");
                send_dv();
            }
            if (time >= nextHeartCheck) {
                nextHeartCheck = time + HEARTCHECK;
                //con_printf("sending hb\n");
                check_dropped();
            }
        }
        sys_sleep(20);
    }
    szb_free(msg);
    net_cleanup();
    con_cleanup();
    tab_cleanup();
    return 0;
}
