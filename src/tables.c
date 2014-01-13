#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

static int *dvtab;
static int dvt_size;
static bool dvt_changed;

static int *nhtab;
static int nht_size;

static int *fdtab;
static int fdt_size;

static bool *dntab;
static int dnt_size;

void tab_init(void) {
    dvtab = NULL;
    dvt_size = 0;
    dvt_changed = false;
    nhtab = NULL;
    nht_size = 0;
    fdtab = NULL;
    fdt_size = 0;
    dntab = NULL;
    dnt_size = 0;
}

void tab_cleanup(void) {
    free(dvtab);
    dvtab = NULL;
    dvt_size = 0;
    free(nhtab);
    nhtab = NULL;
    nht_size = 0;
    free(fdtab);
    fdtab = NULL;
    fdt_size = 0;
    free(dntab);
    dntab = NULL;
    dnt_size = 0;
}

/* Distance-Vector Table */

int tab_dvget(int to) {
    assert(to >= 0);
    if (to >= dvt_size) {
        return -1;
    }
    return dvtab[to];
}

void tab_dvset(int to, int value) {
    assert(to >= 0);
    if (to == options.id) {
        return;
    }
    if (to >= dvt_size) {
        int newsize = dvt_size;
        int *newtab;
        int i;

        if (to >= newsize) {
            newsize = to + 1;
        }
        newtab = malloc(sizeof(*newtab) * newsize);
        for (i = 0; i < newsize; i++) {
            if (i < dvt_size) {
                newtab[i] = dvtab[i];
            } else {
                newtab[i] = -1;
            }
        }
        free(dvtab);
        dvtab = newtab;
        dvt_size = newsize;
    }
    if (dvtab[to] != value) {
        dvt_changed = true;
    }
    dvtab[to] = value;
}

