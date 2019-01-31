/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#ifndef __HeraclesModem_h
#define __HeraclesModem_h

#include "../serial/SerialInterface.h"
#include "../timer/TimerInterface.h"
#include "../traceDebug/DebugInterface.h"
#include "HeraclesTcpClient.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_MUX  255

/**
 * Initialize modem instance, optionally including restarting of Heracles modem.
 * Return 1 on operation success, else 0.
 */
int HeraclesModem__Init(SerialInterface* serialItf, TimerInterface* timerItf, DebugInterface* debugItf, int doReset);

/**
 * Maintain opened connections state. Shall be called periodically, and before any read() sequence.
 */
void HeraclesModem__Maintain();

/**
 * Open connection to host:port, optionally enabling SSL.
 * Index 'mux' (used internally to manage simultaneous different TCP sessions) is set to the 1st free index.
 * Return 1 if success, else 0.
 */
int HeraclesModem__Connect(struct _HeraclesTcpClient* const client, const char* host, unsigned short port, unsigned int *mux, unsigned int sslEnabled);

/**
 * Disconnect connection for index mux.
 */
void HeraclesModem__Disconnect(unsigned int mux);

/**
 * Send data to server.
 */
int HeraclesModem__Send(const unsigned char* buff, int len, unsigned int mux);

/**
 * Get data from server.
 */
int HeraclesModem__Read(int size, unsigned int mux);

#ifdef __cplusplus
}
#endif

#endif
