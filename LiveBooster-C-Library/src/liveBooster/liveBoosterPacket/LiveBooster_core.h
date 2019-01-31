/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#ifndef __LiveBooster_core_h
#define __LiveBooster_core_h

#include "LiveBooster_msg.h"

#include "../../serial/SerialInterface.h"
#include "../../timer/TimerInterface.h"
#include "../../traceDebug/DebugInterface.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Define the full set of parameters to LiveBooster instance
 *
 */
typedef struct {

	char *deviceId;
	unsigned long long apiKeyP1;
	unsigned long long apiKeyP2;

	SerialInterface *serial;
    TimerInterface *timer;
    DebugInterface *debug;

    LiveBooster_SetOfParams_t SetParam;
    LiveBooster_SetofUpdatedParams_t SetUpdatedParam;
    LiveBooster_SetofCommands_t SetCmd;
    LiveBooster_SetOfData_t SetData[LB_MAX_OF_DATA_SET];
    LiveBooster_SetOfResources_t SetRsc;
    LiveBooster_SetOfUpdatedResource_t  SetUpdatedRsc;

} LiveBooster_Instance_t;


#ifdef __cplusplus
}
#endif

#endif /* __LiveBooster_core_h */
