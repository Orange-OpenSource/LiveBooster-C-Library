/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the 'BSD-3-Clause'
 * license which can be found in the file 'LICENSE.txt' in this package distribution
 * or at 'https://opensource.org/licenses/BSD-3-Clause'.
 */

/**
 * @file   A very popular way to encode binary data is Base64.
 *         The basis of this is an encoding table. As you might expect,
 *         there are 64 total characters that go into the tale.
 *         There are multiple implementations of base64 with slight differences.
 *         They are all the same except for the last two characters and line ending requirements.
 *         The first 62 characters are always “ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789”.
 *         PEM and MIME encoding are the most common and use “+/” as the last two characters.
 *         PEM and MIME may use the same characters but they have different maximum line lengths.
 *
 * @brief  A simple Base64 Encode and Decode Algorithm implementation.
 */

#ifndef __LiveBooster_code_b64_H_
#define __LiveBooster_code_b64_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include <stddef.h>

void b64_encode(const char *in, char *out, size_t len);
int b64_decode(const char *in, char *out, size_t outlen);

#if defined(__cplusplus)
}
#endif

#endif /* __LiveBooster_code_b64_H_ */
