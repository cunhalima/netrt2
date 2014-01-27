#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "defs.h"

// vetor redimensionável simples

struct szb_s {
    int pos;
    int size;
    int capacity;
    char *data;
};

#define GRANULARITY     256
#define GRANMASK        0xFFFFFF00

#define ensure_read(buf, s) \
    if (buf->pos + s > buf->size) { \
        return -1; \
    }

static void need_space(szb_t *buf, int space) {
    int npos = buf->pos + space;
    if (npos > buf->size) {
        szb_resize(buf, npos);
    }
}

szb_t *szb_create() {
    szb_t *buf = malloc(sizeof(*buf));
    if (buf == NULL) {
        return buf;
    }
    buf->pos = 0;
    buf->size = 0;
    buf->capacity = 0;
    buf->data = NULL;
    return buf;
}

void szb_free(szb_t *buf) {
    assert(buf != NULL);
    free(buf->data);
    free(buf);;
}

void szb_write8(szb_t *buf, int v) {
    assert(buf != NULL);
    need_space(buf, 1);
    buf->data[buf->pos] = v;
    buf->pos += 1;
}

void szb_write16(szb_t *buf, int v) {
    assert(buf != NULL);
    szb_write8(buf, v & 0xFF);
    szb_write8(buf, (v >> 8) & 0xFF);
}

void szb_write32(szb_t *buf, int v) {
    assert(buf != NULL);
    szb_write8(buf, v & 0xFF);
    szb_write8(buf, (v >> 8) & 0xFF);
    szb_write8(buf, (v >> 16) & 0xFF);
    szb_write8(buf, (v >> 24) & 0xFF);
}

void szb_writeCStr(szb_t *buf, const char *str) {
    int len;

    assert(buf != NULL);
    len = strlen(str) + 1;
    need_space(buf, len);
    memcpy(&buf->data[buf->pos], str, len);
    buf->pos += len;
}

int szb_read8(szb_t *buf) {
    int r;

    assert(buf != NULL);
    ensure_read(buf, 1);
    r = buf->data[buf->pos];
    buf->pos += 1;
    return r;
}

int szb_read16(szb_t *buf) {
    int r;

    assert(buf != NULL);
    r = szb_read8(buf) & 0xFF;
    r |= (szb_read8(buf) << 8) & 0xFF00;
    return r;
}

int szb_read32(szb_t *buf) {
    int r;

    assert(buf != NULL);
    r = szb_read8(buf) & 0xFF;
    r |= (szb_read8(buf) << 8) & 0xFF00;
    r |= (szb_read8(buf) << 16) & 0xFF0000;
    r |= (szb_read8(buf) << 24) & 0xFF000000;
    return r;
}

void szb_print(szb_t *buf) {
    int i;

    assert(buf != NULL);
    for (i = 0; i < buf->size; i++) {
        printf(" %02X", buf->data[i]);
    }
    printf("\n");
}

void szb_rewind(szb_t *buf) {
    assert(buf != NULL);
    buf->pos = 0;
}

void szb_setCapacity(szb_t *buf, int capacity) {
    assert(buf != NULL);
    assert(capacity >= 0);
    capacity = (capacity + GRANULARITY - 1) & GRANMASK;
    if (capacity != buf->capacity) {
        char *data;

        data = realloc(buf->data, capacity);
        if (data == NULL) {
            free(buf->data);
        }
        buf->data = data;
        buf->capacity = capacity;
    }
}

void szb_resize(szb_t *buf, int size) {
    assert(buf != NULL);
    assert(size >= 0);
    if (size > buf->capacity) {
        szb_setCapacity(buf, size);
    }
    buf->size = size;
    if (buf->pos > buf->size) {
        buf->pos = buf->size;
    }
}

void *szb_getPtr(szb_t *buf) {
    assert(buf != NULL);
    return buf->data;
}

void *szb_getPosPtr(szb_t *buf) {
    assert(buf != NULL);
    if (buf->pos < 0 || buf->pos >= buf->size) {
        return NULL;
    }
    return &buf->data[buf->pos];
}

int szb_getSize(szb_t *buf) {
    assert(buf != NULL);
    return buf->size;
}

bool szb_eof(szb_t *buf) {
    assert(buf != NULL);
    if (buf->pos >= buf->size) {
        return true;
    }
    return false;
}

void szb_clear(szb_t *buf) {
    assert(buf != NULL);
    buf->size = 0;
    buf->pos = 0;
}
