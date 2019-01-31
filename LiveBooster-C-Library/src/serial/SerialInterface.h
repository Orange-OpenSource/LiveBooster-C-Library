/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#ifndef __SerialInterface_h
#define __SerialInterface_h

/**
 * @startuml
 * interface serial {
 *    +void open ()
 *    +int available ()
 *    +char get ()
 *    +void write (buffer, size)
 * }
 * @enduml
 */

/**
 * Abstract interface for serial communication
 */
typedef struct _SerialInterface
{
    /**
     * Open connection to a serial port, with baud-rate 115200, 8 bits data, no parity.
     */
    void (* open) ();

    /**
     * Return 1 if at least one received byte is available in the input buffer; else 0.
     */
    int (* available) ();

    /**
     * Return the first byte of incoming data available (or -1 if no data is available).
     */
    char (* get) ();

    /**
     * Send "size" bytes of buffer "buffer" to the serial link.
     * On method return, all bytes shall have been transmitted (it means that this
     * method shall perform buffer flush if needed).
     * Note: "size" could be null.
     */
    void (* write) (const char *buffer, int size);

} SerialInterface;

#endif
