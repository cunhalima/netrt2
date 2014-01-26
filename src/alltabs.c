#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

static table_t *mvec;
static table_t *mtab;

struct mtab_s {
    int distance;
};

struct mvec_s {
    int cost;    // Este custo somente existe para os vizinhos
    bool online;       // Este custo somente existe para os vizinhos online
    int via;       // proximo pulo
    int distance;   // destino calculado (tabela de roteamento)
    int inseqno, outseqno;
};

typedef struct mtab_s mtab_t;
typedef struct mvec_s mvec_t;

void tabs_init(void) {
    mtab_t deftab;
    mvec_t defvec;

    deftab.distance = -1;
    mtab = tab_create(&deftab, sizeof(mtab_t));

    defvec.cost = -1;
    defvec.online = false;
    defvec.via = -1;
    defvec.distance = -1;
    defvec.inseqno = 0;
    defvec.outseqno = 0;
    mvec = tab_create(&defvec, sizeof(mvec_t));
}

void tabs_resetconn(void) {
    int count = num_destinations();
    for (int i = 0; i < count; i++) {
        set_node_inseqno(i, 0);
        set_node_outseqno(i, 0);
    }
}

void tabs_cleanup(void) {
    tab_free(mvec);
    mvec = NULL;
    tab_free(mtab);
    mtab = NULL;
}


bool is_neighbour(int node) {
    mvec_t *vec;

    assert(node >= 0);
    vec = (mvec_t *)tab_get(mvec, 0, node);
    return vec->online;
}

void drop_neighbour(int node) {
    int count;
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.online = false;
    vec.via = -1;
    vec.distance = -1;
    tab_set(mvec, 0, node, &vec);

    count = num_destinations();

    // apaga tudo o que passa por esse nó
    //for (int n = next_neighbour(-1); n >= 0; n = next_neighbour(n)) {
    for (int i = 0; i < count; i++) {
        if (get_rt_via(i) == node) {
            set_rt_via(i, -1);         // se esse caminha passava pelo vizinho q caiu, faz comq nao passe por nenhum
            set_rt_distance(i, -1);     // coloca distancia infinita
        }
    }
    set_rt_distance(node, -1);
    if (options.show_dropadd) {
        con_printf("dropping node %d\n", node);
    }
}

bool add_neighbour(int node) {
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    if (!DIST_EXISTS(vec.cost)) {
        return false;
    }
    if (vec.online) {
        return false;
    }
    vec.online = true;
    vec.via = node;
    vec.distance = vec.cost;
    tab_set(mvec, 0, node, &vec);
    if (options.show_dropadd) {
        con_printf("adding node %d\n", node);
    }
    return true;
}

int next_neighbour(int node) {
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

void set_neighbour_cost(int node, int cost) {
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.cost = cost;
    tab_set(mvec, 0, node, &vec);
}

int get_neighbour_cost(int node) {
    const mvec_t *vec;

    vec = tab_get(mvec, 0, node);
    return vec->cost;
}

void set_node_inseqno(int node, int seqno) {
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.inseqno = seqno;
    tab_set(mvec, 0, node, &vec);
}

int get_node_inseqno(int node) {
    const mvec_t *vec;

    vec = tab_get(mvec, 0, node);
    return vec->inseqno;
}

void set_node_outseqno(int node, int seqno) {
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.outseqno = seqno;
    tab_set(mvec, 0, node, &vec);
}

int get_node_outseqno(int node) {
    const mvec_t *vec;

    vec = tab_get(mvec, 0, node);
    return vec->outseqno;
}

void set_rt_distance(int node, int dist) {
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.distance = dist;
    tab_set(mvec, 0, node, &vec);
}

int get_rt_distance(int node) {
    const mvec_t *vec;

    vec = tab_get(mvec, 0, node);
    return vec->distance;
}

void set_rt_via(int node, int via) {
    mvec_t vec;

    tab_getCopy(mvec, 0, node, &vec);
    vec.via = via;
    tab_set(mvec, 0, node, &vec);
}

int get_rt_via(int node) {
    const mvec_t *vec;

    vec = tab_get(mvec, 0, node);
    return vec->via;
}

void set_dv_distance(int via, int to, int dist) {
    mtab_t tab;

    tab_getCopy(mtab, via, to, &tab);
    tab.distance = dist;
    tab_set(mtab, via, to, &tab);
}

int get_dv_distance(int via, int to) {
    const mtab_t *tab;

    tab = tab_get(mtab, via, to);
    return tab->distance;
}

int num_destinations(void) {
    return max(tab_cols(mvec), tab_cols(mtab));
}

void print_tabs(void) {

    int i;
    int size;

    size = num_destinations();

    con_printf("  +");
    for (i = 0; i < size; i++) {
        con_printf("----");
    }
    con_printf("+\n");

    con_printf("  |");
    for (i = 0; i < size; i++) {
        con_printf(" %3d", i);
    }
    con_printf("|\n");

    con_printf("  +");
    for (i = 0; i < size; i++) {
        con_printf("----");
    }
    con_printf("+\n");

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

    con_printf("  +");
    for (i = 0; i < size; i++) {
        con_printf("----");
    }
    con_printf("+\n");

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


    con_printf("  +");
    for (i = 0; i < size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
}


