#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

options_t options;

void opt_readArgs(int argc, char **argv) {              // lê as opções passadas pela linha de comando
    int i;

    memset(&options, 0, sizeof(options));
    options.heartbeat = 2000;
    options.heartcheck = (2*options.heartbeat);
    options.infinity = 100;
    options.rtimeout = 1000;
    options.rattempts = 3;
    options.ttl = 10;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
            case 'e':
                if (i + 1 < argc) {
                    options.errRate = atoi(argv[++i]);
                    if (options.errRate < 0) {
                        options.errRate = 0;
                    } else if (options.errRate > 100) {
                        options.errRate = 100;
                    }
                }
                break;
            default:
                con_errorf("unknown option: %c\n", argv[i][1]);
                break;
            }
        } else if (argv[i][0] >= '0' && argv[i][0] <= '9') {
            options.id = atoi(argv[i]);
        } else {
            con_errorf("invalid argument: %s\n", argv[i]);
        }
    }
}
