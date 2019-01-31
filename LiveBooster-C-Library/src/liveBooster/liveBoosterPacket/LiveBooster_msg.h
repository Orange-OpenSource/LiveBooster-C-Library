/*
 * Copyright (C) 2016 Orange
 *
 * This software is distributed under the terms and conditions of the 'BSD-3-Clause'
 * license which can be found in the file 'LICENSE.txt' in this package distribution
 * or at 'https://opensource.org/licenses/BSD-3-Clause'.
 */

/**
 * @file   LiveBooster_msg.h
 * @brief  Interface to encode/decode JSON messages
 *
 */

#ifndef __LiveBooster_msg_H_
#define __LiveBooster_msg_H_

#include "LiveBooster_config.h"
#include "LiveBooster_defs.h"

#include "LiveBooster_md5.h"
#include "../../traceDebug/DebugInterface.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Define an array of simple LiveObjects data elements
 */
typedef struct {
	const LiveBooster_Data_t* data_ptr;   /*!< Address of the first simple LiveObjects data element in array */
	int data_nb;                           /*!< Number of elements in array */
} LiveBooster_ArrayOfData_t;

/**
 * @brief Define an array of LiveObjects configuration parameter elements
 */
typedef struct {
	const LiveBooster_Param_t* param_ptr;  /*!< Address of the first LiveObjects parameter element in array */
	int param_nb;                           /*!< Number of elements in array */
} LiveBooster_ArrayOfParams_t;

/**
 * @brief Define a set of user 'status' to be published to the LiveObjects server
 */
typedef struct {
	LiveBooster_ArrayOfData_t data_set;  /*!< Array of data : 'status' elements */
} LiveBooster_SetOfStatus_t;

/**
 * @brief Define a set of user data to be published to the LiveBooster_ server
 *        in a same stream flow (and also in the same time)
 */
typedef struct {
	LiveBooster_ArrayOfData_t data_set;         /*!< Array of data : 'collected data' elements */
	const LiveBooster_GpsFix_t* gps_ptr;        /*!< Optional, current GPS position */
	char stream_id[LB_SETOFDATA_STREAM_ID_SZ];  /*!< stream-id */
	char model[LB_SETOFDATA_MODEL_SZ];          /*!< model */
	char tags[LB_SETOFDATA_TAGS_SZ];            /*!< tags in JSON format */
	char timestamp[24];                         /*!< Time to ISO 8601 format */
} LiveBooster_SetOfData_t;

/**
 * @brief Define the full set of user configuration parameters to be published to the LiveBooster_ server
 *
 */
typedef struct {
	LiveBooster_ArrayOfParams_t param_set;                 /*!< Array of configuration parameters */
	LiveBooster_CallbackParams_t param_callback; /*!< User callback function, called when parameter is updated */
} LiveBooster_SetOfParams_t;

/**
 * @brief Define the partial set of updated (or not) user configuration parameters.
 *
 */
typedef struct {
	int32_t cid;                      /*!< Correlation Identifier */
	int32_t nb_of_params;             /*!< Number of elements in tab_of_param_ptr */
	const LiveBooster_Param_t* tab_of_param_ptr[LB_MAX_OF_PARSED_PARAMS]; /*!< array of configuration parameters */
} LiveBooster_SetofUpdatedParams_t;

/**
 * @brief Define a set of user commands
 *
 */
typedef struct {
	const LiveBooster_Command_t* cmd_ptr;         /*!< Address of the first LiveObjects command element in array */
	int cmd_nb;                                    /*!< Number of elements in array */
	LiveBooster_CallbackCommand_t cmd_callback;   /*!< User callback function called to process the received command */
} LiveBooster_SetofCommands_t;

/**
 * @brief Define a group of user resources
 *
 */
typedef struct {
	const LiveBooster_Resource_t* rsc_ptr;            /*!< Address of the first LiveObjects resource element in array */
	int rsc_nb;                                        /*!< Number of elements in array */
	LiveBooster_CallbackResourceNotify_t rsc_cb_ntfy; /*!< User callback function called to notify begin/end of transfer */
	LiveBooster_CallbackResourceData_t rsc_cb_data;   /*!< User callback function called to notify that data can be read */
	uint8_t pushtoLBServer;
} LiveBooster_SetOfResources_t;

/**
 * @brief Request to update one user resource
 *        received from the LiveObjects platform
 *
 */
typedef struct {
	int32_t ursc_cid;                    /*!< Correlation Id of the current transfer */
	const LiveBooster_Resource_t* ursc_obj_ptr;       /*!< Resource to update */
	char ursc_vers_old[10];              /*!< Old version sent by the LiveObjects platform */
	char ursc_vers_new[10];              /*!< New version sent by the LiveObjects platform */
	unsigned char ursc_md5[16];          /*!< MD5 given by the LiveObjects platform */
	uint32_t ursc_size;                  /*!< Size of the resource to be transfered in device */
	char ursc_uri[80];                   /*!< URI to get the resource */

	uint8_t ursc_connected;              /*!< Flag indicating if device is always  connected to the HTTP server */
	uint8_t ursc_retry;                  /*!< Count the number to (re)connect to the HTTP server */
	uint32_t ursc_offset;                /*!< Offset in the current transfer of resource */

	md5_context_t md5_ctx;               /*!< Context of MD5 algorithm */

} LiveBooster_SetOfUpdatedResource_t;

const char* LiveBooster_msg_encode_status(const LiveBooster_ArrayOfData_t* p);

const char* LiveBooster_msg_encode_data(const LiveBooster_SetOfData_t* p);

const char* LiveBooster_msg_encode_resources(const LiveBooster_SetOfResources_t* p);

const char* LiveBooster_msg_encode_params_all(const LiveBooster_ArrayOfParams_t* p, int32_t cid);

const char* LiveBooster_msg_encode_cmd_resp(int32_t cid, const LiveBooster_Data_t* data_ptr, int data_nb);

const char* LiveBooster_msg_encode_rsc_result(int32_t cid, LiveBooster_ResourceRespCode_t result);

const char* LiveBooster_msg_encode_rsc_error(char* error, char* errorDetails);

const char* LiveBooster_msg_encode_params_update(const LiveBooster_SetofUpdatedParams_t* p);

const char* LiveBooster_msg_encode_cmd_result(int32_t cid, int result);

LiveBooster_ResourceRespCode_t LiveBooster_msg_decode_rsc_req(const char* payload_data,
		                                                      uint32_t payload_len,
                                                              const LiveBooster_SetOfResources_t* pSetRsc,
															  LiveBooster_SetOfUpdatedResource_t* pRscUpd,
															  int32_t* pCid);

/**
 * @brief Decode a received JSON message to update configuration parameters
 *
 */
int LiveBooster_msg_decode_params_req(const char* payload_data, uint32_t payload_len, const LiveBooster_SetOfParams_t* p,
		LiveBooster_SetofUpdatedParams_t* r);

int LiveBooster_msg_decode_cmd_req(const char* payload_data, uint32_t payload_len, const LiveBooster_SetofCommands_t* p, int32_t* pCid);

extern DebugInterface *msgDebug;
extern char traceDebug[500];

#if defined(__cplusplus)
}
#endif

#endif /* __LiveBooster_msg_H_ */
