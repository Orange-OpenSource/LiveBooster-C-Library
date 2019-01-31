/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#ifndef __MqttInterface_h
#define __MqttInterface_h

/**
 * @startuml
 * interface Mqtt {
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
 * Abstract interface for Mqtt
 */
typedef struct _MqttInterface
{


} MqttInterface;

#endif
