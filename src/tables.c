#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "defs.h"

// tabela redimensionável

struct table_s {
    int itemsize;
    int rows, cols;
    char *data;
    char *newitem;
};

int tab_rows(table_t *tab) {
    assert(tab != NULL);
    return tab->rows;
}

int tab_cols(table_t *tab) {
    assert(tab != NULL);
    return tab->cols;
}

table_t *tab_create(void *def, int itemsize) {
    table_t *tab;

    tab = calloc(1, sizeof(*tab));
    tab->newitem = calloc(1, itemsize);
    if (def != NULL) {
        memcpy(tab->newitem, def, itemsize);
    }
    tab->itemsize = itemsize;
    return tab;
}

void tab_free(table_t *tab) {
    assert(tab != NULL);
    free(tab->data);
    free(tab->newitem);
    free(tab);
}

void tab_resize(table_t *tab, int i, int j) {
    int nrows, ncols;
    char *ndata;

    assert(tab != NULL);
    nrows = max(i + 1, tab->rows);
    ncols = max(j + 1, tab->cols);
    ndata = malloc(nrows * ncols * tab->itemsize);
    for (i = 0; i < nrows; i++) {
        for (j = 0; j < ncols; j++) {
            char *src;
            if (i < tab->rows && j < tab->cols) {
                src = &tab->data[(i * tab->cols + j) * tab->itemsize];
            } else {
                src = tab->newitem;
            }
            memcpy(&ndata[(i * ncols + j) * tab->itemsize], src, tab->itemsize);
        }
    }
    free(tab->data);
    tab->data = ndata;
    tab->rows = nrows;
    tab->cols = ncols;
}

const void *tab_get(table_t *tab, int i, int j) {
    assert(tab != NULL);
    if (i < 0 || j < 0 || i >= tab->rows || j >= tab->cols) {
        return tab->newitem;
    }
    return &tab->data[(i * tab->cols + j) * tab->itemsize];
}

void tab_getCopy(table_t *tab, int i, int j, void *data) {
    const void *src;
    assert(tab != NULL);
    src = tab_get(tab, i, j);
    memcpy(data, src, tab->itemsize);
}

void tab_set(table_t *tab, int i, int j, const void *item) {
    assert(tab != NULL);
    if (i < 0 || j < 0) {
        return;
    }
    if (i >= tab->rows || j >= tab->cols) {
        tab_resize(tab, i, j);
    }
    memcpy(&tab->data[(i * tab->cols + j) * tab->itemsize], item, tab->itemsize);
}
