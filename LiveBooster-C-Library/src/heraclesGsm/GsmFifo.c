/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#include "GsmFifo.h"
#include <string.h>

int _inc(int i, int n) {
    return (i + n) % FIFO_SIZE;
}

void GsmFifo_Clear(struct _GsmFifo* const obj) {
    obj->_r = 0;
    obj->_w = 0;
}

int GsmFifo_Size(struct _GsmFifo* const obj) {
    int s = obj->_w - obj->_r;
    if (s < 0) {
        s += FIFO_SIZE;
    }
    return s;
}

int GsmFifo_FreeSize(struct _GsmFifo* const obj) {
    int s = obj->_r - obj->_w;
    if (s <= 0) {
        s += FIFO_SIZE;
    }
    return s - 1;
}

int GsmFifo_Put(struct _GsmFifo* const obj, const unsigned char c) {
    int i = obj->_w;
    int j = i;
    i = _inc(i, 1);
    if (i == obj->_r) {
        return 0;
    }
    obj->_b[j] = c;
    obj->_w = i;
    return 1;
}

int GsmFifo_Get(struct _GsmFifo* const obj, unsigned char* p, int n) {
    int c = n;
    while (c) {
        int f = GsmFifo_Size(obj);
        if (!f) {
            return n - c; // fifo is empty
        }
        // check available data
        if (c < f) {
            f = c;
        }
        int r = obj->_r;
        int m = FIFO_SIZE - r;
        // check wrap
        if (f > m) {
            f = m;
        }
        memcpy(p, &(obj->_b[r]), f);
        obj->_r = _inc(r, f);
        c -= f;
        p += f;
    }
    return n - c;
}

