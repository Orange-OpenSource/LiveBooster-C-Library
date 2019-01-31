/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "HeraclesModem.h"
#include "HeraclesTcpClient.h"

#define DEFAULT_TIMEOUT  10000

#define GSM_NL "\r\n"

#define GSM_MUX_COUNT 2

typedef struct _HeraclesModem {
    SerialInterface* serial;
    TimerInterface* timer;
    DebugInterface* debug;
    struct _HeraclesTcpClient* sockets[GSM_MUX_COUNT];
    int prev_check;
} HeraclesModem;

static struct _HeraclesModem modem;

#define GSM_YIELD { }

enum SimStatus {
    SIM_ERROR = 0, SIM_READY = 1, SIM_LOCKED = 2,
};

enum RegStatus {
    REG_UNREGISTERED = 0, REG_SEARCHING = 2, REG_DENIED = 3, REG_OK_HOME = 1, REG_OK_ROAMING = 5, REG_UNKNOWN = 4,
};

static const char* defaultReponses[5] = { "OK" GSM_NL, "ERROR" GSM_NL, 0, 0, 0 };

void HeraclesModem__sendAT(const char * cmdFormat, ...) {
    char buffer[128];

    va_list ap;

    va_start(ap, cmdFormat);
    vsprintf(buffer, cmdFormat, ap);
    va_end(ap);

    modem.serial->write("AT", 2);
    modem.serial->write(buffer, strlen(buffer));
    modem.serial->write(GSM_NL, strlen(GSM_NL));

    GSM_YIELD;
}

int HeraclesModem__readInt() {

	int res;
    char buffer[9];
    char *bufptr = buffer;

    while (!modem.serial->available()) {
        GSM_YIELD;
    }
    char c = modem.serial->get();

    while (((signed char)c >= 0) && (c != ',') && (c != '\n') && (bufptr < buffer + sizeof(buffer)-1)) {
        *bufptr++ = c;

        while (!modem.serial->available()) {
            GSM_YIELD;
        }
        c = modem.serial->get();
   }
    *bufptr = 0;

    sscanf(buffer, "%d", &res);

    return res;
}

unsigned int waitResponse(unsigned long timeout, unsigned int numResponses, ...) {
    char dataBuffer[100];
    char *data = dataBuffer;

    unsigned int i;
    const char *responses[5];

    va_list ap;
    va_start(ap, numResponses);
    for (i = 0; (i < numResponses) && (i < 5); i++) {
        responses[i] = va_arg(ap, const char*);
    }
    va_end(ap);
    for (; i < 5; i++) {
        responses[i] = defaultReponses[i];
    }

    unsigned long startMillis = modem.timer->millis();
    do {
        GSM_YIELD;
        while (modem.serial->available() > 0) {
            char a = modem.serial->get();
            if (a <= 0) {
                continue; // Skip 0x00 bytes, just in case
            }
            // robustness on dataBuffer
            if (strlen(dataBuffer) == sizeof(dataBuffer)) {
               data = dataBuffer;
            }

            *data++ = a;
            *data = 0;
            if (responses[0] && (strstr(dataBuffer, responses[0]) != 0)) {
               return 1;
            }
            else if (responses[1] && (strstr(dataBuffer, responses[1]) != 0)) {
                return 2;
            }
            else if (responses[2] && (strstr(dataBuffer, responses[2]) != 0)) {
                return 3;
            }
            else if (responses[3] && (strstr(dataBuffer, responses[3]) != 0)) {
                return 4;
            }
            else if (responses[4] && (strstr(dataBuffer, responses[4]) != 0)) {
                return 5;
            }
            else if (strstr(dataBuffer, "+CIPRXGET:" GSM_NL) != 0) {
                int mode = HeraclesModem__readInt();
                if (mode == 1) {
                    int mux = HeraclesModem__readInt();
                    if (mux >= 0 && mux < GSM_MUX_COUNT && modem.sockets[mux]) {
                    	modem.prev_check = 0;
                    }
                    data = dataBuffer;
                    *data = 0;
                }
            }
            else if (strstr(dataBuffer, "CLOSED" GSM_NL) != 0) {
                data = dataBuffer;
                *data = 0;
            }
        }
    } while (modem.timer->millis() - startMillis < timeout);

    return 0;
}

