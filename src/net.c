#include <features.h>
#define __USE_MISC
#include <arpa/inet.h>
#undef __USE_MISC
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "defs.h"

#define INVALID_NODE    -1
#define MAX_PACKET_SIZE 4096

struct node_s;

typedef struct node_s node_t;

struct node_s {
    bool valid;
    int lastTime;
    struct sockaddr_in sin;
};

static node_t *nodes = NULL;
static int numNodes;
static int sock = -1;

/*------------------------------
 * net_init()
 *------------------------------
 * Esta funcao inicializa o sistema de comunicacao.
 */
static int getNodeFromAddr(struct sockaddr_in *sin) {
    int i;

    assert(sin != NULL);
    if (nodes == NULL) {
        return INVALID_NODE;
    }
    for (i = 0; i < numNodes; i++) {
        if (nodes[i].sin.sin_addr.s_addr == sin->sin_addr.s_addr &&
            nodes[i].sin.sin_port == sin->sin_port) {
            return i;
        }
    }
    return INVALID_NODE;
}

/*------------------------------
 * net_init()
 *------------------------------
 * Esta funcao inicializa o sistema de comunicacao.
 */
bool net_secretsend(int node, szb_t *msg) {
    int sendlen;

    assert(msg != NULL);
    if (node < 0 || node >= numNodes) {
        con_errorf("invalid node number: %d\n", node);
        return false;
    }
    if (!nodes[node].valid) {
        return false;
    }
    sendlen = sendto(sock,
        szb_getPtr(msg),
        szb_getSize(msg),
        0,
        (struct sockaddr *)&nodes[node].sin,
        sizeof(nodes[node].sin));
    if (sendlen == -1) {
        con_errorf("sendto returned -1 (errno = %d)\n", errno);
        return false;
    }
    return true;
}

/*------------------------------
 * net_init()
 *------------------------------
 * Esta funcao inicializa o sistema de comunicacao.
 */
bool net_send(int node, szb_t *msg) {
    int sendlen;

    assert(msg != NULL);
    if (node < 0 || node >= numNodes) {
        con_errorf("invalid node number: %d\n", node);
        return false;
    }
    if (!is_neighbour(node)) {
        con_errorf("cheater!! %d is not your neighbour\n", node);
        return false;
    }
    return net_secretsend(node, msg);
}


/*------------------------------
 * net_init()
 *------------------------------
 * Esta funcao inicializa o sistema de comunicacao.
 */
bool net_recv(int *pnode, szb_t *msg) {
    int recvlen;
    struct sockaddr_in sin;
    socklen_t slen;

    assert(msg != NULL);
    slen = sizeof(sin);
    szb_resize(msg, MAX_PACKET_SIZE);
    recvlen = recvfrom(sock,
        szb_getPtr(msg),
        szb_getSize(msg),
        MSG_DONTWAIT,
        (struct sockaddr *)&sin,
        &slen);
    if (recvlen == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            con_error("recvfrom returned -1\n");
        }
        return false;
    }
    int node = getNodeFromAddr(&sin);
    szb_rewind(msg);
    szb_resize(msg, recvlen);
    if (szb_read8(msg) != 102) { //CMD_ORDER) { HACK!!!!
        nodes[node].lastTime = sys_getMilli();
    }
    szb_rewind(msg);
    if (pnode != NULL) {
        *pnode = node;
    }
    return true;
}

/*------------------------------
 * loadRouters()
 *------------------------------
 * Esta funcao carrega o endereco ip e porta de todos os roteadores que
 * compoem a rede.
 */