void tab_dvprint(void) {
    int i;

    con_printf("DV:\n");
    con_printf("+");
    for (i = 0; i < dvt_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
    con_printf("|");
    for (i = 0; i < dvt_size; i++) {
        con_printf(" %3d", i);
    }
    con_printf("|\n");
    con_printf("+");
    for (i = 0; i < dvt_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
    con_printf("|");
    for (i = 0; i < dvt_size; i++) {
        int v = tab_dvget(i);
        if (v >= 0) {
            con_printf(" %3d", v);
        } else {
            con_printf("   -");
        }
    }
    con_printf("|\n");
    con_printf("+");
    for (i = 0; i < dvt_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
}

void tab_dvvalidate(void) {
    dvt_changed = false;
}

bool tab_dvchanged(void) {
    return dvt_changed;
}

int tab_dvsize(void) {
    return dvt_size;
}

/* Next-hop Table */

int tab_nhget(int to) {
    assert(to >= 0);
    if (to >= nht_size) {
        return -1;
    }
    return nhtab[to];
}

void tab_nhset(int to, int value) {
    assert(to >= 0);
    if (to == options.id) {
        return;
    }
    if (to >= nht_size) {
        int newsize = nht_size;
        int *newtab;
        int i;

        if (to >= newsize) {
            newsize = to + 1;
        }
        newtab = malloc(sizeof(*newtab) * newsize);
        for (i = 0; i < newsize; i++) {
            if (i < nht_size) {
                newtab[i] = nhtab[i];
            } else {
                newtab[i] = -1;
            }
        }
        free(nhtab);
        nhtab = newtab;
        nht_size = newsize;
    }
    nhtab[to] = value;
}

void tab_nhprint(void) {
    int i;

    con_printf("RT:\n");
    con_printf("+");
    for (i = 0; i < nht_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
    con_printf("|");
    for (i = 0; i < nht_size; i++) {
        con_printf(" %3d", i);
    }
    con_printf("|\n");
    con_printf("+");
    for (i = 0; i < nht_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
    con_printf("|");
    for (i = 0; i < nht_size; i++) {
        int v = tab_nhget(i);
        if (v >= 0) {
            con_printf(" %3d", v);
        } else {
            con_printf("   -");
        }
    }
    con_printf("|\n");
    con_printf("+");
    for (i = 0; i < nht_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
}

int tab_nhsize(void) {
    return nht_size;
}
/* Fixed-distance Table */

int tab_fdget(int to) {
    assert(to >= 0);
    if (to >= fdt_size) {
        return -1;
    }
    return fdtab[to];
}

void tab_fdset(int to, int value) {
    assert(to >= 0);
    if (to == options.id) {
        return;
    }
    if (to >= fdt_size) {
        int newsize = fdt_size;
        int *newtab;
        int i;

        if (to >= newsize) {
            newsize = to + 1;
        }
        newtab = malloc(sizeof(*newtab) * newsize);
        for (i = 0; i < newsize; i++) {
            if (i < fdt_size) {
                newtab[i] = fdtab[i];
            } else {
                newtab[i] = -1;
            }
        }
        free(fdtab);
        fdtab = newtab;
        fdt_size = newsize;
    }
    fdtab[to] = value;
}

void tab_fdprint(void) {
    int i;

    con_printf("+");
    for (i = 0; i < fdt_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
    con_printf("|");
    for (i = 0; i < fdt_size; i++) {
        con_printf(" %3d", i);
    }
    con_printf("|\n");
    con_printf("+");
    for (i = 0; i < fdt_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
    con_printf("|");
    for (i = 0; i < fdt_size; i++) {
        int v = tab_fdget(i);
        if (v >= 0) {
            con_printf(" %3d", v);
        } else {
            con_printf("   -");
        }
    }
    con_printf("|\n");
    con_printf("+");
    for (i = 0; i < fdt_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
}

int tab_fdsize(void) {
    return fdt_size;
}

/* Down Table */

bool tab_dnget(int to) {
    assert(to >= 0);
    if (to >= dnt_size) {
        return true;
    }
    return dntab[to];
}

void tab_dnset(int to, bool value) {
    assert(to >= 0);
    if (to == options.id) {
        return;
    }
    if (to >= dnt_size) {
        int newsize = dnt_size;
        bool *newtab;
        int i;

        if (to >= newsize) {
            newsize = to + 1;
        }
        newtab = malloc(sizeof(*newtab) * newsize);
        for (i = 0; i < newsize; i++) {
            if (i < dnt_size) {
                newtab[i] = dntab[i];
            } else {
                newtab[i] = true;
            }
        }
        free(dntab);
        dntab = newtab;
        dnt_size = newsize;
    }
    dntab[to] = value;
}

void tab_dnprint(void) {
    int i;

    con_printf("+");
    for (i = 0; i < dnt_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
    con_printf("|");
    for (i = 0; i < dnt_size; i++) {
        con_printf(" %3d", i);
    }
    con_printf("|\n");
    con_printf("+");
    for (i = 0; i < dnt_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
    con_printf("|");
    for (i = 0; i < dnt_size; i++) {
        bool v = tab_dnget(i);
        if (v) {
            con_printf(" yes", v);
        } else {
            con_printf("  no");
        }
    }
    con_printf("|\n");
    con_printf("+");
    for (i = 0; i < dnt_size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
}

int tab_dnsize(void) {
    return dnt_size;
}

void tab_dropnode(int node) {
    tab_dnset(node, true);
    for (int i = 0; i < nht_size; i++) {
        int next = tab_nhget(i);
        if (next == node) {
            tab_dvset(i, -1);
            tab_nhset(i, -1);
            if (is_neighbour(i)) {
                tab_dvset(i, tab_fdget(i));
                tab_nhset(i, i);
            }
        }
    }
}

bool is_neighbour(int node) {
    assert(node >= 0);
    return !tab_dnget(node);
    //return DIST_EXISTS(tab_dvget(node)) && DIST_EXISTS(tab_nhget(node));
    //return DIST_EXISTS(tab_dvget(node)) && DIST_EXISTS(tab_nhget(node));
}

int next_neighbour(int node) {
    assert(node >= -1);
    for (int i = node + 1; i < nht_size; i++) {
        if (is_neighbour(i)) {
            return i;
        }
    }
    return -1;
}

/* ALL TABS */
void tab_allprint(void) {
    int i;
    int size;


    size = dnt_size;
    if (nht_size > size) {
        size = nht_size;
    }
    if (dvt_size > size) {
        size = dvt_size;
    }

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
    for (i = 0; i < dvt_size; i++) {
        int v = tab_dvget(i);
        if (v >= 0) {
            con_printf(" %3d", v);
        } else {
            con_printf("   -");
        }
    }
    con_printf("|\n");

    con_printf("N |");
    for (i = 0; i < nht_size; i++) {
        int v = tab_nhget(i);
        if (v >= 0) {
            con_printf(" %3d", v);
        } else {
            con_printf("   -");
        }
    }
    con_printf("|\n");

    con_printf("S |");
    for (i = 0; i < size; i++) {
        bool v = tab_dnget(i);
        if (v) {
            con_printf("   -", v);
        } else {
            con_printf("  up");
        }
    }
    con_printf("|\n");

    con_printf("  +");
    for (i = 0; i < size; i++) {
        con_printf("----");
    }
    con_printf("+\n");
}


