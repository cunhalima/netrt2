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

// <command> <to> <from> <timetolive> <seqno> <msg>

void rbl_init(void) {                   // inicia sistema de msgs confiáveis
    pending = false;
    sndbuf = szb_create();
}

void rbl_cleanup(void) {                // libera sistema de msgs confiáveis
    szb_free(sndbuf);
    sndbuf = NULL;
}

static void my_router_sendmsg(int to, szb_t *msg) {
    assert(msg != NULL);
    if (options.debugroute) {
        szb_rewind(msg);
        szb_read8(msg);
        int to = szb_read8(msg);                        // pega o destino da msg
        int from = szb_read8(msg);                      // identifica o remetente
        int ttl = szb_read8(msg);                      // identifica o remetente
        int seqno = 0;
        #ifdef USE_SEQ
        seqno = szb_read32(msg);                        // pega o numero de sequencia
        #endif
        int via = get_rt_via(to);
        con_printf("[DEBUG] router %d: msg seqno=%d ttl=%d to %d (src=%d, dst=%d)\n", options.id, seqno, ttl, via, from, to);
    }
    router_sendmessage(to, msg);
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
    szb_write8(msg, options.ttl);           // time to live
    #ifdef USE_SEQ
    szb_write32(msg, seqno);                // manda a sequência
    #endif
    my_router_sendmsg(from, msg);          // manda a msg
    szb_free(msg);

}

static void print_received_msg(int from, const char *msg) {     // mostra na tela a msg q recebeu (bate-papo)
    con_printf("(MSG from %d): %s\n", from, msg);
}

void rbl_readmessage(int cmd, szb_t *msg) {         // processa a msg que recebeu
    assert(msg != NULL);
    int to;
    int seqno = 0;

    to = szb_read8(msg);                            // pega o destino da msg
    int from = szb_read8(msg);                      // identifica o remetente
    int ttl = szb_read8(msg);                      // identifica o remetente
    #ifdef USE_SEQ
    seqno = szb_read32(msg);                        // pega o numero de sequencia
    #endif
    if (to != options.id) {                         // se não for eu o destinatário,
        if (ttl != 0) {
            ttl--;
            szb_rewind(msg);
            szb_read8(msg);    // cmd
            szb_read8(msg);    // to
            szb_read8(msg);    // from
            szb_write8(msg, ttl);       // decrementa timetolive
            my_router_sendmsg(to, msg);                //  manda pro destino correto
        }
        return;
    }
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
                    con_printf("[DEBUG] got out-of-order from %d (expected seq=%d got seq=%d)\n", from, expected, seqno);
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
            con_printf("(MSG to %d sucessful): %s\n", from, szb_getPtr(sndbuf)); // mostra q foi OK
            pending = false;
            #endif
            break;
        default:
            break;
    }
}

void rbl_sendmessage(int dst, const char *text) {       // guarda no buffer a msg para ser enviada de forma confiável
    if (dst < 0) {
        con_printf("[ERROR] broadcast not implemented\n");
        return;
    }
    if (pending) {                                      // só tratamos uma msg de cada vez
        con_printf("[ERROR] can't speak for now, msg pending\n");
        return;
    }
    if (strlen(text) > 100) {
        con_printf("[ERROR] chat message has 100 characters limit, sorry\n");
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
        con_printf("[ERROR] unable to deliver reliably to %d\n", sendtarget);
        int seqno = get_node_outseqno(sendtarget);  // avança o n. de sequencia
        ADVANCE_SEQ(seqno);
        set_node_outseqno(sendtarget, seqno);
        pending = false;            // e desbloqueia para envio de novas msgs
        return;
    }
    int via = get_rt_via(sendtarget);
    if (via < 0) {
        con_printf("[ERROR] destination %d unreachable\n", sendtarget);
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
    szb_write8(msg, options.ttl);
    int seqno = 0;
    #ifdef USE_SEQ
    seqno = get_node_outseqno(sendtarget);
    szb_write32(msg, seqno);
    #endif
    szb_writeCStr(msg, (const char *)szb_getPtr(sndbuf));
    //if (options.debugroute) {
    //    int via = get_rt_via(sendtarget);
    //    con_printf("[DEBUG] router %d: msg seqno=%d to %d (src=%d, dst=%d)\n", options.id, seqno, via, options.id, sendtarget);
    //}
    my_router_sendmsg(sendtarget, msg);    // manda a msg pelo roteador
    szb_free(msg);
    if (options.rdebug) {
        con_printf("[DEBUG] reliable send attempt\n");
    }
}
