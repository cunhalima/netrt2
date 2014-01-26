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

void rbl_init(void) {
    pending = false;
    sndbuf = szb_create();
}

void rbl_cleanup(void) {
    szb_free(sndbuf);
    sndbuf = NULL;
}

#ifdef USE_SEQ
void rbl_confirm(int from, int seqno) {
#else
void rbl_confirm(int from) {
#endif
    szb_t *msg;

    msg = szb_create();
    szb_write8(msg, CMD_CONFIRM);
    szb_write8(msg, from);
    szb_write8(msg, options.id);
    #ifdef USE_SEQ
    szb_write32(msg, seqno);
    #endif
    router_sendmessage(from, msg);
    szb_free(msg);

}

static void print_received_msg(int from, const char *msg) {
    con_printf("(MSG from %d): %s\n", from, msg);
}

void rbl_readmessage(int cmd, szb_t *msg) {
    assert(msg != NULL);
    int to;
    int seqno;

    to = szb_read8(msg);
    if (to != options.id) {
        //con_printf("to %d\n", to);
        router_sendmessage(to, msg);
        return;
    }
    int from = szb_read8(msg);
    #ifdef USE_SEQ
    seqno = szb_read32(msg);
    #endif
    switch (cmd) {
        case CMD_CHAT:
            #ifdef USE_SEQ
            rbl_confirm(from, seqno);
            if (seqno == get_node_inseqno(from)) {
                //con_printf("we got msg(%d): %s\n", seqno, szb_getPosPtr(msg));
                //con_printf("(MSG from %d): %s\n", from, szb_getPosPtr(msg));
                print_received_msg(from, szb_getPosPtr(msg));
                ADVANCE_SEQ(seqno);
                set_node_inseqno(from, seqno);
            } else {
                int expected = get_node_inseqno(from);
                if (options.rdebug) {
                    con_printf("got OOOMSG from %d (expected %d got %d)\n", from, expected, seqno);
                }
                if (expected < seqno) {     // Isso serve  pra tratar o caso em que o nodo é reiniciado
                    print_received_msg(from, szb_getPosPtr(msg));
                    ADVANCE_SEQ(seqno);
                    set_node_inseqno(from, seqno);
                }
            }
            #else
            rbl_confirm(from);
            con_printf("(MSG from %d): %s\n", from, szb_getPosPtr(msg));
            #endif
            break;
        case CMD_CONFIRM:
            #ifdef USE_SEQ
            if (seqno == get_node_outseqno(from)) {
                con_printf("(MSG to %d sucessful): %s\n", from, szb_getPtr(sndbuf));
                ADVANCE_SEQ(seqno);
                set_node_outseqno(from, seqno);
                pending = false;
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

void rbl_sendmessage(int dst, const char *text) {
    if (dst < 0) {
        con_printf("no broadcast by now\n");
        return;
    }
    if (pending) {
        con_printf("cant say now, msg pending\n");
        return;
    }
    szb_clear(sndbuf);
    szb_writeCStr(sndbuf, text);
    pending = true;
    sendtime = sys_getMilli();
    sendtarget = dst;
    attempts = options.rattempts;
    //con_printf("you try to say: <%s>\n", text);
}

void rbl_logic(void) {
    szb_t *msg;

    if (options.sleeping) {
        return;
    }
    if (!pending) {
        return;
    }
    int time = sys_getMilli();
    if (time < sendtime) {
        return;
    }
    attempts--;
    if (attempts < 0) {
        con_printf("(ERROR): Unable to deliver reliably to %d\n", sendtarget);
        int seqno = get_node_outseqno(sendtarget);
        ADVANCE_SEQ(seqno);
        set_node_outseqno(sendtarget, seqno);
        pending = false;
        return;
    }
    sendtime += options.rtimeout;
    msg = szb_create();
    szb_write8(msg, CMD_CHAT);
    szb_write8(msg, sendtarget);
    szb_write8(msg, options.id);
    #ifdef USE_SEQ
    szb_write32(msg, get_node_outseqno(sendtarget));
    #endif
    szb_writeCStr(msg, (const char *)szb_getPtr(sndbuf));
    router_sendmessage(sendtarget, msg);
    szb_free(msg);
    if (options.rdebug) {
        con_printf("rsnd attempt\n");
    }
}
