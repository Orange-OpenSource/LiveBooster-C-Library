/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#ifndef __TimerInterface_H_
#define __TimerInterface_H_

/**
 * @startuml
 * interface timer {
 *    +void timerInit ()
 *    +unsigned long millis ()
 *    +void delay (waitTimeInMs)
 * }
 * @enduml
 */

 /**
  * Abstract interface for timer
  */
typedef struct _TimerInterface
{
    /**
     * Start a timer used to read time.
    */
    void (* timerInit) ();

    /**
     * Returns the number of milliseconds since the target/application began running.
     * This number will overflow (go back to zero) as far as possible.
     */
    unsigned long (* millis) ();

    /**
     * Pauses the program for the amount of time waitTimeInMs (in milliseconds).
     */
    void (* delay) (unsigned long waitTimeInMs);

} TimerInterface;

#endif
