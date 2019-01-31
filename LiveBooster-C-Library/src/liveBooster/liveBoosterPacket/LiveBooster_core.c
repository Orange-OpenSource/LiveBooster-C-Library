/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "../LiveBoosterInterface.h"
#include "LiveBooster_core.h"

#define APIKEY_LENGTH  33

#define LB_SERV_HOST_NAME                   "liveobjects.orange-business.com"
#define LB_SERV_PORT                        8883 //when SSL_ENABLE use port 8883 else use port 1883
#define LB_MQTT_USER_NAME                   "json+device"
#define LB_MQTT_API_KEEPALIVEINTERVAL_SEC   30
#define LB_MQTT_CLIENT_ID                   ""

#define MQTTPacket_willOptions_initializer { {'M', 'Q', 'T', 'W'}, 0, {NULL, {0, NULL}}, {NULL, {0, NULL}}, 0, 0 }

/* Version of MQTT to be used : 4 = 3.1.1  (3 is for  3.1) */
#define MQTTPacket_connectToLo_initializer { \
    {'M', 'Q', 'T', 'C'}, \
    0, 4, {LB_MQTT_CLIENT_ID, {0, NULL}}, \
    LB_MQTT_API_KEEPALIVEINTERVAL_SEC, 1, 0, \
    MQTTPacket_willOptions_initializer, \
    {LB_MQTT_USER_NAME, {0, NULL}}, {NULL, {0, NULL}} \
    }

#define LOC_MQTT_DEF_TOPIC_NAME_SZ 12

typedef struct {
	unsigned int subscribed;
	char topicName[LOC_MQTT_DEF_TOPIC_NAME_SZ];
	messageHandler callback;
} LB_TopicSub_t;


#define TOPIC_CFG_UPD  0
#define TOPIC_COMMAND  1
#define TOPIC_RSC_UPD  2

LB_TopicSub_t LB_TopicSub[3] = {
		{ 0, "dev/cfg/upd", NULL },
		{ 0, "dev/cmd", NULL },
		{ 0, "dev/rsc/upd", NULL }
};
#define SET_TOPIC_NB (sizeof(LB_TopicSub) / sizeof(LB_TopicSub_t))


static LiveBooster_Instance_t liveBooster;
static MQTTClient mqttClient;

/* data use to debug <... */
char traceDebug[500];
DebugInterface *msgDebug;
/* ...> */

static int setStreamId(LiveBooster_SetOfData_t* p_dataSet, const char* stream_id);
static int mqttPublish(enum QoS qos, const char* topic_name, const char* payload_data);
static int processGetRsc(void);
static int processConfig(void);
static void messageHandlerDevCfgUpd (MessageData* msg);
static void messageHandlerDevCmd (MessageData* msg);
static void messageHandlerDevRscUpd (MessageData* msg);


