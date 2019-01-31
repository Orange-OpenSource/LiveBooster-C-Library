/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#ifndef __LiveBoosterInterface_h
#define __LiveBoosterInterface_h

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "liveBoosterPacket/LiveBooster_msg.h"
#include "liveBoosterPacket/LiveBooster_http.h"

#include "../mqttClient/MqttClient.h"

#include "../serial/SerialInterface.h"
#include "../timer/TimerInterface.h"
#include "../traceDebug/DebugInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================== */
/**
 * * \addtogroup Init  Initialization
 *
 * This section describes functions to create the LiveObjects Client device
 * before connecting this device to the remote LiveObjects platform.
 * @{
 */

/**
 * @brief Initialize the LiveBooster Instance (only one instance on board)
 *        This should always be called first.
 *
 * @param ptrDeviceId pointer on device id with format ""
 * @param apikeyP1    First part (16 MSB) of API Key.
 * @param apikeyP2    Second part (16 LSB) of API Key.
 * @param serial      pointer on Serial interface.
 * @param timer       pointer on Timer interface.
 * @param debug       pointer on Debug interface.
 *
 * @return 0 if successful, otherwise a negative value when error occurs.
 */
int LiveBooster_Init(char* ptrDeviceId,
		             unsigned long long apikeyP1, unsigned long long apikeyP2,
                     SerialInterface* serial,
                     TimerInterface* timer,
                     DebugInterface* debug);

/* @} group end : Init */

/* ================================================================== */
/**
 * \addtogroup  DynamicOpe   Dynamic Operations
 *
 * @{
 */

/**
 * @brief Connect the device to the remote LiveObjects Server.
 *
 * @return 0 if successful, otherwise a negative value when error occurs.
 */
int LiveBooster_Connect(void);

/**
 * @brief Do a LiveObjects MQTT cycle
 * - processing all pending message to be sent
 * - and then calling LiveObjectsClient_Yield
 *
 * @param timeout_ms   Time in milliseconds to wait for message sent/published by LiveObjects platform
 *
 * @return 0 if successful, otherwise a negative value when error occurs.
 */
int LiveBooster_Cycle(int timeout_ms);

/**
 * @brief Disconnect the device to the remote LiveObjects Server.
 *
 */
void LiveBooster_Close(void);

/* @} group end : DynamicOpe */

/* ================================================================== */
/**
 * * \addtogroup Config  IoT Configuration Operations
 *
 * This section describes functions to configure the LiveObjects IoT device.
 *
 * @note Device can be already connected to the LiveObjects platform.
 * @{
 */

/**
 * @brief Define the set of user data as the LiveObjects IoT Collected Data.
 *
 *
 * @param stream_id   Pointer to a c-string specifying the Stream Identifier:
 *                    Identifier of the time series this collected data belongs to.
 * @param model       Optional, pointer to a c-string specifying the Model:
 *                    a string identifying the schema used for the "value" part of the message,
 *                    to avoid conflict at data indexing.
 * @param tags        Optional, pointer to a c-string specifying the tags, coded in JSON format
 *                    (i.e "\"tag1\", \"tag2\"" )
 * @param timestamp   Optional, pointer to a c-string specifying the time of produced data
 * @param gps_ptr     Optional, pointer to structure given the current GPS position.
 * @param data_ptr    Pointer to an array of LiveObjects IoT Data
 * @param data_nb     Number of elements in this array.
 *
 * @return an handle value >= 0  if successful, otherwise a negative value when error occurs.
 */
int LiveBooster_AttachData(const char* stream_id,
		                   const char* model,
						   const char* tags,
						   const char* timestamp,
		                   const LiveBooster_GpsFix_t* gps_ptr,
		                   const LiveBooster_Data_t* data_ptr,
						   int32_t data_nb);

/**
 * @brief Define a set of user parameters as the LiveObjects IoT Configuration parameters.
 *
 * @param ptrParam    Pointer to an array of Configuration Parameters
 * @param nbPparam    Number of elements in this array.
 * @param callback    User callback function, called to check the parameter to be updated.
 *
 * @return always  0  (SUCCESS).
 */
int LiveBooster_AttachCfgParameters  (const LiveBooster_Param_t* ptrParam,
		                              uint32_t  nbPparam,
									  LiveBooster_CallbackParams_t callback);

/**
 * @brief Define the set of user commands.
 *
 * @param ptrCmd      Pointer to an array of LiveObjects IoT Commands
 * @param nbCcmd      Number of elements in this array.
 * @param callback    User callback function, called when a command is received from LiveObjects server.
 *
 * @return always  0  (SUCCESS).
 */
int LiveBooster_AttachCommands (const LiveBooster_Command_t* ptrCmd,
		                        int32_t nbCcmd,
								LiveBooster_CallbackCommand_t callback);

/**
 * @brief Define the set of user resources
 *
 * @param rsc_ptr     Pointer to an array of LiveObjects IoT Resources
 * @param rsc_nb      Number of elements in this array.
 * @param ntfyCB      User callback function, called when download operation is requested or completed by LiveObjects server.
 * @param dataCB      User callback function, called when data is ready to be read.
 *
 * @return always  0  (SUCCESS).
 */
int LiveBooster_AttachResources(const LiveBooster_Resource_t* rsc_ptr,
		                        int32_t rsc_nb,
								LiveBooster_CallbackResourceNotify_t ntfyCB,
		                        LiveBooster_CallbackResourceData_t dataCB);

/* @} group end : Config */

/* ================================================================== */
/**
 * \addtogroup  TriggerOpe Trigger to request an action
 *
 * Note that action will be effectively performed in the next loop of LiveObjects IoT Client thread
 * The order of push actions is not guaranteed.
 *
 * @{
 */

/**
 * @brief Request to publish one set of 'collected data' to LiveObjects server.
 *
 * @param handle      Handle of collected data set
 *
 * @return 0 if successful, otherwise a negative value when error occurs.
 */
int LiveBooster_PushData(int handle);

/* @} group end : TriggerOpe */

/* ================================================================== */
/**
 * \addtogroup  Async Asynchronous Operations
 *
 *
 * @{
 */

/**
 * @brief Read data from the current resource transfer.
 *
 * @param rsc_ptr     Pointer to the user resource item.
 * @param data_ptr    Pointer to the user buffer to receive data
 * @param data_len    Length (in bytes) of this buffer
 *
 * @return The number of read bytes, otherwise a negative value or zero is an error.
 */
int LiveBooster_GetResources(const LiveBooster_Resource_t* rsc_ptr,
		                     char* data_ptr,
					         int data_len);

/* @} group end : Async */
#endif

#ifdef __cplusplus
}


#endif /* __LiveBoosterInterface_h */
