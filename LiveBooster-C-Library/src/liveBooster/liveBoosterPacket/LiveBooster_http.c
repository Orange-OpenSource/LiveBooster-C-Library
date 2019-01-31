/*
 * Copyright (C) 2016 Orange
 *
 * This software is distributed under the terms and conditions of the 'BSD-3-Clause'
 * license which can be found in the file 'LICENSE.txt' in this package distribution
 * or at 'https://opensource.org/licenses/BSD-3-Clause'.
 */

/**
 * @file   LiveBooster_http.c
 * @brief Very simple and dirty implementation of HTTP Get
 */

#include <string.h>
#include <stdio.h>

#include "LiveBooster_config.h"
#include "LiveBooster_defs.h"
#include "LiveBooster_http.h"

#define TIMEOUT_IN_MS                500
#define HTTP_SERV_PORT               80
#define HTTP_USER_AGENT              "LiveBooster"
#define HTTP_HD_CONTENT_LENGTH       "Content-Length:"
#define HTTP_HD_CONTENT_RANGE        "Content-Range:"

static HeraclesTcpClient heraclesTcpClient;
static TcpClientInterface *tcpLayer = (TcpClientInterface*)&heraclesTcpClient;
static TimerInterface* httpTimer;

static char httpBuf[400];

void LiveBooster_http_init(SerialInterface* serial, TimerInterface* timer, DebugInterface *debug) {

    HeraclesTcpClient__Init(&heraclesTcpClient, serial, timer, debug, 0);
    httpTimer = timer;
}