/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_Init(char *deviceId,
		                unsigned long long apiKeyP1, unsigned long long apiKeyP2,
						SerialInterface* serial,
						TimerInterface* timer,
						DebugInterface *debug) {

	liveBooster.deviceId = deviceId;
	liveBooster.apiKeyP1 = apiKeyP1;
	liveBooster.apiKeyP2 = apiKeyP2;
	liveBooster.serial = serial;
	liveBooster.timer = timer;
	liveBooster.debug = debug;

	/* define to debug encoded/decoded msg */
	msgDebug = debug;


	return OK;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_Connect(void) {

	int res;
	unsigned int index;
    const char* pMsg;

	/* 1 - Initializing client */
	msgDebug->print("  ... MQTTClientInit\n");
	MQTTClientInit(&mqttClient, liveBooster.serial, liveBooster.timer, liveBooster.debug);

    /* 2 - Connecting to MQTT server */
    MQTTPacket_connectData connectData = MQTTPacket_connectToLo_initializer;
    connectData.clientID.cstring = liveBooster.deviceId;

    char password[APIKEY_LENGTH];
    snprintf(password, APIKEY_LENGTH, "%08lx%08lx%08lx%08lx",
            (unsigned long)(liveBooster.apiKeyP1>>32), (unsigned long)liveBooster.apiKeyP1,
			(unsigned long)(liveBooster.apiKeyP2>>32), (unsigned long)liveBooster.apiKeyP2);
    connectData.password.cstring = password;

    msgDebug->print("  ... MQTTConnect\n");
    res = MQTTConnect(&mqttClient, &connectData, LB_SERV_HOST_NAME, LB_SERV_PORT, SSL_ENABLE);

    if (!(res == OK)) {
    	return res;
    }
    /* 3 - Subscribe Topic */
    index=0;
    for (index=0;index < SET_TOPIC_NB; index++) {
       if (LB_TopicSub[index].callback != NULL) {
    	    msgDebug->print("  ... MQTTSubscribe\n");
    	   res = MQTTSubscribe(&mqttClient, LB_TopicSub[index].topicName, QOS0, LB_TopicSub[index].callback);
           if (!(res == OK)) {
    	      return res;
           }
       }
    }

	/* 4 - Publish Msg on topic "dev/cfg" and dev/rsc*/
	if (liveBooster.SetParam.param_set.param_ptr != NULL) {
		msgDebug->print("  ... mqttPublish (dev/cfg)\n");
		pMsg = LiveBooster_msg_encode_params_all(&liveBooster.SetParam.param_set, 0);
		res = mqttPublish(QOS0, "dev/cfg", pMsg);
		sprintf(traceDebug,">> Publish on \"dev/cfg\":  %s\n",pMsg); msgDebug->print(traceDebug);
	}

	if (liveBooster.SetRsc.rsc_ptr != NULL) {
		msgDebug->print("  ... mqttPublish (dev/rsc\n");
		pMsg = LiveBooster_msg_encode_resources(&liveBooster.SetRsc);
		res = mqttPublish(QOS0, "dev/rsc", pMsg);
		sprintf(traceDebug,">> Publish on \"dev/rsc\":  %s\n",pMsg); msgDebug->print(traceDebug);
    }

    return res;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_Cycle(int timeout_ms) {
	int ret;

	if (!MQTTIsConnected(&mqttClient)) {
		msgDebug->print("MQTT Is not Connected\n");
		return ERR_LB_CYCLE;
	}

    if (LB_TopicSub[TOPIC_CFG_UPD].callback != NULL) {
	   /* Something to update ?  */
	   /*  -- Config Parameters ? */
	   ret = processConfig();
    }

    ret = processGetRsc();
	if (ret < 0) {
       msgDebug->print("WARNING: Problem on connection HTTP\n");
	}

	/* Get and process some MQTT messages received from the LiveObject Server */
    ret = MQTTYield(&mqttClient, timeout_ms);
	if (ret < 0) {
        return ret;
	}

	return LB_SUCCESS;
}

/* --------------------------------------------------------------------------------- */
/*  */
void LiveBooster_Close(void) {

	int ret;

	ret = MQTTIsConnected(&mqttClient);
	sprintf(traceDebug,"MQTT is connected: %d %s\n",ret, ret==0 ? " =>Non" : " =>Oui"); msgDebug->print(traceDebug);
	if (ret) {
		MQTTDisconnect(&mqttClient);
		liveBooster.debug->print ("Disconnected !\n");
	}
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_AttachData(const char* stream_id,
						   const char* model,
						   const char* tags,
						   const char* timestamp,
		                   const LiveBooster_GpsFix_t* gps_ptr,
						   const LiveBooster_Data_t* data_ptr,
						   int32_t data_nb) {

	int data_hdl;
	if ((stream_id == NULL) || (*stream_id == 0) || (data_ptr == NULL) || (data_nb == 0)) {
		return ERR_LB_ATTACH_DATA;
	}
	for (data_hdl = 0; data_hdl < LB_MAX_OF_DATA_SET; data_hdl++) {
		if (liveBooster.SetData[data_hdl].stream_id[0] == 0) {
			break;
		}
	}

	if (data_hdl < LB_MAX_OF_DATA_SET) {
		size_t len;
		LiveBooster_SetOfData_t* p_dataSet = &liveBooster.SetData[data_hdl];

		int ret = setStreamId(p_dataSet, stream_id);
		if (ret != 0) {
			return ERR_LB_ATTACH_DATA;
		}
		if ((model) && (*model)) {
				len = strlen(model);
				memset(p_dataSet->model, 0, sizeof(p_dataSet->model));
				memcpy(p_dataSet->model, model, len < sizeof(p_dataSet->model) ? len : sizeof(p_dataSet->model));
				p_dataSet->model[sizeof(p_dataSet->model) - 1] = 0;
		}
		else {
			p_dataSet->model[0] = 0;
		}

		if ((tags) &&(*tags)) {
			len = strlen(tags);
			memset(p_dataSet->tags, 0, sizeof(p_dataSet->tags));
			memcpy(p_dataSet->tags, tags, len < sizeof(p_dataSet->tags) ? len : sizeof(p_dataSet->tags));
			p_dataSet->tags[sizeof(p_dataSet->tags) - 1] = 0;
		}
		else {
			p_dataSet->tags[0] = 0;
		}

		if ((timestamp) &&(*timestamp)) {
			len = strlen(timestamp);
			memset(p_dataSet->timestamp, 0, sizeof(p_dataSet->timestamp));
			memcpy(p_dataSet->timestamp, timestamp, len < sizeof(p_dataSet->timestamp) ? len : sizeof(p_dataSet->timestamp));
			p_dataSet->timestamp[sizeof(p_dataSet->timestamp) - 1] = 0;
		}
		else {
			p_dataSet->timestamp[0] = 0;
		}

		p_dataSet->gps_ptr = gps_ptr;

		p_dataSet->data_set.data_ptr = data_ptr;
		p_dataSet->data_set.data_nb = data_nb;

		return data_hdl;
	}
	return ERR_LB_ATTACH_DATA;
}
/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_PushData(int data_hdl) {
	if ((data_hdl >= 0) && (data_hdl < LB_MAX_OF_DATA_SET)
			&& liveBooster.SetData[data_hdl].stream_id[0] && liveBooster.SetData[data_hdl].data_set.data_ptr) {

		const char *pMsg = LiveBooster_msg_encode_data(&liveBooster.SetData[data_hdl]);
		if (pMsg) {
			sprintf(traceDebug,"=> PUBLISH Data %s\n",pMsg); msgDebug->print(traceDebug);
			/* Publish now because it is LiveObjects Client thread */
			return mqttPublish(QOS0, "dev/data", pMsg);
		}
	}
	msgDebug->print("ERROR while publishing data !\n");
	return ERR_LB_PUSH_DATA;
}


/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_AttachCfgParameters  (const LiveBooster_Param_t* ptrParam,
		                              uint32_t  nbParam,
									  LiveBooster_CallbackParams_t callback) {

	liveBooster.SetParam.param_set.param_ptr = ptrParam;
	liveBooster.SetParam.param_set.param_nb = nbParam;
	liveBooster.SetParam.param_callback = callback;

	LB_TopicSub[TOPIC_CFG_UPD].callback = messageHandlerDevCfgUpd;

	memset(&liveBooster.SetUpdatedParam, 0, sizeof(liveBooster.SetUpdatedParam));

	return LB_SUCCESS;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_AttachCommands(const LiveBooster_Command_t* ptrCmd,
		                       int32_t cmd_nb,
		                       LiveBooster_CallbackCommand_t callback) {
	liveBooster.SetCmd.cmd_ptr = ptrCmd;
	liveBooster.SetCmd.cmd_nb = cmd_nb;
	liveBooster.SetCmd.cmd_callback = callback;

	LB_TopicSub[TOPIC_COMMAND].callback = messageHandlerDevCmd;

	return LB_SUCCESS;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_AttachResources(const LiveBooster_Resource_t* rsc_ptr,
		                        int32_t rsc_nb,
		                        LiveBooster_CallbackResourceNotify_t ntfyCB,
								LiveBooster_CallbackResourceData_t dataCB) {

	liveBooster.SetRsc.rsc_ptr = rsc_ptr;
	liveBooster.SetRsc.rsc_nb = rsc_nb;
	liveBooster.SetRsc.rsc_cb_ntfy = ntfyCB;
	liveBooster.SetRsc.rsc_cb_data = dataCB;

	LB_TopicSub[TOPIC_RSC_UPD].callback = messageHandlerDevRscUpd;

	LiveBooster_http_init(liveBooster.serial, liveBooster.timer, liveBooster.debug);

	return LB_SUCCESS;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_GetResources(const LiveBooster_Resource_t* rsc_ptr,
		                     char* data_ptr,
						     int data_len) {

	int ret;
	/* see code in processGetRsc() function */
	if ((liveBooster.SetUpdatedRsc.ursc_cid) && (liveBooster.SetUpdatedRsc.ursc_obj_ptr == rsc_ptr)) {
		ret = LiveBooster_http_data(data_ptr, data_len);
		if (ret > 0) {
			/* Update MD5 algorithm and offset */
			MD5Update(&liveBooster.SetUpdatedRsc.md5_ctx, (const void *)data_ptr, (size_t) ret);
			liveBooster.SetUpdatedRsc.ursc_offset += ret;
		}
		else if (ret == 0) {
			sprintf(traceDebug,
					"No byte while reading %d bytes (offset=%"PRIu32"/%"PRIu32" of  %s)\n",
					data_len, liveBooster.SetUpdatedRsc.ursc_offset, liveBooster.SetUpdatedRsc.ursc_size,
					rsc_ptr->rsc_name); msgDebug->print(traceDebug);
		}
		else {
			sprintf(traceDebug,
					"ERROR(%d) while reading %d bytes (offset=%"PRIu32"/%"PRIu32" of  %s)",
					ret, data_len, liveBooster.SetUpdatedRsc.ursc_offset, liveBooster.SetUpdatedRsc.ursc_size,
					rsc_ptr->rsc_name); msgDebug->print(traceDebug);
		}
	}
	else {
		sprintf(traceDebug,"ERROR - No running resource download !\n"); msgDebug->print(traceDebug);
		LiveBooster_http_close();
		ret = ERR_LB_GET_RESOURCES;
	}
	return ret;
}



/* --------------------------------------------------------------------------------- */
/* PRIVATE fonctions */
/* --------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------- */
/*  */
static int setStreamId(LiveBooster_SetOfData_t* p_dataSet, const char* stream_id) {

	size_t len = strlen(stream_id);
	memset(p_dataSet->stream_id, 0, sizeof(p_dataSet->stream_id));
	memcpy(p_dataSet->stream_id, stream_id,
			len < sizeof(p_dataSet->stream_id) ? len : sizeof(p_dataSet->stream_id));
	p_dataSet->stream_id[sizeof(p_dataSet->stream_id) - 1] = 0;

	return LB_SUCCESS;
}


/* --------------------------------------------------------------------------------- */
/*  */
static int mqttPublish(enum QoS qos, const char* topic_name, const char* payload_data) {
	int res;
	MQTTMessage mqttMsg;

	mqttMsg.qos = qos;
	mqttMsg.retained = 0;
	mqttMsg.dup = 0;
	mqttMsg.id = 0;
	mqttMsg.payload = (void*) payload_data;
	mqttMsg.payloadlen = strlen(payload_data);

	res = MQTTPublish(&mqttClient, topic_name, &mqttMsg);

	return res;
}


/* --------------------------------------------------------------------------------- */
/*  */
static void messageHandlerDevCfgUpd (MessageData* msg) {

	LiveBooster_msg_decode_params_req((const char*) msg->message->payload,
			                           msg->message->payloadlen,
									   &liveBooster.SetParam,
			                           &liveBooster.SetUpdatedParam);
}

/* --------------------------------------------------------------------------------- */
/*  */
static void messageHandlerDevCmd (MessageData* msg)  {
	int ret;
	int32_t cid = 0;

	ret = LiveBooster_msg_decode_cmd_req((const char*) msg->message->payload,
			                             msg->message->payloadlen,
									     &liveBooster.SetCmd,
			                             &cid);
	if (cid) {
		const char* pMsg;
		/* send immediately a command response */
		pMsg = LiveBooster_msg_encode_cmd_result(cid, ret);
		if (pMsg) {

			sprintf(traceDebug,"=> Publish  %s\n",pMsg); msgDebug->print(traceDebug);
            ret = mqttPublish(QOS0, "dev/cmd/res", pMsg);
		}
	}

}

/* --------------------------------------------------------------------------------- */
/*  */
static void messageHandlerDevRscUpd (MessageData* msg)  {

	LiveBooster_ResourceRespCode_t rsc_result;
	const char* pMsg;
	int32_t cid = 0;

	rsc_result = LiveBooster_msg_decode_rsc_req((const char*) msg->message->payload,
			                                     (uint32_t)msg->message->payloadlen,
												 &liveBooster.SetRsc,
			                                     &liveBooster.SetUpdatedRsc,
												 &cid);

	pMsg = LiveBooster_msg_encode_rsc_result(cid, rsc_result);
	sprintf(traceDebug,"=> Publish Resource %s\n",pMsg); msgDebug->print(traceDebug);
	if (pMsg) {
		mqttPublish(QOS0, "dev/rsc/upd/res", pMsg);
	}
}

/* --------------------------------------------------------------------------------- */
/*  */
static int processGetRsc(void) {
	int rc = LB_SUCCESS;
	const char* pMsg;

	if ((liveBooster.SetUpdatedRsc.ursc_cid) && (liveBooster.SetUpdatedRsc.ursc_obj_ptr)) {
		if (liveBooster.SetRsc.rsc_cb_data) {
			if (liveBooster.SetUpdatedRsc.ursc_connected) {
				rc = liveBooster.SetRsc.rsc_cb_data(liveBooster.SetUpdatedRsc.ursc_obj_ptr,
						                            liveBooster.SetUpdatedRsc.ursc_offset);
				if (rc < 0) {
					sprintf(traceDebug,"ERROR returned by User callback function\n"); msgDebug->print(traceDebug);
					rc = ERR_LB_HANDLER_PROCESS_GET_RSC;
				}
				else if (rc == 0) {
					rc = -50;
				}

				if (liveBooster.SetUpdatedRsc.ursc_offset == liveBooster.SetUpdatedRsc.ursc_size) {
					unsigned int i;
					unsigned char computedMd5[16];
					MD5Final(computedMd5, &liveBooster.SetUpdatedRsc.md5_ctx);
					/* Check computed MD5 value with the value given by the LO server */
					for (i = 0; i < sizeof(computedMd5); i++) {
						if (computedMd5[i] != liveBooster.SetUpdatedRsc.ursc_md5[i]) {
							sprintf(traceDebug,
									"Computed MD5 %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
									computedMd5[0], computedMd5[1], computedMd5[2], computedMd5[3], computedMd5[4], computedMd5[5], computedMd5[6],
									computedMd5[7], computedMd5[8], computedMd5[9], computedMd5[10], computedMd5[11], computedMd5[12], computedMd5[13],
									computedMd5[14], computedMd5[15]); msgDebug->print(traceDebug);
							sprintf(traceDebug,
									"LO Server MD5 %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
									liveBooster.SetUpdatedRsc.ursc_md5[0], liveBooster.SetUpdatedRsc.ursc_md5[1],
									liveBooster.SetUpdatedRsc.ursc_md5[2], liveBooster.SetUpdatedRsc.ursc_md5[3],
									liveBooster.SetUpdatedRsc.ursc_md5[4], liveBooster.SetUpdatedRsc.ursc_md5[5],
									liveBooster.SetUpdatedRsc.ursc_md5[6], liveBooster.SetUpdatedRsc.ursc_md5[7],
									liveBooster.SetUpdatedRsc.ursc_md5[8], liveBooster.SetUpdatedRsc.ursc_md5[9],
									liveBooster.SetUpdatedRsc.ursc_md5[10], liveBooster.SetUpdatedRsc.ursc_md5[11],
									liveBooster.SetUpdatedRsc.ursc_md5[12], liveBooster.SetUpdatedRsc.ursc_md5[13],
									liveBooster.SetUpdatedRsc.ursc_md5[14], liveBooster.SetUpdatedRsc.ursc_md5[15]); msgDebug->print(traceDebug);
							sprintf(traceDebug,"MD5 ERROR - [%d] %02x != %02x\n", i, computedMd5[i],
									liveBooster.SetUpdatedRsc.ursc_md5[i]); msgDebug->print(traceDebug);
							break;
						}
					}

					if (liveBooster.SetRsc.rsc_cb_ntfy) {
						LiveBooster_ResourceRespCode_t rsc_ntfy;
						rsc_ntfy = liveBooster.SetRsc.rsc_cb_ntfy((i == sizeof(computedMd5)) ? 1 : 2,
								                        liveBooster.SetUpdatedRsc.ursc_obj_ptr,
														liveBooster.SetUpdatedRsc.ursc_vers_old,
								                        liveBooster.SetUpdatedRsc.ursc_vers_new,
														liveBooster.SetUpdatedRsc.ursc_size);

						if (rsc_ntfy == RSC_RSP_OK) {
						    if (liveBooster.SetRsc.rsc_ptr != NULL) {
						        pMsg = LiveBooster_msg_encode_resources(&liveBooster.SetRsc);
								sprintf(traceDebug,">> Publish on \"dev/rsc\":  %s\n",pMsg); msgDebug->print(traceDebug);
								rc = mqttPublish(QOS0, "dev/rsc", pMsg);
						    }
						}
						else {
							pMsg = LiveBooster_msg_encode_rsc_error("INVALID_RESSOURCE", "md5 error");
							sprintf(traceDebug,"=> Publish Resource %s\n",pMsg); msgDebug->print(traceDebug);
							if (pMsg) {
								rc = mqttPublish(QOS0, "dev/rsc/upd/err", pMsg);
							}
						}
					}
					rc = ERR_LB_HANDLER_PROCESS_GET_RSC;
				}
			}
			else {
				sprintf(traceDebug,
						"PROCESS PENDING RESOURCE %s - cid=%" PRIi32" retry=%d offset=%" PRIu32" => connect to %s ...\n",
						liveBooster.SetUpdatedRsc.ursc_obj_ptr->rsc_name, liveBooster.SetUpdatedRsc.ursc_cid,
						liveBooster.SetUpdatedRsc.ursc_retry, liveBooster.SetUpdatedRsc.ursc_offset,
						liveBooster.SetUpdatedRsc.ursc_uri); msgDebug->print(traceDebug);

				rc = LiveBooster_http_start(liveBooster.SetUpdatedRsc.ursc_uri,
						                    liveBooster.SetUpdatedRsc.ursc_size,
						                    liveBooster.SetUpdatedRsc.ursc_offset);
				if (rc == LB_SUCCESS) {
					sprintf(traceDebug,"PROCESS RESOURCE %s - cid=%" PRIi32" uri='%s'\n",
							liveBooster.SetUpdatedRsc.ursc_obj_ptr->rsc_name,
							liveBooster.SetUpdatedRsc.ursc_cid,
							liveBooster.SetUpdatedRsc.ursc_uri); msgDebug->print(traceDebug);
					liveBooster.SetUpdatedRsc.ursc_connected = 1;
					if (liveBooster.SetUpdatedRsc.ursc_offset == 0) {
					    MD5Init(&liveBooster.SetUpdatedRsc.md5_ctx);
					}
				}
				else {
					pMsg = LiveBooster_msg_encode_rsc_error("ERROR HTTP", "Failure HTTP connection or data not received");
					sprintf(traceDebug,"=> Publish Resource %s\n",pMsg); msgDebug->print(traceDebug);
					if (pMsg) {
						// keep rc value
						mqttPublish(QOS0, "dev/rsc/upd/err", pMsg);
					}
				}
			}
		}
		else {
			sprintf(traceDebug,
					"PROCESS PENDING RESOURCE cid=%" PRIi32" - %s => NO USER Callback => ABORT !\n",
					liveBooster.SetUpdatedRsc.ursc_cid, liveBooster.SetUpdatedRsc.ursc_obj_ptr->rsc_name); msgDebug->print(traceDebug);
		}

		if (rc < LB_SUCCESS) {
			if (liveBooster.SetUpdatedRsc.ursc_connected) {
				LiveBooster_http_close();
				if ((rc == -50) && (liveBooster.SetUpdatedRsc.ursc_offset != liveBooster.SetUpdatedRsc.ursc_size)) {
				    pMsg = LiveBooster_msg_encode_rsc_error("ERROR HTTP", "All data not received");
					sprintf(traceDebug,"=> Publish Resource %s\n",pMsg); msgDebug->print(traceDebug);
				    if (pMsg) {
						rc = mqttPublish(QOS0, "dev/rsc/upd/err", pMsg);
				    }
				}
			}

			liveBooster.SetUpdatedRsc.ursc_cid = 0;
			liveBooster.SetUpdatedRsc.ursc_obj_ptr = NULL;
			liveBooster.SetUpdatedRsc.ursc_connected = 0;
			liveBooster.SetUpdatedRsc.ursc_retry = 0;
			rc = LB_SUCCESS;
		}
	}

	return rc;
}

/* --------------------------------------------------------------------------------- */
/*  */
static int processConfig(void) {
	int rc = 0;

	if (liveBooster.SetParam.param_set.param_ptr != NULL) {
		const char* pMsg;
		if (liveBooster.SetUpdatedParam.cid != 0) {
			if ((liveBooster.SetUpdatedParam.nb_of_params) && (liveBooster.SetUpdatedParam.tab_of_param_ptr[0])) {
				pMsg = LiveBooster_msg_encode_params_update(&liveBooster.SetUpdatedParam);
				if (pMsg) {
					rc = mqttPublish(QOS0, "dev/cfg", pMsg);
					if (rc == 0) {
						liveBooster.SetUpdatedParam.cid = 0;
					}
				}
				else {
					liveBooster.SetUpdatedParam.cid = 0;
				}
			}
			else {
				pMsg = LiveBooster_msg_encode_params_all(&liveBooster.SetParam.param_set, liveBooster.SetUpdatedParam.cid);
				if (pMsg) {
					rc = mqttPublish(QOS0, "dev/cfg", pMsg);
					if (rc == 0) {
						liveBooster.SetUpdatedParam.cid = 0;
					}
				}
				else {
					liveBooster.SetUpdatedParam.cid = 0;
				}
			}
		}
	}
	return rc;
}







