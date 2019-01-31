/*
 * Copyright (C) 2016 Orange
 *
 * This software is distributed under the terms and conditions of the 'BSD-3-Clause'
 * license which can be found in the file 'LICENSE.txt' in this package distribution
 * or at 'https://opensource.org/licenses/BSD-3-Clause'.
 */
/**
 * @file  LiveBooster_msg_decode.c
 * @brief Decode and process the received JSON messages
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "LiveBooster_msg.h"
#include "LiveBooster_json_api.h"

#include "../../jsonParser/jsmn.h"

#include "LiveBooster_config.h"
#include "LiveBooster_code_b64.h"


/* --------------------------------------------------------------------------------- */
/*  */
static int check_token(const char* payload_json, const jsmntok_t* token, const char* name) {
	if ((payload_json == NULL) || (token == NULL) || (name == NULL) || (*name == 0)) {
		return -1;
	}
	if (token->type == JSMN_STRING) {
		unsigned int tk_len = token->end - token->start;
		const char* tk_ptr = payload_json + token->start;
		if ((tk_len == strlen(name)) && !strncmp(tk_ptr, name, tk_len)) {
			return 1;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
static int isValidTokenPrimitive(const char* payload_json, const jsmntok_t* token) {
	if ((payload_json == NULL) || (token == NULL)) {
		return -1;
	}
	if (token->type != JSMN_PRIMITIVE) {
		return -1;
	}
	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
static int getValueINT32(int32_t* value, const char* payload_json, const jsmntok_t* token) {
	if (isValidTokenPrimitive(payload_json, token))
		return -1;
	if (1 != sscanf(payload_json + token->start, "%"SCNi32, value))
		return -1;
	return 0;
}

static int getValueUINT32(uint32_t* value, const char* payload_json, const jsmntok_t* token) {
	if (isValidTokenPrimitive(payload_json, token))
		return -1;
	if (1 != sscanf(payload_json + token->start, "%" SCNu32, value))
		return -1;
	return 0;
}

static int getValueFLOAT(float* value, const char* payload_json, const jsmntok_t* token) {
	if (isValidTokenPrimitive(payload_json, token)) {
		return -1;
	}
#ifdef ARDUINO
	double df;
	df = atof(payload_json + token->start);
	*value = df;
#else
	if (1 != sscanf(payload_json + token->start, "%f", value))
		return -1;
#endif
	return 0;
}


/* --------------------------------------------------------------------------------- */
/*  */
static int get_CorrelationId(int32_t* pCid, const char* payload_data, const jsmntok_t* tokens, int32_t token_cnt) {
	int idx;
	if ((payload_data == NULL) || (tokens == NULL) || (pCid == NULL)) {
		return -1;
	}

	token_cnt--;
	for (idx = 0; idx < token_cnt; idx++) {
		if ((tokens[idx].type == JSMN_STRING) && (tokens[idx].size == 1) && (3 == (tokens[idx].end - tokens[idx].start))
				&& (!strncmp("cid", payload_data + tokens[idx].start, 3))) {

			// Get next. Must be a primitive value (implicit, it is an integer)
			idx++;
			if ((tokens[idx].type == JSMN_PRIMITIVE) && (tokens[idx].size == 0)) {
				int ret = getValueINT32(pCid, payload_data, &tokens[idx]);
				return ret;
			}
			return -1;
		}
	}
	return -1;
}

/* --------------------------------------------------------------------------------- */
/*  */
static int updateCnfParam(const char* payload_json, const jsmntok_t* token, const LiveBooster_Param_t* param_ptr,
		LiveBooster_CallbackParams_t cfgCB) {
	int ret;
	if ((payload_json == NULL) || (token == NULL) || (param_ptr == NULL)) {
		return -1;
	}

	if (param_ptr->parm_data.data_type == LB_TYPE_STRING_C) {
		if (token->type != JSMN_STRING) {
			return -1;
		}
		*((char *)(payload_json + token->end)) = '\0';
		ret = cfgCB(param_ptr, (const void*) (payload_json + token->start), token->end - token->start);
	}
	else if (param_ptr->parm_data.data_type == LB_TYPE_BIN) {
		if (token->type != JSMN_STRING) {
			return -1;
		}
		char bin64[LB_BIN64_BUF_SZ];
		char value[LB_BIN64_BUF_SZ];
		memcpy(bin64,(const char *)(payload_json + token->start),token->end - token->start);
		bin64[(token->end - token->start)]= '\0';
		ret = b64_decode(bin64, value, token->end - token->start);
		ret = cfgCB(param_ptr, (const void*) value, token->end - token->start);
	}
	else {
		if (token->type != JSMN_PRIMITIVE) {
			return -1;
		}

		if (param_ptr->parm_data.data_type == LB_TYPE_UINT32) {
			uint32_t value;
			ret = getValueUINT32(&value, payload_json, token);
			if ((ret == 0) && (param_ptr->parm_data.data_value)) {
				ret = cfgCB(param_ptr, (const void*) &value, sizeof(uint32_t));
				if (ret == 0)
					*((uint32_t*) param_ptr->parm_data.data_value) = value;
			}
		}
		else if (param_ptr->parm_data.data_type == LB_TYPE_INT32) {
			int32_t value;
			ret = getValueINT32(&value, payload_json, token);
			if ((ret == 0) && (param_ptr->parm_data.data_value)) {
				ret = cfgCB(param_ptr, (const void*) &value, sizeof(int32_t));
				if (ret == 0)
					*((int32_t*) param_ptr->parm_data.data_value) = value;
			}
		}
		else if (param_ptr->parm_data.data_type == LB_TYPE_FLOAT) {
			float value;
			ret = getValueFLOAT(&value, payload_json, token);
			if ((ret == 0) && (param_ptr->parm_data.data_value)) {
				ret = cfgCB(param_ptr, (const void*) &value, sizeof(float));
				if (ret == 0)
					*((float*) param_ptr->parm_data.data_value) = value;
			}
		}
		else {
		}
	}
	return ret;
}


/* --------------------------------------------------------------------------------- */
/*  */
static int get_md5FromString(const unsigned char *s, unsigned char *buf_ptr, uint32_t buf_len) {
	uint32_t i, j, k;
	if ((s == NULL) || (buf_ptr == NULL) || (buf_len == 0)) {
		return -1;
	}
	memset(buf_ptr, 0, buf_len);
	for (i = 0; i < buf_len * 2; i++, s++) {
		if (*s >= '0' && *s <= '9') {
			j = *s - '0';
		}
		else if (*s >= 'A' && *s <= 'F') {
			j = *s - '7';
		}
		else if (*s >= 'a' && *s <= 'f') {
			j = *s - 'W';
		}
		else {
			/* 'a' (97) - 10 = */
			return -1;
		}

		k = ((i & 1) != 0) ? j : j << 4;
		buf_ptr[i >> 1] = (unsigned char) (buf_ptr[i >> 1] | k);
	}
	return (0);
}

/* --------------------------------------------------------------------------------- */
/* Decode a received JSON message to download resource
 */
LiveBooster_ResourceRespCode_t LiveBooster_msg_decode_rsc_req(const char* payload_data,
		                                                      uint32_t payload_len,
                                                              const LiveBooster_SetOfResources_t* pSetRsc,
															  LiveBooster_SetOfUpdatedResource_t* pRscUpd,
															  int32_t* pCid) {

	int ret;
	int token_cnt;
	jsmn_parser parser;
	jsmntok_t tokens[20];
	int idx;
	int len;
	int size;

	if ((pSetRsc == NULL) || (payload_data == NULL) || (payload_len == 0) || (pRscUpd == NULL) || (pCid == NULL)) {
		return RSC_RSP_ERR_INTERNAL_ERROR;
	}

	*pCid = 0;

	sprintf(traceDebug,"\nLiveBooster_msg_decode_rsc_req: %s\n",payload_data); msgDebug->print(traceDebug);

	memset(&tokens, 0, sizeof(tokens));
	jsmn_init(&parser);
	token_cnt = jsmn_parse(&parser, payload_data, payload_len, tokens, 20);
	if (token_cnt < 0) {
		return RSC_RSP_ERR_INTERNAL_ERROR;
	}
	if (token_cnt == 0) {
		return RSC_RSP_OK;
	}

	if (token_cnt < 1) {
		return RSC_RSP_ERR_INTERNAL_ERROR;
	}

	if ((tokens[0].type != JSMN_OBJECT) || (tokens[0].size <= 0)) {
		return RSC_RSP_ERR_INTERNAL_ERROR;
	}

	// Get the Correlation Id
	ret = get_CorrelationId(pCid, payload_data, &tokens[1], token_cnt);
	if (ret) {
		return RSC_RSP_ERR_INTERNAL_ERROR;
	}

	if (pRscUpd->ursc_cid) {
		return RSC_RSP_ERR_NOT_AUTHORIZED; // RSC_RSP_ERR_BUSY
	}

	memset(pRscUpd, 0, sizeof(LiveBooster_SetOfUpdatedResource_t));

	pRscUpd->ursc_cid = *pCid;

	// get resource id
	for (idx = 0; idx < (token_cnt - 1); idx++) {
		if ((tokens[idx].type == JSMN_STRING) && (tokens[idx].size == 1) && (2 == (tokens[idx].end - tokens[idx].start))
				&& (!strncmp("id", payload_data + tokens[idx].start, 2))) {
			int jw;
			const LiveBooster_Resource_t* rsc_ptr;

			// Get next. Must be a string value
			idx++;
			if ((tokens[idx].type != JSMN_STRING) || (tokens[idx].size != 0)) {
				return RSC_RSP_ERR_INTERNAL_ERROR;
			}
			// found
			rsc_ptr = pSetRsc->rsc_ptr;
			len = tokens[idx].end - tokens[idx].start;
			for (jw = 0; jw < pSetRsc->rsc_nb; jw++, rsc_ptr++) {
				if ((len == (int) strlen(rsc_ptr->rsc_name))
						&& (!strncmp(rsc_ptr->rsc_name, payload_data + tokens[idx].start, len))) {
					pRscUpd->ursc_obj_ptr = rsc_ptr;
					break;
				}
			}
			if (pRscUpd->ursc_obj_ptr == NULL)
			break;
		}
	}
	if (pRscUpd->ursc_obj_ptr == NULL) {
		if (idx == token_cnt)
		return RSC_RSP_ERR_INVALID_RESOURCE;
	}

	// Now, get each  parameter
	size = tokens[0].size;
	idx = 1;

	while ((size > 0) && (token_cnt > 0)) {
		int8_t val_type;
		char* val_ptr;
		int32_t val_len;
		int len = tokens[idx].end - tokens[idx].start;
		if ((tokens[idx].type != JSMN_STRING) || (tokens[idx].size != 1)) {
			// error;
			return RSC_RSP_ERR_INTERNAL_ERROR;
		}
		size--;

		if ((len == 1) && !strncmp("m", payload_data + tokens[idx].start, len)) {
			int ms;
			if ((tokens[idx + 1].type != JSMN_OBJECT) || (tokens[idx + 1].size < 3)) {
				return RSC_RSP_ERR_INTERNAL_ERROR;
			}
			ms = tokens[idx + 1].size;

			idx += 2;
			token_cnt -= 2;
			while (ms > 0) {
				if ((tokens[idx].type != JSMN_STRING) || (tokens[idx].size != 1)
						|| (tokens[idx + 1].type != JSMN_STRING) || (tokens[idx + 1].size != 0)) {
					return RSC_RSP_ERR_INTERNAL_ERROR;
				}
				len = tokens[idx].end - tokens[idx].start;
				ms--;
				val_type = 0;
				if ((len == 4) && !strncmp("size", payload_data + tokens[idx].start, len)) {
					val_type = 3;
					val_ptr = (char*) &pRscUpd->ursc_size;
					val_len = 0;
				}
				else if ((len == 3) && !strncmp("uri", payload_data + tokens[idx].start, len)) {
					val_type = 2;
					val_ptr = pRscUpd->ursc_uri;
					val_len = sizeof(pRscUpd->ursc_uri) - 1;
				}
				else if ((len == 3) && !strncmp("md5", payload_data + tokens[idx].start, len)) {
					val_type = 4;
					val_ptr = (char*)pRscUpd->ursc_md5;
					val_len = sizeof(pRscUpd->ursc_md5);
				}

				if (val_type) {
					if (val_ptr) {
						if (val_type == 2) {
							len = tokens[idx + 1].end - tokens[idx + 1].start;
							if (len > val_len) {
								len = val_len;
							}
							strncpy(val_ptr, payload_data + tokens[idx + 1].start, len);
							val_ptr[len] = 0;
						}
						else if (val_type == 3) {
							if (1 != sscanf(payload_data + tokens[idx + 1].start, "%" SCNu32, (uint32_t*) val_ptr)) {
								return RSC_RSP_ERR_INTERNAL_ERROR;
							}
						}
						else if (val_type == 4) {
							if ((tokens[idx + 1].end - tokens[idx + 1].start) == (val_len * 2)) {
								if (get_md5FromString((const unsigned char*) (payload_data + tokens[idx + 1].start),
										(unsigned char*) val_ptr, val_len)) {
									//return 2;
								}
							}
							else {
								//return 2;
							}
						}
						else {
							return RSC_RSP_ERR_INTERNAL_ERROR;
						}
					}
				}
				idx += 2;
				token_cnt -= 2;
			}
		}
		else {
			if ((tokens[idx].type != JSMN_STRING) || (tokens[idx].size != 1) || (tokens[idx + 1].size != 0)
					|| ((tokens[idx + 1].type != JSMN_STRING) && (tokens[idx + 1].type != JSMN_PRIMITIVE))) {
				return RSC_RSP_ERR_INTERNAL_ERROR;
			}

			val_type = 0;
			if ((len == 2) && !strncmp("id", payload_data + tokens[idx].start, len)) {
				// skip, already processed
				val_type = 1;
			}
			else if ((len == 3) && !strncmp("old", payload_data + tokens[idx].start, len)) {
				val_type = 2;
				val_ptr = pRscUpd->ursc_vers_old;
				val_len = sizeof(pRscUpd->ursc_vers_old) - 1;
			}
			else if ((len == 3) && !strncmp("new", payload_data + tokens[idx].start, len)) {
				val_type = 2;
				val_ptr = pRscUpd->ursc_vers_new;
				val_len = sizeof(pRscUpd->ursc_vers_new) - 1;
			}
			else if ((len == 3) && !strncmp("cid", payload_data + tokens[idx].start, len)) {
				// skip, already processed
				val_type = 1;
			}
			if (val_type) {
				if ((val_type == 2) && (val_ptr)) {
					int len = tokens[idx + 1].end - tokens[idx + 1].start;
					if (len > val_len) {
						len = val_len;
					}
					strncpy(val_ptr, payload_data + tokens[idx + 1].start, len);
					val_ptr[len] = 0;
				}
			}
			idx += 2;
			token_cnt -= 2;
		}
	}

	if (pSetRsc->rsc_cb_ntfy) { // User callback function
		LiveBooster_ResourceRespCode_t rsc_resp_code;
		rsc_resp_code = pSetRsc->rsc_cb_ntfy(0, pRscUpd->ursc_obj_ptr, pRscUpd->ursc_vers_old, pRscUpd->ursc_vers_new,
				pRscUpd->ursc_size);
		if (rsc_resp_code) { // Refused by user
			pRscUpd->ursc_cid = 0;
			pRscUpd->ursc_obj_ptr = NULL;
			return rsc_resp_code;
		}
	}

	sprintf(traceDebug,"md5= %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			pRscUpd->ursc_md5[0], pRscUpd->ursc_md5[1], pRscUpd->ursc_md5[2], pRscUpd->ursc_md5[3],
			pRscUpd->ursc_md5[4], pRscUpd->ursc_md5[5], pRscUpd->ursc_md5[6], pRscUpd->ursc_md5[7],
			pRscUpd->ursc_md5[8], pRscUpd->ursc_md5[9], pRscUpd->ursc_md5[10], pRscUpd->ursc_md5[11],
			pRscUpd->ursc_md5[12], pRscUpd->ursc_md5[13], pRscUpd->ursc_md5[14], pRscUpd->ursc_md5[15]); msgDebug->print(traceDebug);

	pRscUpd->ursc_connected = 0;
	pRscUpd->ursc_offset = 0;

	return RSC_RSP_OK;
}

/* --------------------------------------------------------------------------------- */
/* Decode a received JSON message to update configuration parameters
 */
int  LiveBooster_msg_decode_params_req(const char* payload_data, uint32_t payload_len, const LiveBooster_SetOfParams_t* pSetCfg,
		LiveBooster_SetofUpdatedParams_t* pSetCfgUpdate) {

#define NB_TK_FOR_PARMS (5+6*LB_MAX_OF_PARSED_PARAMS+1)  //  6 tokens by parameter
	int ret;
	int token_cnt;
	jsmn_parser parser;
	jsmntok_t tokens[NB_TK_FOR_PARMS];
	int idx;
	int size;
	const char* pc;
	int len;

	if ((pSetCfg == NULL) || (payload_data == NULL) || (payload_len == 0) || (pSetCfgUpdate == NULL)) {
		return -1;
	}

	pSetCfgUpdate->cid = 0;
	pSetCfgUpdate->nb_of_params = 0;

	memset(&tokens, 0, sizeof(tokens));
	jsmn_init(&parser);
	token_cnt = jsmn_parse(&parser, payload_data, payload_len, tokens, NB_TK_FOR_PARMS);
	if (token_cnt < 0) {
		return -1;
	}
#undef NB_TK_FOR_PARMS
	if (token_cnt == 0) {
		return 0;
	}

	if (token_cnt < 2) {
		return -1;
	}

	if ((tokens[0].type != JSMN_OBJECT) || (tokens[0].size <= 0)) {
		return -1;
	}

	// Get the Correlation Id
	ret = get_CorrelationId(&pSetCfgUpdate->cid, payload_data, &tokens[1], token_cnt);
	if (ret) {
		return -1;
	}

	if ((tokens[1].type != JSMN_STRING) || (tokens[1].size != 1) // should be 'cfg'
			|| (tokens[2].type != JSMN_OBJECT) || (tokens[2].size < 0))  {
		return -1;
	}

	ret = check_token(payload_data, &tokens[1], "cfg");
	if (ret <= 0) {
		return -1;
	}
	// skip the first tokens '{ "cfg" : {'
	idx = 3;
	token_cnt -= 3;

	// Get the number of parameters
	size = tokens[2].size;
	// Now, get each configuration parameter
	while (size > 0) {
		if ((token_cnt < 6) || (tokens[idx].type != JSMN_STRING) // 0: should be the Parameter Name
				|| (tokens[idx + 1].type != JSMN_OBJECT) // 1: should be {
				|| (tokens[idx + 2].type != JSMN_STRING) // 2: Should be equal to "t"
				|| (tokens[idx + 3].type != JSMN_STRING) // 3: Should be the parameter type : "u32" , "u16", ...
				|| (tokens[idx + 4].type != JSMN_STRING) // 4: Should be equal to "v"
														 // 5: Value : either JSMN_PRIMITIVE or JSMN_STRING
				|| !((tokens[idx + 5].type == JSMN_PRIMITIVE) || (tokens[idx + 5].type == JSMN_STRING))) {
			return -2;
		}

		ret = check_token(payload_data, &tokens[idx + 2], "t");
		if (ret <= 0) {
			return -2;
		}

		ret = check_token(payload_data, &tokens[idx + 4], "v");
		if (ret <= 0) {
			return -2;
		}

		// Parameter Name
		len = tokens[idx].end - tokens[idx].start;
		pc = payload_data + tokens[idx].start;

		if (pSetCfg->param_set.param_nb) {
			int i;
			const LiveBooster_Param_t* param_ptr = pSetCfg->param_set.param_ptr;
			for (i = 0; i < pSetCfg->param_set.param_nb; i++, param_ptr++) {
				int param_name_len = strlen(param_ptr->parm_data.data_name);
				if ((len == param_name_len) && (!strncmp(pc, param_ptr->parm_data.data_name, param_name_len))) {
					// Config Parameter Name is found in the user list
					// Get the type of this config parameter
					LiveBooster_Type_t type = LB_getDataTypeFromStrL(payload_data + tokens[idx + 3].start,
							tokens[idx + 3].end - tokens[idx + 3].start);
					if (type == LB_TYPE_UNKNOWN) {
						sprintf(traceDebug," Error type inconnu %d \n  ",type); msgDebug->print(traceDebug);
					}
					else if (type != param_ptr->parm_data.data_type) {
						sprintf(traceDebug," type incoherant %d    %d \n  ", type, param_ptr->parm_data.data_type); msgDebug->print(traceDebug);
					}
					else if ((type == LB_TYPE_STRING_C) && (tokens[idx + 5].type != JSMN_STRING)) {
						sprintf(traceDebug," type string incoherant %d    %d \n  ", type, tokens[idx + 5].type); msgDebug->print(traceDebug);
					}
					else {
						ret = updateCnfParam(payload_data, &tokens[idx + 5], param_ptr, pSetCfg->param_callback);
						if (pSetCfgUpdate->nb_of_params < LB_MAX_OF_PARSED_PARAMS) {
							pSetCfgUpdate->tab_of_param_ptr[pSetCfgUpdate->nb_of_params++] = param_ptr;
						}
					}
					break;
				}
			}
		}
		size--;
		token_cnt -= 6;
		idx += 6;
	}

	return 0;
}


/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_msg_decode_cmd_req(const char* payload_data,
		                           uint32_t payload_len,
								   const LiveBooster_SetofCommands_t* pSetCmd,
		                           int32_t* pCid) {
	int ret;
	int token_cnt;
	jsmn_parser parser;
	jsmntok_t tokens[20];
	int idx;
	int size;
	const LiveBooster_Command_t* cmd_ptr;

	if ((pSetCmd == NULL) || (payload_data == NULL) || (pCid == NULL)) {
		return -1;
	}

	*pCid = 0;
	sprintf(traceDebug,"===> LiveBooster_msg_decode_cmd_req  %s\n",payload_data); msgDebug->print(traceDebug);

	memset(&tokens, 0, sizeof(tokens));
	jsmn_init(&parser);
	token_cnt = jsmn_parse(&parser, payload_data, payload_len, tokens, 20);
	if (token_cnt < 0) {
		return -1;
	}
	if (token_cnt == 0) {
		return 0;
	}

	if (token_cnt < 5) {
		return -1;
	}

	if ((tokens[0].type != JSMN_OBJECT) || (tokens[0].size <= 0)) {
		return -1;
	}

	// Get the Correlation Id.
	ret = get_CorrelationId(pCid, payload_data, &tokens[1], token_cnt - 1);
	if (ret) {
		return -1;
	}

	// Now, Check the first tokens
	// Note: suppose that token[1]= "req" and token[3]= "arg", and so cid at the end !
	if ((tokens[0].type != JSMN_OBJECT) || (tokens[0].size < 3) // at least 3 items : "req", "arg" and "cid"
			|| (tokens[1].type != JSMN_STRING) || (tokens[1].size != 1) // expected : "req"
			|| (tokens[2].type != JSMN_STRING) || (tokens[2].size != 0) // expected : name of request
			) {
		return -2;
	}

	ret = check_token(payload_data, &tokens[1], "req");
	if (ret <= 0) {
		return -2;
	}

	// Is it registered by user ?
	cmd_ptr = NULL;
	size = tokens[2].end - tokens[2].start;
	for (idx = 0; idx < pSetCmd->cmd_nb; idx++) {
		if ((size == (int) strlen(pSetCmd->cmd_ptr[idx].cmd_name))
				&& !strncmp(pSetCmd->cmd_ptr[idx].cmd_name, payload_data + tokens[2].start, size)) {
			cmd_ptr = &pSetCmd->cmd_ptr[idx];
			break;
		}
	}
	if (cmd_ptr == NULL) { // not found in the set of commands
		return -3;
	}
	if (pSetCmd->cmd_callback == NULL) { // No callback function !
		return -4;
	}
	// Check the second token :  "arg" : { ... }
	// TODO: position of 'arg' should be anywhere. (after or before 'cid')
	if ((tokens[3].type != JSMN_STRING) || (tokens[3].size != 1) // expected : "arg"
			|| (tokens[4].type != JSMN_OBJECT)  // expected :  ': {'
			) {
		return -2;
	}
	ret = check_token(payload_data, &tokens[3], "arg");
	if (ret <= 0) {
		return -2;
	}

	// Get the number of arguments
	size = tokens[4].size;

	// Now, get each argument. Support only simple type - "name" : string or primitive value
	idx = 5;
	token_cnt -= 5;
	if (size > 0) {
		char* pm;
		LiveBooster_CommandRequestBlock_t* pReqBlkWithArgs;
		LiveBooster_CommandArg_t* pArgs;
		char* pLine;

		int len = sizeof(LiveBooster_CommandRequestBlock_t) + (size - 1) * sizeof(LiveBooster_CommandArg_t)
				+ tokens[idx + (size * 2) + 1].end - tokens[idx].start + 1;
		pm = (char*) malloc(len);
		if (pm == NULL) {
			return -6;
		}

		pReqBlkWithArgs = (LiveBooster_CommandRequestBlock_t*) pm;
		pArgs = (LiveBooster_CommandArg_t*) pReqBlkWithArgs->args_array;
		pLine = (char*) (pm + sizeof(LiveBooster_CommandRequestBlock_t)
				+ (size - 1) * sizeof(LiveBooster_CommandArg_t));

		pReqBlkWithArgs->hd.cmd_blk_len = len;
		pReqBlkWithArgs->hd.cmd_ptr = cmd_ptr;
		pReqBlkWithArgs->hd.cmd_cid = *pCid;
		pReqBlkWithArgs->hd.cmd_args_nb = 0;

		while ((token_cnt >= 2) && (size > 0)) {
			if ((tokens[idx].type != JSMN_STRING) || (tokens[idx].size != 1) || (tokens[idx + 1].size != 0)
					|| ((tokens[idx + 1].type != JSMN_STRING) && (tokens[idx + 1].type != JSMN_PRIMITIVE))) {
				return -2;
			}


			pArgs->arg_name = pLine;
			memcpy(pLine, payload_data + tokens[idx].start, tokens[idx].end - tokens[idx].start);
			pLine += tokens[idx].end - tokens[idx].start;
			*pLine++ = 0;

			pArgs->arg_value = pLine;
			memcpy(pLine, payload_data + tokens[idx + 1].start, tokens[idx + 1].end - tokens[idx + 1].start);
			pLine += tokens[idx + 1].end - tokens[idx + 1].start;
			*pLine++ = 0;

			pArgs->arg_type = (tokens[idx + 1].type == JSMN_STRING) ? 1 : 0;

			pArgs++;
			pReqBlkWithArgs->hd.cmd_args_nb++;

			size--;
			idx += 2;
			token_cnt -= 2;
		}

		if (size) {
			return -2;
		}

		ret = pSetCmd->cmd_callback(pReqBlkWithArgs);

		free(pReqBlkWithArgs);
	}
	else {
		LiveBooster_CommandRequestHeader_t* pReqWithoutArg = (LiveBooster_CommandRequestHeader_t*) malloc(
				sizeof(LiveBooster_CommandRequestHeader_t));
		if (pReqWithoutArg == NULL) {
			return -6;
		}

		pReqWithoutArg->cmd_blk_len = sizeof(LiveBooster_CommandRequestHeader_t);
		pReqWithoutArg->cmd_ptr = cmd_ptr;
		pReqWithoutArg->cmd_cid = *pCid;
		pReqWithoutArg->cmd_args_nb = 0;

		ret = pSetCmd->cmd_callback(pReqWithoutArg);

		free(pReqWithoutArg);
	}
	return ret;
}
