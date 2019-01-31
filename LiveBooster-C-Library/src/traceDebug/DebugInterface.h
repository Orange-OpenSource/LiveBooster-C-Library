/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#ifndef __DebugInterface_h
#define __DebugInterface_h

/**
 * @startuml
 * interface Debug {
 *    +void print (log)
 * }
 * @enduml
 */

/**
 * Abstract interface for Debug
 */
typedef struct _DebugInterface
{

	/*
	* Print a log message.
    */
    void (* print) (const char *log);


} DebugInterface;

#endif
