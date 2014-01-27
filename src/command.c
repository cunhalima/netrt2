#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

static void cmd_all(const char *args);

static void cmd_showtime(const char *args) {            // alterar o intervalo de tempo que mostra na tela as tabelas
    if (sscanf(args, "%d", &options.showtime) != 1) {
        con_error("usage: showtime <milliseconds>\n");
    }
}

static void cmd_ttl(const char *args) {                // time to live
    if (sscanf(args, "%d", &options.ttl) != 1) {
        con_error("usage: ttl <ttl>\n");
        return;
    }
}

static void cmd_debugrt(const char *args) {                // mostrar alteracoes na tabela de roteamento (debug)
    int yesno;

    if (sscanf(args, "%d", &yesno) != 1) {
        con_error("usage: debugrt <0/1>\n");
        return;
    }
    options.debugrt = yesno;
}

static void cmd_debugpacket(const char *args) {                // mostrar envio/recebimento de pacotes (debug)
    int yesno;

    if (sscanf(args, "%d", &yesno) != 1) {
        con_error("usage: debugpacket <0/1>\n");
        return;
    }
    options.debugpacket = yesno;
}

static void cmd_debuglost(const char *args) {                // mostrar perda de pacotes (debug)
    int yesno;

    if (sscanf(args, "%d", &yesno) != 1) {
        con_error("usage: debuglost <0/1>\n");
        return;
    }
    options.debuglost = yesno;
}

static void cmd_debugroute(const char *args) {                // mostrar msgs de roteamento (debug)
    int yesno;

    if (sscanf(args, "%d", &yesno) != 1) {
        con_error("usage: debugroute <0/1>\n");
        return;
    }
    options.debugroute = yesno;
}

static void cmd_drop(const char *args) {                // derrubar um vizinho manualmente (debug)
    int node;

    if (sscanf(args, "%d", &node) != 1) {
        con_error("usage: drop <node>\n");
        return;
    }
    drop_neighbour(node);                               // FIXME: deveria chamar a função de recalcular as rotas agora
    //tab_nhcalc();
}

void cmd_tabs(void) {                            // mostrar todas as tabelas
    print_tabs();
}

static void cmd_stall(void) {                           // pára o andamento automático do algoritumo (debug)
    options.stalled = true;
    con_printf("algorithm stalled\n");
}

static void cmd_heartbeat(void) {                       // envia o heartbeat
    router_heartbeat();
    if (!options.sleeping) {
        con_printf("heartbeat sent\n");
    }
}

static void cmd_heartcheck(void) {                      // faz o heartcheck
    router_dropcheck();
    if (!options.sleeping) {
        con_printf("heartcheck done\n");
    }
}

static void cmd_sleep(void) {                           // dorme para simular queda de roteador
    options.sleeping = true;
    con_printf("sleeping\n");
}

static void cmd_wake(void) {                            // acorda para simular reinício de roteador
    options.sleeping = false;
    router_restart();
    con_printf("waking up\n");
}

static void cmdparser(const char *text) {               // faz o parsing da linha de comando
    if (text[0] == '/') {
        text++;
        if ((strcmp(text, "tabs") == 0) ||
            (strcmp(text, "t") == 0)) {
            cmd_tabs();
        } else if (strncmp(text, "ttl ", 3) == 0) {
            cmd_ttl(&text[3]);
        } else if (strncmp(text, "debugrt ", 8) == 0) {
            cmd_debugrt(&text[8]);
        } else if (strncmp(text, "debugpacket ", 12) == 0) {
            cmd_debugpacket(&text[12]);
        } else if (strncmp(text, "debuglost ", 10) == 0) {
            cmd_debuglost(&text[10]);
        } else if (strncmp(text, "debugroute ", 11) == 0) {
            cmd_debugroute(&text[11]);
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
        while (*text == ' ') {                          // faz a análise léxica do chat: <destino> <mensagem>
            text++;
        }
        int dst = 0;
        const char *st = text;
        while (*text >= '0' && *text <= '9') {          // lê o destino (número inteiro)
            dst = dst * 10 + (*text) - '0';
            text++;
        }
        if (st == text) {
            dst = -1;
        }
        while (*text == ' ') {
            text++;
        }
        if (*text != '\0') {
            rbl_sendmessage(dst, text);                     // manda uma msg confiável (agora text aponta pro início da msg)
        }
    }
}

void cmd_order(const char *text) {                      // recebemos uma ordem, então vamos executá-la
    cmdparser(text);
}

void cmd_all(const char *args) {                        // manda uma ordem para todos os roteadores (debug)
    szb_t *msg;
    int to;

    msg = szb_create();
    szb_write8(msg, CMD_ORDER);                         // codifica a ordem na mensagem
    szb_writeCStr(msg, args);
    for (to = net_nextNode(-1); to >= 0; to = net_nextNode(to)) {   // para todos os nós da rede (inclusive eu)
        net_secretsend(to, msg);                                    // manda diretamente a msg pro nó (cheat)
    }
    szb_free(msg);
}

void cmd_init(void) {                                   // registra nosso parser no módulo de entrada
    con_register(cmdparser);
}

void cmd_cleanup(void) {
}
