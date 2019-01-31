/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#ifndef __TcpClientInterface_h
#define __TcpClientInterface_h

/**
 * @startuml
 * interface TcpClient {
 *    +int connect (host, port)
 *    +void stop ()
 *    +int connected ()
 *    +int available ()
 *    +int read (buffer, maxSize, timeoutInMs)
 *    +int write (data, size, timeoutInMs)
 * }
 * @enduml
 */

/**
 * Abstract interface for TCP client
 */
typedef struct _TcpClientInterface
{
    /**
     * Open TCP connection to server "host:port".
     * Return 1 if operation success, else 0.
     */
    int (* connect) (struct _TcpClientInterface* const obj, const char *host, unsigned short port, unsigned int sslEnabled);

    /**
     * Stop (close) the TCP connection.
     */
    void (* stop) (struct _TcpClientInterface* const obj);

    /**
     * Return 1 is the client is always connected to the server, else 0 (connection closed).
     */
    int ( * connected) (struct _TcpClientInterface* const obj);

    /**
     * Return the number of available bytes received from the server.
     */
    int ( * available) (struct _TcpClientInterface* const obj);

    /**
     * Get into buffer the available bytes received from the server.
     * Return the actual size written in buffer.
     */
    int (* read) (struct _TcpClientInterface* const obj, unsigned char *buffer, int maxSize, int timeoutInMs);

    /**
     * Send data to the server.
     * Return the number of bytes effectively sent, or -1 if an error occurred.
     */
    int (* write) (struct _TcpClientInterface* const obj, const unsigned char *data, int size);

} TcpClientInterface;

#endif