int testAT(unsigned long timeout) {
    unsigned long start;
    for (start = modem.timer->millis(); modem.timer->millis() - start < timeout;) {
        HeraclesModem__sendAT("");
        if (waitResponse(200, 0) == 1) {
            GSM_YIELD;
            return 1;
        }
        modem.timer->delay(200);
    }
    return 0;
}

enum SimStatus getSimStatus(unsigned long timeout) {
    unsigned long start;
    for (start = modem.timer->millis(); modem.timer->millis() - start < timeout;) {
        HeraclesModem__sendAT("+CPIN?");
        if (waitResponse(DEFAULT_TIMEOUT, 1, GSM_NL "+CPIN:") != 1) {
        	modem.timer->delay(1000);
            continue;
        }
        int status = waitResponse(DEFAULT_TIMEOUT, 4, "READY", "SIM PIN", "SIM PUK", "NOT INSERTED");
        waitResponse(DEFAULT_TIMEOUT, 0);
        switch (status) {
        case 2:
        case 3:
            return SIM_LOCKED;
        case 1:
            return SIM_READY;
        default:
            return SIM_ERROR;
        }
    }
    return SIM_ERROR;
}

void streamSkipUntil(const char terminator) {
    unsigned long startMillis = modem.timer->millis();
	while (modem.timer->millis() - startMillis < DEFAULT_TIMEOUT) {
        while (!modem.serial->available()) {
            GSM_YIELD;
        }
        if (modem.serial->get() == terminator) {
            break;
        }
    }
}

enum RegStatus getRegistrationStatus() {
    HeraclesModem__sendAT("+CREG?");
    if (waitResponse(DEFAULT_TIMEOUT, 1, GSM_NL "+CREG:") != 1) {
        return REG_UNKNOWN;
    }
    streamSkipUntil(','); // Skip format (0)
    int status = HeraclesModem__readInt();
    waitResponse(DEFAULT_TIMEOUT, 0);
    return (enum RegStatus) status;
}

int isNetworkConnected() {
    enum RegStatus s = getRegistrationStatus();
    return (s == REG_OK_HOME || s == REG_OK_ROAMING);
}

int waitForNetwork() {
    unsigned long start;
    for (start = modem.timer->millis(); modem.timer->millis() - start < 60000L;) {
        if (isNetworkConnected()) {
            return 1;
        }
        modem.timer->delay(250);
    }
    return 0;
}

