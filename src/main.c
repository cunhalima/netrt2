#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

int main(int argc, char **argv) {
    szb_t *msg;

    sys_init();                         // Inicializa coisas do sistema (temporizador)
    opt_readArgs(argc, argv);           // Lê os parâmetros do programa
    tabs_init();                        // inicializa o gerenciamento das tabelas
    con_init();                         // inicializa o console assíncrono
    con_printf("id=%d\n", options.id);  // mostra o id deste roteador na tela
    if (!net_init()) {                  // inicializa o módulo de rede
        con_cleanup();                  // finaliza o console se der erro
        tabs_cleanup();                 //  ... e também as tabelas
        return -1;
    }
    cmd_init();                         // inicializa o interpretadore de comandos
    router_init();                      // inicializa o módulo de roteamento
    rbl_init();                         // inicializa o módulo de mensagens confiáveis
    msg = szb_create();                 // prepara o buffer de dados
    while (con_isRunning()) {           // executa o laço enquanto o usuário não quiser sair
        int sender;

        while (net_recv(&sender, msg)) {                        // recebe dados da rede
            int cmd = szb_read8(msg);                           // primeiro byte = comando
            if (cmd == CMD_ORDER) {                             // ordens são processadas à parte
                cmd_order(szb_getPosPtr(msg));                  // processa o comando a partir da posição atual no buffer
            } else if (!options.sleeping) {                     // nós dormindo não processam mensagens q chegam
                if (cmd >= CMD_ROUTER && cmd < CMD_RELIABLE) {  // mensagem do tipo ROTEADOR
                    router_readmessage(cmd, msg);
                } else if (cmd >= CMD_RELIABLE) {               // mensagem do tipo CONFIÁVEL
                    rbl_readmessage(cmd, msg);
                } else {
                    con_errorf("got invalid CMD=%d\n", cmd);    // mensagem de tipo desconhecido
                    break;
                }
            }
        }
        router_logic();                 // lógica de execução do roteador
        rbl_logic();                    // lógica de execução das mensagens confiáveis
        con_checkInput();               // leitura assíncrona da entrada
        sys_sleep(20);                  // dê um tempo à CPU para não esgotar o processador
    }
    szb_free(msg);                      // libera tudo...
    rbl_cleanup();
    router_cleanup();
    cmd_cleanup();
    net_cleanup();
    con_cleanup();
    tabs_cleanup();
    return 0;
}
