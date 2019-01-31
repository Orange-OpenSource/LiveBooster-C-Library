/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the 'BSD-3-Clause'
 * license which can be found in the file 'LICENSE.txt' in this package distribution
 * or at 'https://opensource.org/licenses/BSD-3-Clause'.
 */

/**
 * @file   LiveBooster_http.h
 * @brief  Interface to get a resource using HTTP GET request
 *
 */

#ifndef __LiveBooster_http_H_
#define __LiveBooster_http_H_

#include "../../heraclesGsm/HeraclesTcpClient.h"
#include "../../serial/SerialInterface.h"
#include "../../timer/TimerInterface.h"

#include <stdint.h>
#include <inttypes.h>

#if defined(__cplusplus)
extern "C" {
#endif

void LiveBooster_http_init(SerialInterface* serial, TimerInterface* timer, DebugInterface *debug);

int LiveBooster_http_start(const char* uri, uint32_t rsc_size, uint32_t rsc_offset);

int  LiveBooster_http_data(char* pData, int len);

void LiveBooster_http_close(void);


#if defined(__cplusplus)
}
#endif

#endif /* __LiveBooster_http_H_ */
