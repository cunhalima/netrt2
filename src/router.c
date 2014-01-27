#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"


/* ==================

HEARTBEAT é o envio periódico do meu vetor-distância pros meus vizinhos
HEARTCHECK é a verificação periódica se algum vizinho parou de mandar msgs (caiu)

*/

#define CMD_DV      (CMD_ROUTER+1)

static int time;
static int nextHeartBeat;
static int nextHeartCheck;
static int nextShow;

bool update_rt(void);

void send_dv(void) {            // manda o vetor de distancias pros vizinhos
    szb_t *msg;
    int to;
    int tabsize;
    int i;

    if (options.sleeping) {     // qdo ta dormindo nao faz nada
        return;
    }
    tabsize = num_destinations();       // pega o total de nós conhecidos na rede
    msg = szb_create();
    szb_write8(msg, CMD_DV);            // prepara a msg <comando> [ <nó> <distância>, <nó> <distânci>, ... ] <-1>
    szb_write32(msg, options.id);
    for (i = 0; i < tabsize; i++) {     // grava todos os nós conhecidos
        int dist = get_rt_distance(i);
        szb_write32(msg, i);
        szb_write32(msg, dist);
    }
    szb_write32(msg, -1);               // marca o final
    for (to = next_neighbour(-1); to >= 0; to = next_neighbour(to)) {   // e manda pra todos os vizinhos conhecidos
        net_send(to, msg);
    }
    szb_free(msg);
}

static void my_print_rtab(void) {
    con_printf("[DEBUG] current routing table (timestamp = %d):\n", sys_getMilli());
    print_rtab();
}

void router_dropcheck(void) {           // verifica se algum vizinho caiu
    int i;
    bool dropped = false;

    if (options.sleeping) {
        return;
    }
    if (options.stalled) {          // quando o algoritmo está parado, o modo de identificar os vizinhos dormindo é diferente
        int largest_time = -1;
        //int base_time = -1;
        for (i = next_neighbour(-1); i >= 0; i = next_neighbour(i)) {
            int thistime = net_lastTime(i);
            if (thistime > largest_time) {
                //base_time = largest_time;
                largest_time = thistime;
            //} else if (thistime > base_time) {
            //    base_time = thistime;
            }
        }
        int base_time = largest_time;               // agora
        base_time -= 1000;              /* one second behind */
        for (i = next_neighbour(-1); i >= 0; i = next_neighbour(i)) {
            if (net_lastTime(i) < base_time) {
                //con_printf("dropping someone\n");
                drop_neighbour(i);
                dropped = true;
            }
        }
    } else {                        // esse é o método tradicional de identificar vizinhos dormindo
        for (i = next_neighbour(-1); i >= 0; i = next_neighbour(i)) {
            if (net_lastTime(i) + options.heartcheck < time) {          // se faz tempo q o vizinho nao manda nada...
                drop_neighbour(i);                                      // ... derruba ele!
                dropped = true;
            }
        }
    }
    if (dropped) {              // se alguémm foi derrubado, recalcula as rotas
        update_rt();
        send_dv();
        if (options.debugrt) {
            my_print_rtab();
        }
    }
}

bool update_rt(void) {                  // recálculo das rotas
    int ndests = num_destinations();
    bool changed = false;
    for (int to = 0; to < ndests; to++) {       // passa por todos os nós da rede que nós conhecemos
        if (to == options.id) {                 // não recalcula a rota pra mim, obviamente
            continue;
        }
        int mindist = -1;
        int minvia = -1;
        if (is_neighbour(to)) {                 // vizinhos têm a distância direta além da indireta
            mindist = get_neighbour_cost(to);
            minvia = to;
        }
        for (int via = next_neighbour(-1); via >= 0; via = next_neighbour(via)) {   // testa o caminho por cada vizinho
            int newdist = get_dv_distance(via, to);
            if (DIST_LESS(newdist, mindist)) {                  // se for uma distância menor, usa ela
                mindist = newdist;
                minvia = via;
            }
        }
        int olddist = get_rt_distance(to);
        if (olddist != mindist) {                   //  se a distância mudou...
            int oldvia = get_rt_via(to);
            changed = true;
            set_rt_distance(to, mindist);           // altera a entrada na tabelona
            set_rt_via(to, minvia);

            if (options.show_dropadd) {                 // mostra o q fez na tela (debug)
                if (!DIST_EXISTS(olddist)) {
                    con_printf("[DEBUG] adding node %d\n", to);
                }
                if (!DIST_EXISTS(mindist)) {
                    con_printf("[DEBUG] dropping node %d\n", to);
                }
            }
        }
    }
    return changed;                 // opa, algo mudou na tabela de roteamento, avisa isso
}

