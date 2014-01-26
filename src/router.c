#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

#define CMD_DV      (CMD_ROUTER+1)

static int startTime;
static int time;
static int nextHeartBeat;
static int nextHeartCheck;
static int nextShow;

bool update_rt(void);

void send_dv(void) {
    szb_t *msg;
    int to;
    int tabsize;
    int i;

    if (options.sleeping) {
        return;
    }
    tabsize = num_destinations();
    msg = szb_create();
    szb_write8(msg, CMD_DV);
    szb_write32(msg, options.id);
    for (i = 0; i < tabsize; i++) {
        int dist = get_rt_distance(i);
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


void router_dropcheck(void) {
    int i;
    bool dropped = false;

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
                //con_printf("dropping someone\n");
                drop_neighbour(i);
                dropped = true;
            }
        }
    } else {
        for (i = next_neighbour(-1); i >= 0; i = next_neighbour(i)) {
            if (net_lastTime(i) + options.heartcheck < time) {
                drop_neighbour(i);
                dropped = true;
            }
        }
    }
    if (dropped) {
        update_rt();
        send_dv();
    //    tab_nhcalc();
    }
}

#if 0
void update_rt(void) {
    int ndests = num_destinations();
    for (;;) {
        bool changed = false;
        for (int via = next_neighbour(-1); via >= 0; via = next_neighbour(via)) {
            int viadist = get_rt_distance(via);
            if (!DIST_EXISTS(viadist)) {
                continue;
            }
            for (int to = 0; to < ndests; to++) {
                int newdist = get_dv_distance(via, to);
                if (!DIST_EXISTS(newdist)) {
                    continue;
                }
                newdist += viadist;
                int olddist = get_rt_distance(to);
                if (newdist < olddist || !DIST_EXISTS(olddist)) {
                    set_rt_distance(to, newdist);
                    set_rt_via(to, via);
                    changed = true;
                }
            }
        }
        if (!changed) {
            break;
        }
    }
}
#endif

bool update_rt(void) {
    int ndests = num_destinations();
    bool changed = false;
    for (int to = 0; to < ndests; to++) {
        if (to == options.id) {
            continue;
        }
        int mindist = -1;
        int minvia = -1;
        if (is_neighbour(to)) {
            mindist = get_neighbour_cost(to);
            minvia = to;
        }
        for (int via = next_neighbour(-1); via >= 0; via = next_neighbour(via)) {
            //int viadist = get_neighbour_cost(via);
            //if (!DIST_EXISTS(viadist)) {
            //    continue;
            //}
            //int newdist = get_dv_distance(via, to);
            //if (!DIST_EXISTS(newdist)) {
            //    continue;
            //}
            //newdist += viadist;
            int newdist = get_dv_distance(via, to);
            if (DIST_LESS(newdist, mindist)) {
            //if (DIST_EXISTS(newdist) && ((newdist < mindist) || !DIST_EXISTS(mindist))) {
                mindist = newdist;
                minvia = via;
            }
        }
        int olddist = get_rt_distance(to);
        if (olddist != mindist) {
            int oldvia = get_rt_via(to);
            //con_printf("changed to %d: via %d=%d -- via %d=%d\n", to, oldvia, olddist, minvia, mindist);
            changed = true;
            set_rt_distance(to, mindist);
            set_rt_via(to, minvia);

            if (options.show_dropadd) {
                if (!DIST_EXISTS(olddist)) {
                    con_printf("adding node %d\n", to);
                }
                if (!DIST_EXISTS(mindist)) {
                    con_printf("dropping node %d\n", to);
                }
            }
        }
    }
    return changed;
}

void recv_dv(szb_t *msg) {
    int via;
    int viadist;
    int nh;
    bool changed;

    assert(msg != NULL);
    via = szb_read32(msg);              /* identifica o vizinho q manda a msg */
    assert(via >= 0);                   /* certifica-se de que eh um numero valido */
    changed = add_neighbour(via);                 /* adiciona-o como vizinho se ja nao for */
    viadist = get_neighbour_cost(via);
    assert(DIST_EXISTS(viadist));
    for (;;) {
        int to, dist, curdist;

        to = szb_read32(msg);           /* atual destino */
        if (to < 0) {                   /* condicao de parada - final da mensagem */
            break;
        }
        dist = szb_read32(msg);         /* distancia do meu vizinho ateh esse destino  */
        if (DIST_EXISTS(dist)) {
            dist += viadist;
        }
        if (dist >= options.infinity) {
            dist = -1;
        }
        set_dv_distance(via, to, dist);
    }
    //if (options.id == 5) {
    //con_printf("DBG %d get from %d\n", options.id, via);
    //}
    if (update_rt()) {
        changed = true;
    }
    if (changed) {
        send_dv();
    }
}

void router_init(void) {
    startTime = time = sys_getMilli();                     /* Inicia o cronometro */
    con_runfile("autoexec.script");
    int nextHeartBeat = startTime + options.heartbeat;
    int nextHeartCheck = startTime + options.heartcheck;
    int nextShow = startTime + options.showtime;
}

void router_cleanup(void) {

}

void router_logic(void) {
    time = sys_getMilli();
    if (!options.stalled) {
        if (options.showtime > 0 && time >= nextShow) {
            nextShow = time + options.showtime;
            cmd_tabs();
        }
        if (time >= nextHeartBeat) {
            nextHeartBeat = time + options.heartbeat;
            //con_printf("sending hb\n");
            send_dv();
        }
        if (time >= nextHeartCheck) {
            nextHeartCheck = time + options.heartcheck;
            //con_printf("sending hb\n");
            router_dropcheck();
        }
    }
}

void router_readmessage(int cmd, szb_t *msg) {
    assert(msg != NULL);
    switch(cmd) {
    case CMD_DV:
        //con_printf("got DV from \n");
        //if (sender == 4) {
        //    con_printf("got DV from %d\n", sender);
        //}
        recv_dv(msg);
        break;
    default:
        con_errorf("got invalid CMD=%d\n", cmd);
        break;
    }
}

void router_heartbeat(void) {
    send_dv();
}

void router_sendmessage(int dst, szb_t *msg) {
    assert(msg != NULL);
    int via = get_rt_via(dst);
//        con_printf("sss %d %d \n", dst, via);
    if (via >= 0) {
 //       con_printf("aaa\n");
        net_send(via, msg);
    }
}

void router_restart(void) {
    net_loadLinks();
    tabs_resetconn();
    send_dv();
}
