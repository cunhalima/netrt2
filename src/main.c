#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

int main(int argc, char **argv) {
    szb_t *msg;

    sys_init();
    opt_readArgs(argc, argv);
    tabs_init();
    con_init();
    con_printf("id=%d\n", options.id);
    if (!net_init()) {
        con_cleanup();
        tabs_cleanup();
        return -1;
    }
    cmd_init();
    router_init();
    rbl_init();
    msg = szb_create();
    while (con_isRunning()) {
        int sender;

        while (net_recv(&sender, msg)) {
            int cmd = szb_read8(msg);
            if (cmd == CMD_ORDER) {
                cmd_order(szb_getPosPtr(msg));
            } else if (!options.sleeping) {
                if (cmd >= CMD_ROUTER && cmd < CMD_RELIABLE) {
                    router_readmessage(cmd, msg);
                } else if (cmd >= CMD_RELIABLE) {
                    rbl_readmessage(cmd, msg);
                } else {
                    con_errorf("got invalid CMD=%d\n", cmd);
                    break;
                }
            }
        }
        router_logic();
        rbl_logic();
        con_checkInput();
        sys_sleep(20);
    }
    szb_free(msg);
    rbl_cleanup();
    router_cleanup();
    cmd_cleanup();
    net_cleanup();
    con_cleanup();
    tabs_cleanup();
    return 0;
}
