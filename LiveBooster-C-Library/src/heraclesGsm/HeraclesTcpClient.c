/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#include "HeraclesTcpClient.h"

/**
 * interface implementation
 */

/*
 * @startuml
 *    hide footbox
 *    participant UserApp as "Upper software\nlayer"
 *    participant TcpClient as "HeraclesTcpClient"
 *    participant HeraclesModem as "HeraclesModem"
 *    UserApp -> TcpClient : connect(<host>, <port>)
 *    TcpClient -> HeraclesModem : modemConnect(<host>, <port>, \n\t\t\t<mux>, <ssl>)
 *    HeraclesModem -> Serial : "AT+CIPSSL=<ssl>"
 *    note right : Enable or disable SSL function
 *    HeraclesModem <-- Serial : "OK"
 *    HeraclesModem -> Serial : "AT+CIPSTART=<mux>,TCP,\n\t\t\t<host>,<port>"
 *    note right : Start up the connection
 *    HeraclesModem <-- Serial : "CONNECT OK"
 *    note left : The TCP connection has been established successfully.\nSSL certificate handshake finished.
 *    TcpClient <-- HeraclesModem : status
 *    UserApp <-- TcpClient : status
 * @enduml
 */
int HeraclesTcpClient__Connect(struct _TcpClientInterface* const obj, const char *host, unsigned short port,  unsigned int sslEnabled) {
    struct _HeraclesTcpClient* const self = (struct _HeraclesTcpClient* const) obj;
    GsmFifo_Clear(&self->rx);

    self->sock_connected = HeraclesModem__Connect(self, host, port, &self->mux, sslEnabled);
    return self->sock_connected;
}

/*
 * @startuml
 *    hide footbox
 *    participant UserApp as "Upper software\nlayer"
 *    participant TcpClient as "HeraclesTcpClient"
 *    participant HeraclesModem as "HeraclesModem"
 *    UserApp -> TcpClient : stop()
 *    TcpClient -> HeraclesModem : disconnect()
 *    HeraclesModem -> Serial : "AT+CIPCLOSE=<mux>"
 *    note right : Close connection
 *    HeraclesModem <-- Serial : "CLOSE OK"
 *    TcpClient <-- HeraclesModem : "CLOSE OK"
 * @enduml
 */
void HeraclesTcpClient__Stop(struct _TcpClientInterface* const obj) {
    struct _HeraclesTcpClient* const self = (struct _HeraclesTcpClient* const) obj;
    HeraclesModem__Disconnect(self->mux);
    self->sock_connected = 0;
    GsmFifo_Clear(&self->rx);
}

/*
 * @startuml
 *    hide footbox
 *    participant UserApp as "Upper software\nlayer"
 *    participant TcpClient as "HeraclesTcpClient"
 *    participant HeraclesModem as "HeraclesModem"
 *    UserApp -> TcpClient : write(<buf>, <size>)
 *    TcpClient -> HeraclesModem : maintain()
 *    TcpClient -> HeraclesModem : send(<buf>, <size>, <mux>)
 *    HeraclesModem -> Serial : "AT+CIPSEND=<mux>,<size>"
 *    note right : Write command
 *    HeraclesModem <-- Serial : ">"
 *    HeraclesModem -> Serial : write(<buf>, <size>)
 *    note right : Provide data to send
 *    HeraclesModem -> Serial : flush()
 *    HeraclesModem <-- Serial : "DATA ACCEPT:"
 *    note left : Sending is successful
 *    TcpClient <-- HeraclesModem : status
 *    UserApp <-- TcpClient : status
 * @enduml
 */
int HeraclesTcpClient__Write(struct _TcpClientInterface* const obj, const unsigned char *data, int size) {
    struct _HeraclesTcpClient* const self = (struct _HeraclesTcpClient* const) obj;
    HeraclesModem__Maintain();
    return HeraclesModem__Send(data, size, self->mux);
}

int HeraclesTcpClient__Available(struct _TcpClientInterface* const obj) {
    struct _HeraclesTcpClient* const self = (struct _HeraclesTcpClient* const) obj;
    if (!GsmFifo_Size(&self->rx) && self->sock_connected) {
        HeraclesModem__Maintain();
    }

    return GsmFifo_Size(&self->rx) + self->sock_available;
}

/*
 * @startuml
 *    hide footbox
 *    participant UserApp as "Upper software\nlayer"
 *    participant TcpClient as "HeraclesTcpClient"
 *    participant HeraclesModem as "HeraclesModem"
 *    UserApp -> TcpClient : read(<buf>, size)
 *    TcpClient -> HeraclesModem : maintain()
 *    TcpClient -> HeraclesModem : read(...)
 *    HeraclesModem -> Serial : "AT+CIPRXGET=2,<mux>,<size>"
 *    note right : Get Data from Network Manually
 *    loop while a char is available
 *      HeraclesModem -> Serial : Serial.read()
 *      HeraclesModem <-- Serial : received char
 *    end loop
 *    HeraclesModem <-- Serial : "OK"
 *    TcpClient <-- HeraclesModem : number of bytes read
 *    UserApp <-- TcpClient : number of bytes read
 * @enduml
 */
int HeraclesTcpClient__Read(struct _TcpClientInterface* const obj, unsigned char *buffer, int maxSize, int timeoutInMs) {
    struct _HeraclesTcpClient* const self = (struct _HeraclesTcpClient* const) obj;

    HeraclesModem__Maintain();

    int cnt = 0;
    unsigned long start = self->timer->millis();
    while (cnt < maxSize) {
        if (self->timer->millis() - start > timeoutInMs) {
        	break;
        }
        int chunk = (GsmFifo_Size(&self->rx) < (maxSize - cnt)) ? GsmFifo_Size(&self->rx) : (maxSize - cnt);
        if (chunk > 0) {
            GsmFifo_Get(&self->rx, buffer, chunk);
            buffer += chunk;
            cnt += chunk;
            continue;
        }
        HeraclesModem__Maintain();
        if (self->sock_available > 0) {
            HeraclesModem__Read(GsmFifo_FreeSize(&self->rx), self->mux);
        }
        else {
            break;
        }
    }
    return cnt;
}

int HeraclesTcpClient__Connected(struct _TcpClientInterface* const obj) {
    struct _HeraclesTcpClient* const self = (struct _HeraclesTcpClient* const) obj;
    if (HeraclesTcpClient__Available(obj)) {
        return 1;
    }

    return self->sock_connected;
}

/**
 * public initializer
 */

void HeraclesTcpClient__Init(struct _HeraclesTcpClient* client,
							 SerialInterface* serialItf,
							 TimerInterface* timerItf,
							 DebugInterface* debugItf,
							 int doReset) {

	/* Heracles Modem initialisation */
    int res = HeraclesModem__Init(serialItf, timerItf, debugItf, doReset);
    debugItf->print("HeraclesModem  ");
    res==1 ? debugItf->print("initialized\n") :debugItf->print("not initialized\n");

    /* Interface implementation */
    client->_ = (struct _TcpClientInterface) {
        HeraclesTcpClient__Connect,
        HeraclesTcpClient__Stop,
        HeraclesTcpClient__Connected,
        HeraclesTcpClient__Available,
        HeraclesTcpClient__Read,
        HeraclesTcpClient__Write
    };

    /* Private attributes initialization */
    client->mux = INVALID_MUX;
    client->sock_available = 0;
    client->sock_connected = 0;
    client->debug = debugItf;
    client->timer = timerItf;

}
