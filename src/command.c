#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

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
    drop_neighbour(node);
    //tab_nhcalc();
}

void cmd_tabs(void) {
    //tab_tvprint();
    //tab_dvprint();
    //tab_nhprint();
    //tab_allprint();
    print_tabs();
}

static void cmd_stall(void) {
    options.stalled = true;
    con_printf("algorithm stalled\n");
}

static void cmd_all(const char *args);

static void cmd_heartbeat(void) {
    router_heartbeat();
    if (!options.sleeping) {
        con_printf("heartbeat sent\n");
    }
}

static void cmd_heartcheck(void) {
    router_dropcheck();
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
    router_restart();
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
        } else if (strncmp(text, "hbtime ", 7) == 0) {
            options.heartbeat = atoi(&text[7]);
        } else if (strcmp(text, "hc") == 0) {
            cmd_heartcheck();
        } else if (strncmp(text, "hctime ", 7) == 0) {
            options.heartcheck = atoi(&text[7]);
        } else if (strncmp(text, "infinity ", 9) == 0) {
            options.infinity = atoi(&text[9]);
        } else if (strcmp(text, "sleep") == 0) {
            cmd_sleep();
        } else if (strncmp(text, "dropadd ", 8) == 0) {
            options.show_dropadd = (text[8] != '0');
        } else if (strcmp(text, "wake") == 0) {
            cmd_wake();
        } else if (strncmp(text, "rtimeout ", 9) == 0) {
            options.rtimeout = atoi(&text[9]);
        } else if (strncmp(text, "rattempts ", 10) == 0) {
            options.rattempts = atoi(&text[10]);
        } else {
            con_errorf("unknown command: %s\n", text);
        }
    } else {
        while (*text == ' ') {
            text++;
        }
        int dst = 0;
        const char *st = text;
        while (*text >= '0' && *text <= '9') {
            dst = dst * 10 + (*text) - '0';
            text++;
        }
        if (st == text) {
            dst = -1;
        }
        while (*text == ' ') {
            text++;
        }
        rbl_sendmessage(dst, text);
    }
}

void cmd_order(const char *text) {
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

void cmd_init(void) {
    con_register(cmdparser);
}

void cmd_cleanup(void) {

}
