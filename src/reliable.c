#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

#define CMD_CHAT    (CMD_RELIABLE+1)
#define CMD_CONFIRM (CMD_RELIABLE+2)

#define USE_SEQ
#define ADVANCE_SEQ(x) x++

static szb_t *sndbuf;
static bool pending;
static int sendtarget;
static int sendtime;
static int attempts;

void rbl_init(void) {                   // inicia sistema de msgs confiáveis
    pending = false;
    sndbuf = szb_create();
}

void rbl_cleanup(void) {                // libera sistema de msgs confiáveis
    szb_free(sndbuf);
    sndbuf = NULL;
}

#ifdef USE_SEQ
void rbl_confirm(int from, int seqno) {     // manda uma confirmação pro nó que enviou a msg confiável
#else
void rbl_confirm(int from) {
#endif
    szb_t *msg;

    msg = szb_create();
    szb_write8(msg, CMD_CONFIRM);           // msg CONFIRM
    szb_write8(msg, from);                  // mandar para a origem da msg confiável
    szb_write8(msg, options.id);            // nos identifica como remetente
    #ifdef USE_SEQ
    szb_write32(msg, seqno);                // manda a sequência
    #endif
    router_sendmessage(from, msg);          // manda a msg
    szb_free(msg);

}

static void print_received_msg(int from, const char *msg) {     // mostra na tela a msg q recebeu (bate-papo)
    con_printf("(MSG from %d): %s\n", from, msg);
}

void rbl_readmessage(int cmd, szb_t *msg) {         // processa a msg que recebeu
    assert(msg != NULL);
    int to;
    int seqno;

    to = szb_read8(msg);                            // pega o destino da msg
    if (to != options.id) {                         // se não for eu o destinatário,
        //con_printf("to %d\n", to);
        router_sendmessage(to, msg);                //  manda pro destino correto
        return;
    }
    int from = szb_read8(msg);                      // identifica o remetente
    #ifdef USE_SEQ
    seqno = szb_read32(msg);                        // pega o numero de sequencia
    #endif
    switch (cmd) {
        case CMD_CHAT:                              // recebeu uma msg de bate-papo
            #ifdef USE_SEQ
            rbl_confirm(from, seqno);               // manda de volta a confirmação
            if (seqno == get_node_inseqno(from)) {      // se for a squencia esperada, blz
                print_received_msg(from, szb_getPosPtr(msg));       // só imprime na tela
                ADVANCE_SEQ(seqno);                                 // e avança a sequência
                set_node_inseqno(from, seqno);
            } else {                                    // se não for a seq esperada
                int expected = get_node_inseqno(from);
                if (options.rdebug) {                   // mostra o problema (debug)
                    con_printf("got OOOMSG from %d (expected %d got %d)\n", from, expected, seqno);
                }
                if (expected < seqno) {     // Isso serve  pra tratar o caso em que o nodo é reiniciado (espera sequencia menor do q recebe)
                    print_received_msg(from, szb_getPosPtr(msg));       // mostra mesmo assim
                    ADVANCE_SEQ(seqno);
                    set_node_inseqno(from, seqno);                      // e avança a sequencia (ignora a esperada)
                }
            }
            #else
            rbl_confirm(from);                                          // manda de volta a confirmação
            con_printf("(MSG from %d): %s\n", from, szb_getPosPtr(msg));
            #endif
            break;
        case CMD_CONFIRM:                   // recebemos uma confirmacao
            #ifdef USE_SEQ
            if (seqno == get_node_outseqno(from)) {     // se for a confirmação que estamos esperando
                con_printf("(MSG to %d sucessful): %s\n", from, szb_getPtr(sndbuf)); // mostra q foi OK
                ADVANCE_SEQ(seqno);                 // avança a sequencia de saída
                set_node_outseqno(from, seqno);
                pending = false;                    // e desbloqueia o envio de msgs de bate-papo
            }
            #else
            con_printf("other side got my msg\n");
            pending = false;
            #endif
            break;
        default:
            break;
    }
}

void rbl_sendmessage(int dst, const char *text) {       // guarda no buffer a msg para ser enviada de forma confiável
    if (dst < 0) {
        con_printf("no broadcast by now\n");
        return;
    }
    if (pending) {                                      // só tratamos uma msg de cada vez
        con_printf("cant say now, msg pending\n");
        return;
    }
    szb_clear(sndbuf);                      // guarda no buffer
    szb_writeCStr(sndbuf, text);
    pending = true;                         // sinaliza q estamos tentando mandar
    sendtime = sys_getMilli();              // guarda o tempo de início do envio
    sendtarget = dst;                       // guarda o destino
    attempts = options.rattempts;           // e prepara o número de tentativas
}

void rbl_logic(void) {              // essa função é chamada o tempo todo (uma vez por loop)
    szb_t *msg;

    if (options.sleeping) {         // não faz nada se o nó estiver dormindo, logicamente
        return;
    }
    if (!pending) {                 // só continua se tiver msg pra mandar
        return;
    }
    int time = sys_getMilli();      // pega a hora atual
    if (time < sendtime) {          // se não for momento de mandar msg, esquece
        return;
    }
    attempts--;                     // decrementa o número de tentativas
    if (attempts < 0) {             // esgotou o n. de tentativas
        con_printf("(ERROR): Unable to deliver reliably to %d\n", sendtarget);
        int seqno = get_node_outseqno(sendtarget);  // avança o n. de sequencia
        ADVANCE_SEQ(seqno);
        set_node_outseqno(sendtarget, seqno);
        pending = false;            // e desbloqueia para envio de novas msgs
        return;
    }
    sendtime += options.rtimeout;   // guarda o momento em q vai tentar enviar de novo
    msg = szb_create();
    szb_write8(msg, CMD_CHAT);      // prepara a msg confiável <comando> <dstino> <origem> <sequencia> <msg>
    szb_write8(msg, sendtarget);
    szb_write8(msg, options.id);
    #ifdef USE_SEQ
    szb_write32(msg, get_node_outseqno(sendtarget));
    #endif
    szb_writeCStr(msg, (const char *)szb_getPtr(sndbuf));
    router_sendmessage(sendtarget, msg);    // manda a msg pelo roteador
    szb_free(msg);
    if (options.rdebug) {
        con_printf("rsnd attempt\n");
    }
}