/* --------------------------------------------------------------------------------- */
/*  */
static void http_build_get_query(char* buf_ptr, int buf_len, const char* pURL, const char* pHost, uint32_t offset) {
	int rc;
	char* pc = buf_ptr;
	const char *tpl = "GET /%s HTTP/1.0\r\n"
			"Host: %s\r\n"
			"User-Agent: " HTTP_USER_AGENT "\r\n"
			"Connection: keep-alive\r\n";

	if (pURL[0] == '/') {
		pURL = pURL + 1;
	}

	rc = snprintf(pc, buf_len, tpl, pURL, pHost);
	pc += rc;
	buf_len -= rc;
	if (offset > 0) {
		/* rc = snprintf(pc, buf_len, "Range: bytes=%"PRIu32"-\r\n\r\n", offset); */
		;
	}
	else {
		rc = snprintf(pc, buf_len, "\r\n");
	}
	pc += rc;
	*pc = 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
int read_line(char* buf_ptr, int buf_len) {
	int len = 0;
	int ret;
	short retry = 0;
	unsigned char cc;

	if ((buf_ptr == NULL) || (buf_len <= 0)) {
		return ERR_LB_HTTP_READ_LINE_NULL;
	}

	while (1) {
		if (tcpLayer->connected(tcpLayer)) {
			ret = tcpLayer->read(tcpLayer, &cc, 1, TIMEOUT_IN_MS);

			if (ret == 0) {
				if (++retry < 20) {
					httpTimer->delay(200);
					continue;
				}
				else {
					break;
				}
			}

			retry = 0;
			buf_ptr[len++] = cc;
			if (len >= buf_len) {
				// Buffer is too small
				return ERR_LB_HTTP_READ_LINE_SMALL_BUFFER;
			}

			if ((cc == '\n') || (cc == 0) || (cc == (unsigned char)-1)) {
				// End of line
				break;
			}
		}
	}
	buf_ptr[len] = 0;
	return len;
}

/* --------------------------------------------------------------------------------- */
/*  */
static int http_query(const char* pURL, const char* pHost, uint32_t rsc_size, uint32_t rsc_offset) {
	int ret;
	int len;
	int http_value;
	uint32_t http_content_length;
	char* pc;

	http_build_get_query(httpBuf, sizeof(httpBuf) - 1, pURL, pHost, rsc_offset);

	len = tcpLayer->write(tcpLayer, (unsigned char*)httpBuf, strlen(httpBuf));
	if (len != (int)strlen(httpBuf)) {
		return ERR_LB_HTTP_QUERY_WRITE;
	}

	len = read_line(httpBuf, sizeof(httpBuf));
	if (len <= 0) {
		return ERR_LB_HTTP_READ_LINE;
	}

	/* Parse HTTP response */
	http_value = 0;
	ret = sscanf(httpBuf, "HTTP/%*d.%*d %d %*s", &http_value);
	if (ret != 1) {
		/* Cannot match string, error */
		return ERR_LB_HTTP_QUERY_INCORRECT_ANSWER;
	}

	if ((http_value != 200) && !((http_value == 206) && (rsc_offset > 0))) {
		return ERR_LB_HTTP_QUERY_INCORRECT_CODE;
	}

	http_content_length = 0;
	while (1) {
		ret = read_line(httpBuf, sizeof(httpBuf));
		if (ret < 0) {
			return ERR_LB_HTTP_READ_LINE;
		}

		pc = strstr(httpBuf, ":");
		if (pc != NULL) {
			pc++;
			if (!strncasecmp(httpBuf, HTTP_HD_CONTENT_LENGTH, strlen(HTTP_HD_CONTENT_LENGTH))) {
				ret = sscanf(pc, "%" SCNu32, &http_content_length);
			}
			else if (!strncasecmp(httpBuf, HTTP_HD_CONTENT_RANGE, strlen(HTTP_HD_CONTENT_RANGE))) {
			}
		}
		else {
			/* Body ... */
			break;
		}
	}

	if (http_content_length == 0) {
		return ERR_LB_HTTP_NULL_CONTENT_LENGTH;
	}

	if (http_content_length != (rsc_size - rsc_offset)) {
		return ERR_LB_HTTP_INCORRECT_CONTENT_LENGTH;
	}

	return LB_SUCCESS;
}


/* --------------------------------------------------------------------------------- */
/*  */
void LiveBooster_http_close(void) {
	tcpLayer->stop(tcpLayer);
}


/* --------------------------------------------------------------------------------- */
/*  */
int  LiveBooster_http_start(const char* uri, uint32_t rsc_size, uint32_t rsc_offset) {
	int ret;
	const char* pc = uri;
	const char* ps;

	char host_name[40];
	uint16_t host_port = 80;

	if ((pc == NULL) || (*pc == 0) || (rsc_size == 0) || (rsc_offset >= rsc_size)) {
		return ERR_LB_HTTP_START_NULL;
	}

	if (strncasecmp(pc, "http", 4)) {
		return ERR_LB_HTTP_START_URI_ERROR;
	}
	pc += 4;
	if ((*pc == 's') || (*pc == 'S')) {
		return ERR_LB_HTTP_START_URI_ERROR;
	}
	if (strncmp(pc, "://", 3)) {
		return ERR_LB_HTTP_START_URI_ERROR;
	}
	pc += 3;
	ps = pc;
	while ((*pc != ':') && (*pc != '/') && (*pc != 0))
		pc++;
	memcpy(host_name, ps, pc - ps);
	host_name[pc - ps] = 0;

	if (*pc == ':') { /* get port */
		ps = ++pc;
		while ((*pc != '/') && (*pc != 0))
			pc++;
		if (sscanf(ps, "%" SCNu16, &host_port) != 1) {
			return ERR_LB_HTTP_START_PORT_NOT_FOUND;
		}
	}
	if (*pc != '/') {
		return ERR_LB_HTTP_START_URL_NOT_FOUND;
	}

    ret = tcpLayer->connect(tcpLayer, host_name, HTTP_SERV_PORT, SSL_NOT_ENABLE);

	if (ret <= 0) {
		return ERR_LB_HTTP_START_FAIL_CONNEXION;
	}

	ret = http_query(pc, host_name, rsc_size, rsc_offset);
	if (ret < 0) {
		tcpLayer->stop(tcpLayer);
		return ret;
	}

	return LB_SUCCESS;
}


/* --------------------------------------------------------------------------------- */
/*  */
int  LiveBooster_http_data(char* pData, int len) {
	int ret;

	if (!tcpLayer->connected(tcpLayer)) {
		return ERR_LB_HTTP_DATA_DISCONNECTED;
	}

	ret = tcpLayer->read(tcpLayer, (unsigned char*)pData, len, TIMEOUT_IN_MS);
	if (ret < 0) {
		tcpLayer->stop(tcpLayer);
		return ERR_LB_HTTP_STOPPED;
	}

	if (ret == 0) {
		pData[ret] = 0;
		return LB_SUCCESS;
	}

	pData[ret] = 0;

	return ret;
}

