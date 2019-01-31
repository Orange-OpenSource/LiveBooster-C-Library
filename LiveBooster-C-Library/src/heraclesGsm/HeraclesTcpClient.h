/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#ifndef __HeraclesTcpClient_h
#define __HeraclesTcpClient_h

#include "GsmFifo.h"
#include "HeraclesModem.h"
#include "TcpClientInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @startuml
 *
 * interface TcpClient {
 *    [...]
 * }
 *
 * class HeraclesTcpClient {
 *    +int connect (host, port)
 *    +void stop ()
 *    +int connected ()
 *    +int available ()
 *    +int read (buffer, maxSize, timeoutInMs)
 *    +int write (data, size, timeoutInMs)
 *    -at : HeraclesModem
 *    -rx : GsmFifo
 * }
 * TcpClient <|-- HeraclesTcpClient
 *
 * class HeraclesModem {
 *    +init()
 *    +maintain()
 *    +connect()
 *    +disconnect()
 *    +send()
 *    +read()
 * }
 * HeraclesTcpClient "0..2" --o "1" HeraclesModem
 *
 * interface serial
 * serial "1" -down-o HeraclesModem
 *
 * @enduml
 */

typedef struct _HeraclesTcpClient {

    /* public */
    struct _TcpClientInterface _;

    /* private */
    unsigned int mux;
    unsigned int sock_available;
    int sock_connected;
    GsmFifo rx;
    DebugInterface* debug;
    TimerInterface* timer;
} HeraclesTcpClient;

void HeraclesTcpClient__Init(struct _HeraclesTcpClient* client,
							 SerialInterface* serialItf,
							 TimerInterface* timerItf,
							 DebugInterface* debugItf,
							 int doReset);

#ifdef __cplusplus
}
#endif

#endif
