/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#ifndef __GsmFifo_h
#define __GsmFifo_h

#ifdef __cplusplus
extern "C" {
#endif

#define FIFO_SIZE 64

typedef struct _GsmFifo {
    unsigned char _b[FIFO_SIZE];
    int _w;
    int _r;
} GsmFifo;

/**
 * Clear the FIFO.
 */
void GsmFifo_Clear(struct _GsmFifo* const obj);

/**
 * Return the used size in the FIFO.
 */
int GsmFifo_Size(struct _GsmFifo* const obj);

/**
 * Return the available (free) size in the FIFO.
 */
int GsmFifo_FreeSize(struct _GsmFifo* const obj);

/**
 * Put char c in the FIFO.
 */
int GsmFifo_Put(struct _GsmFifo* const obj, const unsigned char c);

/**
 * Get n chars from the FIFO.
 */
int GsmFifo_Get(struct _GsmFifo* const obj, unsigned char* p, const int n);

#ifdef __cplusplus
}
#endif

#endif
