#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

options_t options;

void opt_readArgs(int argc, char **argv) {
    int i;

    memset(&options, 0, sizeof(options));
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
            case 'e':
                if (i + 1 < argc) {
                    options.errRate = atoi(argv[++i]);
                    if (options.errRate < 1) {
                        options.errRate = 1;
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