static bool loadRouters() {
    FILE *file;
    int id, port;
    char host[256];
    static char filename[] = "roteador.config";

    numNodes = 0;                                                   /* Inicia a contagem em zero */
    file = fopen(filename, "rt");                                   /* Tenta abrir o arquivo de configuracao */
    if (file == NULL) {
        con_errorf("unable to open %s\n", filename);
        return false;
    }
    while(fscanf(file, "%d %d %255s", &id, &port, host) == 3) {     /* Primeira passada pelo arquivo. Serve */
        if (id > numNodes) {                                        /*  apenas para obter a quantidade */
            numNodes = id;                                          /*  de roteadores e armazena-la em */
        }                                                           /*  numNodes */
    }
    numNodes++;                                                     /* Ateh agora numNodes era o id mais alto. */
                                                                    /* Para obter a quantidade, vamos acrescentar o numero um */

    if (numNodes <= 0) {                                            /* Se nao houver nos para alocar, ja pode sair da funcao */
        fclose(file);
        return false;
    }

    fseek(file, 0, SEEK_SET);                                       /* Inicia a segunda passada pelo arquivo */
    nodes = (node_t *)calloc(numNodes, sizeof(*nodes));             /* Agora ja sabemos o total de roteadores e podemos */
                                                                    /*  alocar o vetor com o tamanho correto */
    while(fscanf(file, "%d %d %255s", &id, &port, host) == 3) {
        if (inet_aton(host, &nodes[id].sin.sin_addr) == 0) {        /* Tenta converter uma string em um endereco IP */
            free(nodes);
            nodes = NULL;
            fclose(file);
            con_errorf("unable to resolve %s\n", host);
            return false;
        }
        nodes[id].valid = true;
        nodes[id].sin.sin_family = AF_INET;
        nodes[id].sin.sin_port = htons(port);
    }
    fclose(file);
    return true;
}   

/*------------------------------
 * loadLinks()
 *------------------------------
 */
static bool loadLinks() {
    FILE *file;
    int a, b, cost;
    static char filename[] = "enlaces.config";

    file = fopen(filename, "rt");                                   /* Tenta abrir o arquivo de configuracao */
    if (file == NULL) {
        con_errorf("unable to open %s\n", filename);
        return false;
    }
    tab_dvset(options.id, 0);
    while(fscanf(file, "%d %d %d", &a, &b, &cost) == 3) {
        if (a == options.id) {
            tab_dvset(b, cost);
            tab_nhset(b, b);
            tab_fdset(b, cost);
            tab_dnset(b, false);
        }
        if (b == options.id) {
            tab_dvset(a, cost);
            tab_nhset(a, a);
            tab_fdset(a, cost);
            tab_dnset(a, false);
        }
    }
    fclose(file);
    return true;
}

/*------------------------------
 * net_init()
 *------------------------------
 * Esta funcao inicializa o sistema de comunicacao.
 */
bool net_init(void) {
    if (!loadRouters()) {                               /* carrega os enderecos de todos os roteadores */
        net_cleanup();
        return false;
    }
    if (!loadLinks()) {                                 /* carrega os links dos roteadores vizinhos */
        net_cleanup();
        return false;
    }
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        con_errorf("socket allocation error\n");
        return false;
    }
    if (bind(sock, (struct sockaddr *)&nodes[options.id].sin, sizeof(nodes[options.id].sin)) == -1) {
        close(sock);
        sock = -1;
        con_errorf("socket bind error\n");
        return false;
    }
    return true;
}

/*------------------------------
 * net_init()
 *------------------------------
 * Esta funcao inicializa o sistema de comunicacao.
 */
void net_cleanup(void) {
    if (sock != -1) {
        close(sock);
        sock = -1;
    }
    free(nodes);
    nodes = NULL;
}

/*------------------------------
 * net_init()
 *------------------------------
 * Esta funcao inicializa o sistema de comunicacao.
 */
int net_lastTime(int node) {
    if (node < 0 || node >= numNodes) {
        con_errorf("invalid node number: %d\n", node);
        return 0;
    }
    return nodes[node].lastTime;
}

int net_nextNode(int node) {
    node = node + 1;
    if (node < 0 || node >= numNodes) {
        return -1;
    }
    return node;
}