int attachGPRS() {

    // Set the connection type to GPRS
    HeraclesModem__sendAT("+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
    waitResponse(DEFAULT_TIMEOUT, 0);

    // Activate the PDP context
    HeraclesModem__sendAT("+CGACT=1,1");
    waitResponse(60000, 0);

    // Open the defined GPRS bearer context
    HeraclesModem__sendAT("+SAPBR=1,1");
    waitResponse(85000, 0);

    // Query the GPRS bearer context status
    HeraclesModem__sendAT("+SAPBR=2,1");
    if (waitResponse(30000, 0) != 1) {
        return 0;
    }

    // Attach to GPRS
    HeraclesModem__sendAT("+CGATT=1");
    if (waitResponse(75000, 0) != 1) {
        return 0;
    }

    // Set mode TCP
    HeraclesModem__sendAT("+CIPMODE=0");
    if (waitResponse(DEFAULT_TIMEOUT, 0) != 1) {
        return 0;
    }

    // Set to multiple-IP
    HeraclesModem__sendAT("+CIPMUX=1");
    if (waitResponse(DEFAULT_TIMEOUT, 0) != 1) {
        return 0;
    }

    // Put in "quick send" mode (thus no extra "Send OK")
    HeraclesModem__sendAT("+CIPQSEND=1");
    if (waitResponse(DEFAULT_TIMEOUT, 0) != 1) {
        return 0;
    }

    // Set to get data manually
    HeraclesModem__sendAT("+CIPRXGET=1");
    if (waitResponse(DEFAULT_TIMEOUT, 0) != 1) {
        return 0;
    }

    // Default configuration for Heracles board: just AT+CSTT
    HeraclesModem__sendAT("+CSTT");
    if (waitResponse(60000, 0) != 1) {
        return 0;
    }

    // Bring Up Wireless Connection with GPRS or CSD
    HeraclesModem__sendAT("+CIICR");
    if (waitResponse(60000, 0) != 1) {
        return 0;
    }

    // Get Local IP Address, only assigned after connection
    HeraclesModem__sendAT("+CIFSR;E0");
    if (waitResponse(DEFAULT_TIMEOUT, 0) != 1) {
        return 0;
    }

    // Configure Domain Name Server (DNS)
    HeraclesModem__sendAT("+CDNSCFG=\"8.8.8.8\",\"8.8.4.4\"");
    if (waitResponse(DEFAULT_TIMEOUT, 0) != 1) {
        return 0;
    }

    return 1;
}

int HeraclesModem__Init(SerialInterface* serialItf, TimerInterface* timerItf, DebugInterface* debugItf, int doReset) {

	modem.serial = serialItf;
	modem.timer = timerItf;
	modem.debug = debugItf;

    modem.prev_check = 0;

    modem.serial->open();
    modem.debug->print("Serial interface initialized\n");

    modem.timer->timerInit();
    modem.debug->print("Timer interface initialized\n");

    if (!testAT(DEFAULT_TIMEOUT)) {
        return 0;
    }

    if (doReset) {
    	modem.debug->print("Reset Heracles modem\n");

    	memset(modem.sockets, 0, sizeof(modem.sockets));

        HeraclesModem__sendAT("+CFUN=0");
        if (waitResponse(DEFAULT_TIMEOUT, 0) != 1) {
            return 0;
        }
        HeraclesModem__sendAT("+CFUN=1,1");
        if (waitResponse(DEFAULT_TIMEOUT, 0) != 1) {
            return 0;
        }

        modem.timer->delay(5000);

        HeraclesModem__sendAT("&F0");  // Set all TA parameters to manufacturer defaults
        if (waitResponse(DEFAULT_TIMEOUT, 0) != 1) {
            return 0;
        }
        HeraclesModem__sendAT("E0");   // Echo Off
        if (waitResponse(DEFAULT_TIMEOUT, 0) != 1) {
            return 0;
        }

        getSimStatus(DEFAULT_TIMEOUT);

		HeraclesModem__sendAT("+CLTS=1");  // Enable refresh of time and time zone from network
        if (waitResponse(DEFAULT_TIMEOUT, 0) != 1) {
            return 0;
        }

        if (waitForNetwork() != 1) {
            return 0;
        }

        if (attachGPRS() != 1) {
            return 0;
        }
    }

    return 1; // Success
}

int modemGetConnected(unsigned int mux) {
    HeraclesModem__sendAT("+CIPSTATUS=%d", mux);

    int res = waitResponse(DEFAULT_TIMEOUT, 4, ",\"CONNECTED\"", ",\"CLOSED\"", ",\"CLOSING\"", ",\"INITIAL\"");
    waitResponse(DEFAULT_TIMEOUT, 0);
    return (res == 1);
}

unsigned int modemGetAvailable(unsigned int mux) {
    HeraclesModem__sendAT("+CIPRXGET=4,%d", mux);

    unsigned int result = 0;
    if (waitResponse(DEFAULT_TIMEOUT, 1, "+CIPRXGET:") == 1) {
        streamSkipUntil(','); // Skip mode 4
        streamSkipUntil(','); // Skip mux
        result = HeraclesModem__readInt();
        waitResponse(DEFAULT_TIMEOUT, 0);
    }
    if (!result) {
        modem.sockets[mux]->sock_connected = modemGetConnected(mux);
    }
    return result;
}

void HeraclesModem__Maintain() {
    unsigned int mux;
    if (modem.timer->millis() - modem.prev_check > 500) {
        modem.prev_check = modem.timer->millis();
        for (mux = 0; mux < GSM_MUX_COUNT; mux++) {
            struct _HeraclesTcpClient* sock = modem.sockets[mux];
            if (sock) {
                sock->sock_available = modemGetAvailable(mux);
            }
        }
    }

    while (modem.serial->available()) {
        waitResponse(10, 2, 0, 0);
    }
}

int HeraclesModem__Connect(struct _HeraclesTcpClient* client,
		                   const char* host,
                           unsigned short port,
						   unsigned int *mux,
						   unsigned int sslEnabled) {
     unsigned int freeMux;

	*mux = INVALID_MUX;

	for (freeMux = 0; freeMux < GSM_MUX_COUNT; freeMux++) {
		if (modem.sockets[freeMux] == 0) {
			*mux = freeMux;
			break;
		}
	}

	if (*mux != INVALID_MUX) {
		modem.sockets[*mux] = client;

		HeraclesModem__sendAT("+SSLOPT=0,0"); // enable root certificate
		int rsp = waitResponse(DEFAULT_TIMEOUT, 0);

		HeraclesModem__sendAT("+SSLOPT=1,1"); // enable client authentication
		rsp = waitResponse(DEFAULT_TIMEOUT, 0);

		HeraclesModem__sendAT("+CIPSSL=%d", sslEnabled);
		rsp = waitResponse(DEFAULT_TIMEOUT, 0);
		if (sslEnabled && (rsp != 1)) {
			return 0;
		}

		HeraclesModem__sendAT("+CIPSTART=%d,\"TCP\",\"%s\",%d", *mux, host, port);
		rsp = waitResponse(75000, 5, "CONNECT OK" GSM_NL, "CONNECT FAIL" GSM_NL, "ALREADY CONNECT" GSM_NL,
				"ERROR" GSM_NL, "CLOSE OK" GSM_NL   // Happens when HTTPS handshake fails
				);
		return (1 == rsp);
	}

	return 0;
}

void HeraclesModem__Disconnect(unsigned int mux) {
    HeraclesModem__sendAT("+CIPCLOSE=%d", mux);
    waitResponse(DEFAULT_TIMEOUT, 0);

    modem.sockets[mux] = 0;
}

int HeraclesModem__Send(const unsigned char* buff, int len, unsigned int mux) {
    HeraclesModem__sendAT("+CIPSEND=%d,%d", mux, len);
    if (waitResponse(DEFAULT_TIMEOUT, 1, ">") != 1) {
        return -1;
    }
    modem.serial->write((const char*)buff, len);
    if (waitResponse(DEFAULT_TIMEOUT, 1, "DATA ACCEPT:") != 1) {
    	return -1;
    }
    streamSkipUntil(','); // Skip mux
    return  HeraclesModem__readInt();
}

int HeraclesModem__Read(int size, unsigned int mux) {
    int i;
    HeraclesModem__sendAT("+CIPRXGET=2,%d,%d", mux, size);
    if (waitResponse(DEFAULT_TIMEOUT, 1, "+CIPRXGET:") != 1) {
        return 0;
    }

    streamSkipUntil(','); // Skip mode 2
    streamSkipUntil(','); // Skip mux
    int len = HeraclesModem__readInt();
    modem.sockets[mux]->sock_available = HeraclesModem__readInt();

    for (i = 0; i < len; i++) {
        while (!modem.serial->available()) {
            GSM_YIELD;
        }
        char c = modem.serial->get();
        GsmFifo_Put(&modem.sockets[mux]->rx, c);
    }
    waitResponse(DEFAULT_TIMEOUT, 0);
    return len;
}
