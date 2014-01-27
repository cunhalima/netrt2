#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

static table_t *mvec = NULL;       // vetor de distâncias, roteamento etc
static table_t *mtab = NULL;       // armazena o vetor-distância recebido pelos vizinhos

struct mtab_s {
    int distance;           // a tabelona só tem a distância mesmo
};

struct mvec_s {
    int cost;               // este custo somente existe para os vizinhos em potencial
    bool online;            // online = true: vizinho detectado, false: vizinho caiu ou nunca foi vizinho
    int via;                // proximo pulo
    int distance;           // destino calculado (tabela de roteamento)
    int inseqno, outseqno;  // números de sequência para as mensagens confiáveis
};

typedef struct mtab_s mtab_t;   // cada item da tabelona de vetor-distância
typedef struct mvec_s mvec_t;   // cada item da tabela de destinos

void tabs_init(void) {                              // inicializa este módulo
    mtab_t deftab;
    mvec_t defvec;

    deftab.distance = -1;                           // distância padrão = INFINITA
    mtab = tab_create(&deftab, sizeof(mtab_t));

    defvec.cost = -1;                               // custo padrão infinito
    defvec.online = false;                          // por padrão nenhum é vizinho
    defvec.via = -1;                                // por padrão não temos como alcançar os nós
    defvec.distance = -1;                           // por padrão distância infinita aos nós
    defvec.inseqno = 0;                             // sequenciamento começa em 0
    defvec.outseqno = 0;
    mvec = tab_create(&defvec, sizeof(mvec_t));
}

void tabs_cleanup(void) {                           // libera a memória utilizada por este módulo
    tab_free(mvec);
    mvec = NULL;
    tab_free(mtab);
    mtab = NULL;
}

bool is_neighbour(int node) {                       // verifica se é vizinho olhando para o atributo "online"
    mvec_t *vec;

    assert(node >= 0);
    vec = (mvec_t *)tab_get(mvec, 0, node);
    return vec->online;
}

void drop_neighbour(int node) {                     // quando detecta que o vizinho caiu
    int count;
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.online = false;                             // coloca ele como offline
    vec.via = -1;                                   // coloca como inalcancavel, já que a nova rota vai ser recalculada em seguida mesmo
    vec.distance = -1;                              // coloca como inalcancavel, já que a nova rota vai ser recalculada em seguida mesmo
    tab_set(mvec, 0, node, &vec);

    count = num_destinations();

    // apaga tudo o que passa por esse nó
    //for (int n = next_neighbour(-1); n >= 0; n = next_neighbour(n)) {
    for (int i = 0; i < count; i++) {
        if (get_rt_via(i) == node) {
            set_rt_via(i, -1);          // se esse caminha passava pelo vizinho q caiu, faz comq nao passe por nenhum
            set_rt_distance(i, -1);     // coloca distancia infinita
        }
    }
    //set_rt_distance(node, -1);          
    if (options.show_dropadd) {
        con_printf("[DEBUG] dropping node %d\n", node);
    }
}

bool add_neighbour(int node) {                  // vamos adicionar um vizinho
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    if (!DIST_EXISTS(vec.cost)) {               // se não tem custo, não é vizinho em potencial
        return false;
    }
    if (vec.online) {                           // se já está online, não reajusta o estado
        return false;
    }
    vec.online = true;                          // coloca como online
    vec.via = node;                             // já que até agora não era vizinho, coloca o acesso direto como via de acesso
    vec.distance = vec.cost;                    // já que até agora não era vizinho, coloca o acesso direto como via de acesso
    tab_set(mvec, 0, node, &vec);
    if (options.show_dropadd) {
        con_printf("[DEBUG] adding node %d\n", node);
    }
    return true;
}

int next_neighbour(int node) {                  // obtém o próximo vizinho iterativamente. Retorna -1 se não houver mais vizinhos
    int count;

    assert(node >= -1);
    count = tab_cols(mvec);
    for (int i = node + 1; i < count; i++) {
        if (is_neighbour(i)) {
            return i;
        }
    }
    return -1;
}

void set_neighbour_cost(int node, int cost) {   // muda o custo desse vizinho
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.cost = cost;
    tab_set(mvec, 0, node, &vec);
}

int get_neighbour_cost(int node) {              // pega o custo desse vizinho
    const mvec_t *vec;

    vec = tab_get(mvec, 0, node);
    return vec->cost;
}