void recv_dv(szb_t *msg) {          // recebemos um vetor distancia de um vizinho
    int via;
    int viadist;
    int nh;
    bool changed;

    assert(msg != NULL);
    via = szb_read32(msg);              /* identifica o vizinho q manda a msg */
    assert(via >= 0);                   /* certifica-se de que eh um numero valido */
    changed = add_neighbour(via);                 /* adiciona-o como vizinho se ja nao for */
    viadist = get_neighbour_cost(via);  // pega nosso custo até o vizinho
    assert(DIST_EXISTS(viadist));
    for (;;) {                          // passa por todos os destinos q o vizinho nos mandou
        int to, dist, curdist;

        to = szb_read32(msg);           /* atual destino */
        if (to < 0) {                   /* condicao de parada - final da mensagem */
            break;
        }
        dist = szb_read32(msg);         /* distancia do meu vizinho ateh esse destino  */
        if (DIST_EXISTS(dist)) {
            dist += viadist;            // acrescenta o nosso custo até o vizinho
        }
        if (dist >= options.infinity) {     // aqui a gente resolve o problema do crescimento ao infinito
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
        if (options.debugrt) {
            my_print_rtab();
        }
    }
}

void reset_timeouts(void) {
    int startTime = time = sys_getMilli();                     /* Inicia o cronometro */
    nextHeartBeat = startTime + options.heartbeat;      // próximo hertbeat
    nextHeartCheck = startTime + options.heartcheck;       // ajusta tempo de heartcheck para não derrubar os vizinhos lodo de cara 
    nextShow = startTime + options.showtime;            // próxima mostrada de tabelas (debug)
}

void router_restart(void) {         // reiniciar o roteador
    tabs_cleanup();
    tabs_init();
    net_loadLinks();                // descobre os vizinhos
    time = sys_getMilli();
    router_heartbeat();             // faz o heartbeat logo que acorda
    reset_timeouts();
    if (options.debugrt) {
        my_print_rtab();
    }
}

void router_init(void) {
    router_restart();
    con_runfile("autoexec.script");
    reset_timeouts();
}

void router_cleanup(void) {

}

void router_logic(void) {                           // isso é executado o tempo todo (uma vez por loop)
    time = sys_getMilli();
    if (options.stalled) {              // se estiver parada, esquece
        return;
    }
    if (options.showtime > 0 && time >= nextShow) {     // opa, hora de mostrar as tabelas na tela (debug)
        nextShow = time + options.showtime;
        cmd_tabs();
    }
    if (time >= nextHeartBeat) {                        // opa, hora de dar um heartbeat
        nextHeartBeat = time + options.heartbeat;
        //con_printf("sending hb\n");
        send_dv();
    }
    if (time >= nextHeartCheck) {                       // opa, hora de fazer um heartcheck
        nextHeartCheck = time + options.heartcheck;
        //con_printf("sending hb\n");
        router_dropcheck();
    }
}

void router_readmessage(int cmd, szb_t *msg) {              // nosso roteador recebeu uma msg
    assert(msg != NULL);
    switch(cmd) {
    case CMD_DV:
        recv_dv(msg);                       // processa o vetor distancia q recebemos
        break;
    default:
        con_errorf("got invalid CMD=%d\n", cmd);
        break;
    }
}

void router_heartbeat(void) {               // faz um heartbeat (manda DV pros vizinhos)
    send_dv();
}

void router_sendmessage(int dst, szb_t *msg) {          // para mandar algo pelo roteador, nós mandamos pro vizinho da tabela de rotemaneto
    assert(msg != NULL);
    int via = get_rt_via(dst);                  // acessa a tabela de rotemaneto pra saber pra que vizinho mandar
    if (via >= 0) {
        net_send(via, msg);             // manda!!
    }
}
