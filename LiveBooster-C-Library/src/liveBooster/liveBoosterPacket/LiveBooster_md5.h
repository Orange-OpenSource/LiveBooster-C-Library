/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the 'BSD-3-Clause'
 * license which can be found in the file 'LICENSE.txt' in this package distribution
 * or at 'https://opensource.org/licenses/BSD-3-Clause'.
 */

/**
 * @file   This is an OpenSSL-compatible implementation of the RSA Data Security,
 *         Inc. MD5 Message-Digest Algorithm (RFC 1321).
 *         Written by Solar Designer <solar at openwall.com> in 2001, and placed
 *         in the public domain. There's absolutely no warranty.
 *
 *         This differs from Colin Plumb's older public domain implementation in
 *         that no 32-bit integer data type is required, there's no compile-time
 *         endianness configuration, and the function prototypes match OpenSSL's.
 *         The primary goals are portability and ease of use.
 *
 *         This implementation is meant to be fast, but not as fast as possible.
 *         Some known optimizations are not included to reduce source code size
 *         and avoid compile-time configuration.
 *
 * @brief  A simple MD5 Algorithm implementation.
 */

#ifndef __LiveBooster_md5_H_
#define __LiveBooster_md5_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include <stdint.h>

typedef struct {
    uint32_t lo, hi;
    uint32_t a, b, c, d;
    unsigned char buffer[64];
    uint32_t block[16];
} md5_context_t;

void MD5Init(md5_context_t *ctx);

void MD5Update(md5_context_t *ctx, const void *data, size_t size);

void MD5Final(unsigned char *result, md5_context_t *ctx);

#if defined(__cplusplus)
}
#endif

#endif /* __LiveBooster_md5_H_ */