void set_node_inseqno(int node, int seqno) {    // muda a sequência de chegada (mensagens confiáveis)
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.inseqno = seqno;
    tab_set(mvec, 0, node, &vec);
}

int get_node_inseqno(int node) {                // pega a sequência de chegada (mensagens confiáveis)
    const mvec_t *vec;

    vec = tab_get(mvec, 0, node);
    return vec->inseqno;
}

void set_node_outseqno(int node, int seqno) {   // muda a sequência de saída (mensagens confiáveis)
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.outseqno = seqno;
    tab_set(mvec, 0, node, &vec);
}

int get_node_outseqno(int node) {               // pega a sequência de saída (mensagens confiáveis)
    const mvec_t *vec;

    vec = tab_get(mvec, 0, node);
    return vec->outseqno;
}

void set_rt_distance(int node, int dist) {      // muda a distância atual na tabela de roteamento
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.distance = dist;
    tab_set(mvec, 0, node, &vec);
}

int get_rt_distance(int node) {                 // muda a distância atual na tabela de roteamento
    const mvec_t *vec;

    vec = tab_get(mvec, 0, node);
    return vec->distance;
}

void set_rt_via(int node, int via) {            // muda a via de acesso (proximo pulo)
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.via = via;
    tab_set(mvec, 0, node, &vec);
}

int get_rt_via(int node) {                      // pega a via de acesso (proximo pulo)
    const mvec_t *vec;

    vec = tab_get(mvec, 0, node);
    return vec->via;
}

void set_dv_distance(int via, int to, int dist) {   // muda a distancia na tabelna de DVs
    mtab_t tab;

    tab_getCopy(mtab, via, to, &tab);
    tab.distance = dist;
    tab_set(mtab, via, to, &tab);
}

int get_dv_distance(int via, int to) {              // pega a distancia na tabelna de DVs
    const mtab_t *tab;

    tab = tab_get(mtab, via, to);
    return tab->distance;
}

int num_destinations(void) {                        // pega o número total de destinos
    return max(tab_cols(mvec), tab_cols(mtab));
}

static void internal_print_hline(int size) {
    int i;

    con_printf("  +");
    for (i = 0; i < size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
}

static void internal_print_numbers(int size) {
    int i;

    con_printf("  |");
    for (i = 0; i < size; i++) {
        con_printf(" %3d", i);
    }
    con_printf("|\n");

}

static void internal_print_distances(int size) {
    int i;

    con_printf("D |");
    for (i = 0; i < size; i++) {
        int v = get_rt_distance(i);
        if (v >= 0) {
            con_printf(" %3d", v);
        } else {
            con_printf("   -");
        }
    }
    con_printf("|\n");
}

static void internal_print_via(int size) {
    int i;

    con_printf("V |");
    for (i = 0; i < size; i++) {
        int v = get_rt_via(i);
        if (v >= 0) {
            con_printf(" %3d", v);
        } else {
            con_printf("   -");
        }
    }
    con_printf("|\n");
}

static void internal_print_neighbours(int size) {
    int i;

    con_printf("N |");
    for (i = 0; i < size; i++) {
        bool v = is_neighbour(i);
        if (v) {
            con_printf(" ngb", v);
        } else {
            con_printf("   -");
        }
    }
    con_printf("|\n");
}

static void internal_print_dvtab(int size) {
    int i;

    for (int n = 0; n < size; n++) {
        if (!is_neighbour(n)) {
            continue;
        }
        con_printf("%d |", n);
        for (i = 0; i < size; i++) {
            int v = get_dv_distance(n, i);
            if (v >= 0) {
                con_printf(" %3d", v);
            } else {
                con_printf("   -");
            }
        }
        con_printf("|\n");
    }
}

static void internal_print_rtab(int size) {
    internal_print_hline(size);
    internal_print_numbers(size);
    internal_print_hline(size);
    internal_print_distances(size);
    internal_print_via(size);
}

void print_rtab(void) {
    int size = num_destinations();
    internal_print_rtab(size);
    internal_print_hline(size);
}

void print_tabs(void) {                             // imprima a tabela de roteamento e a tabelona de DVs
    int size = num_destinations();
    internal_print_rtab(size);
    internal_print_neighbours(size);
    internal_print_hline(size);
    internal_print_dvtab(size);
    internal_print_hline(size);
}
