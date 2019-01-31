/*
 * Copyright (C) 2016 Orange
 *
 * This software is distributed under the terms and conditions of the 'BSD-3-Clause'
 * license which can be found in the file 'LICENSE.txt' in this package distribution
 * or at 'https://opensource.org/licenses/BSD-3-Clause'.
 */

/**
 * @file  LiveBooster_msg_encode.c
 * @brief Encode JSON messages to be published
 */

#include <stdio.h>
#include <string.h>

#include "LiveBooster_config.h"
#include "LiveBooster_msg.h"
#include "LiveBooster_json_api.h"

/* --------------------------------------------------------------------------------- */
/*  */
 const char* LiveBooster_msg_encode_status_buf(char* buf_ptr, uint32_t buf_len, const LiveBooster_ArrayOfData_t* pObjSet) {
	int ret, i;
	const LiveBooster_Data_t* data_ptr;
	ret = LiveBooster_json_begin_section(buf_ptr, buf_len, "info");
	if (ret) {
		return NULL;
	}
	data_ptr = pObjSet->data_ptr;
	for (i = 0; i < pObjSet->data_nb; i++) {
		ret = LiveBooster_json_add_item(data_ptr, buf_ptr, buf_len);
		if (ret) {
			return NULL;
		}
		data_ptr++;
	}
	ret = LiveBooster_json_end_section(buf_ptr, buf_len);
	if (ret) {
		return NULL;
	}
	return buf_ptr;
}

/* --------------------------------------------------------------------------------- */
/*  */
static const char* LiveBooster_msg_encode_data_buf(char* buf_ptr, uint32_t buf_len, const LiveBooster_SetOfData_t* pSetData) {
	int ret;

	ret = LiveBooster_json_begin(buf_ptr, buf_len);

	if (ret == 0) {
		// stream id
		ret = LiveBooster_json_add_name_str("s", pSetData->stream_id, buf_ptr, buf_len);
	}

	// timestamp
	if ((ret == 0) && (pSetData->timestamp[0]))
		ret = LiveBooster_json_add_name_str("ts", pSetData->timestamp, buf_ptr, buf_len);

	// model
	if ((ret == 0) && (pSetData->model[0]))
		ret = LiveBooster_json_add_name_str("m", pSetData->model, buf_ptr, buf_len);

	// Add GPS localization
	if ((ret == 0) && (pSetData->gps_ptr) && (pSetData->gps_ptr->gps_valid)) {
		char msg[80];
		snprintf(msg, sizeof(msg) - 1, "%3.6f,%3.6f", pSetData->gps_ptr->gps_lat, pSetData->gps_ptr->gps_long);
		ret = LiveBooster_json_add_name_array("loc", msg, buf_ptr, buf_len);
	}

	if (ret == 0)
		ret = LiveBooster_json_add_section_start("v", buf_ptr, buf_len);

	if (ret == 0) {
		int i;
		const LiveBooster_Data_t* data_ptr = pSetData->data_set.data_ptr;
		for (i = 0; i < pSetData->data_set.data_nb; i++) {
			ret = LiveBooster_json_add_item(data_ptr, buf_ptr, buf_len);
			if (ret) {
				break;
			}
			data_ptr++;
		}
	}

	if (ret == 0) {
		ret = LiveBooster_json_add_section_end(buf_ptr, buf_len);
	}

	if ((ret == 0) && (pSetData->tags[0]))
		ret = LiveBooster_json_add_name_array("t", pSetData->tags, buf_ptr, buf_len);

	if (ret == 0)
		ret = LiveBooster_json_end(buf_ptr, buf_len);

	return (ret == 0) ? buf_ptr : NULL;
}


/* --------------------------------------------------------------------------------- */
/*  */
static const char* LiveBooster_msg_encode_resources_buf(char* buf_ptr, uint32_t buf_len,
		const LiveBooster_SetOfResources_t* pSetResources) {
	int ret, i;
	const LiveBooster_Resource_t* rsc_ptr;

	ret = LiveBooster_json_begin_section(buf_ptr, buf_len, "rsc");
	if (ret) {
		return NULL;
	}
	rsc_ptr = pSetResources->rsc_ptr;
	for (i = 0; i < pSetResources->rsc_nb; i++) {
		if (ret == 0) {
			ret = LiveBooster_json_add_section_start(rsc_ptr->rsc_name, buf_ptr, buf_len);
		}
		if (ret == 0) {
			ret = LiveBooster_json_add_name_str("v", rsc_ptr->rsc_version_ptr, buf_ptr, buf_len);
		}

		// metadata section: empty
		if (ret == 0) {
			ret = LiveBooster_json_add_section_start("m", buf_ptr, buf_len);
		}
		if (ret == 0) {
			ret = LiveBooster_json_add_section_end(buf_ptr, buf_len);
		}

		if (ret == 0) {
			ret = LiveBooster_json_add_section_end(buf_ptr, buf_len);
		}
		if (ret) {
			return NULL;
		}
		rsc_ptr++;
	}
	ret = LiveBooster_json_end_section(buf_ptr, buf_len);
	if (ret) {
		return NULL;
	}
	return buf_ptr;
}

/* --------------------------------------------------------------------------------- */
/*  */
const char* LiveBooster_msg_encode_params_all_buf(char* buf_ptr, uint32_t buf_len, const LiveBooster_ArrayOfParams_t* params_array,
		int32_t cid) {
	int ret, i;
	const LiveBooster_Param_t* param_ptr;
	ret = LiveBooster_json_begin_section(buf_ptr, buf_len, "cfg");
	if (ret) {
		return NULL;
	}
	param_ptr = params_array->param_ptr;
	for (i = 0; i < params_array->param_nb; i++) {
		ret = LiveBooster_json_add_param(&param_ptr->parm_data, buf_ptr, buf_len);
		if (ret) {
			return NULL;
		}
		param_ptr++;
	}

	if (cid) {
		if (ret == 0) {
			ret = LiveBooster_json_add_section_end(buf_ptr, buf_len);
		}

		if (ret == 0) {
			ret = LiveBooster_json_add_name_int("cid", cid, buf_ptr, buf_len);
		}
		if (ret == 0) {
			ret = LiveBooster_json_end(buf_ptr, buf_len);
		}
	}
	else {
		ret = LiveBooster_json_end_section(buf_ptr, buf_len);
		if (ret) {
			return NULL;
		}
	}
	return buf_ptr;
}

/* --------------------------------------------------------------------------------- */
/*  */
static const char* LiveBooster_msg_encode_cmd_resp_buf(char* buf_ptr,
		                                               uint32_t buf_len,
													   int32_t cid,
		                                               const LiveBooster_Data_t* data_ptr,
		                                               int data_nb) {
	int ret;

	if (cid == 0) {
		return NULL;
	}

	ret = LiveBooster_json_begin_section(buf_ptr, buf_len, "res");

	if ((ret == 0) && (data_ptr) &&(data_nb > 0)) {
		int i;
		const LiveBooster_Data_t* p_data = data_ptr;
		for (i = 0; i < data_nb; i++) {
			ret = LiveBooster_json_add_item(p_data, buf_ptr, buf_len);
			if (ret) {
				break;
			}
			p_data++;
		}
	}

	if (ret == 0)
		ret = LiveBooster_json_add_section_end(buf_ptr, buf_len);

	if (ret == 0)
		ret = LiveBooster_json_add_name_int("cid", cid, buf_ptr, buf_len);

	if (ret == 0)
		ret = LiveBooster_json_end(buf_ptr, buf_len);

	return ((ret == 0) ? buf_ptr : NULL);
}

/* ================================================================================= */

/* --------------------------------------------------------------------------------- */
/*  */

static char _LiveBooster_msg_buf[LB_JSON_BUF_SZ];

/* --------------------------------------------------------------------------------- */
/*  */
static const char* lib_rsc_res[] = {
	"OK",
	"INTERNAL_ERROR",
	"UNKNOWN_RESOURCE",
	"WRONG_SOURCE_VERSION",
	"INVALID_RESOURCE",
	"NOT_AUTHORIZED",
	"BUSY"
};

const char* LiveBooster_msg_encode_rsc_result(int32_t cid, LiveBooster_ResourceRespCode_t result) {
	int ret;

	if (cid == 0) {
		return NULL;
	}

	ret = LiveBooster_json_begin(_LiveBooster_msg_buf, LB_JSON_BUF_SZ);

	if (ret == 0) {
		int res_idx = result;
		if ((res_idx < 0) || (res_idx >= RCP_RSP_MAX))
			res_idx = RSC_RSP_ERR_INTERNAL_ERROR;
		ret = LiveBooster_json_add_name_str("res", lib_rsc_res[res_idx], _LiveBooster_msg_buf, LB_JSON_BUF_SZ);
	}

	if (ret == 0) {
		ret = LiveBooster_json_add_name_int("cid", cid, _LiveBooster_msg_buf, LB_JSON_BUF_SZ);
	}

	if (ret == 0) {
		ret = LiveBooster_json_end(_LiveBooster_msg_buf, LB_JSON_BUF_SZ);
	}
	return (ret == 0) ? _LiveBooster_msg_buf : NULL;
}


/* --------------------------------------------------------------------------------- */
/*  */
const char* LiveBooster_msg_encode_rsc_error(char* error, char* errorDetails) {
	int ret;

	if ((error == NULL) || (errorDetails == NULL))  {
		return NULL;
	}

	ret = LiveBooster_json_begin(_LiveBooster_msg_buf, LB_JSON_BUF_SZ);

	if (ret == 0) {
		ret = LiveBooster_json_add_name_str("errorCode", error, _LiveBooster_msg_buf, LB_JSON_BUF_SZ);
	}

	if (ret == 0) {
		ret = LiveBooster_json_add_name_str("errorDetails", errorDetails, _LiveBooster_msg_buf, LB_JSON_BUF_SZ);
	}

	if (ret == 0) {
		ret = LiveBooster_json_end(_LiveBooster_msg_buf, LB_JSON_BUF_SZ);
	}
	return (ret == 0) ? _LiveBooster_msg_buf : NULL;
}

/* --------------------------------------------------------------------------------- */
/*  */
const char* LiveBooster_msg_encode_params_update(const LiveBooster_SetofUpdatedParams_t* pParamUpdateSet) {
	int ret;

	if (pParamUpdateSet == NULL) {
		return NULL;
	}

	if (pParamUpdateSet->cid == 0) {
		return NULL;
	}

	if ((pParamUpdateSet->nb_of_params == 0) || (pParamUpdateSet->tab_of_param_ptr[0] == NULL)) {
		return NULL;
	}

	ret = LiveBooster_json_begin_section(_LiveBooster_msg_buf, LB_JSON_BUF_SZ, "cfg");

	if (ret == 0) {
		int i;
		const LiveBooster_Param_t* param_ptr;
		for (i = 0; i < pParamUpdateSet->nb_of_params; i++) {
			param_ptr = pParamUpdateSet->tab_of_param_ptr[i];
			if (param_ptr == NULL) {
				break;
			}
			ret = LiveBooster_json_add_param(&param_ptr->parm_data, _LiveBooster_msg_buf, LB_JSON_BUF_SZ);
			if (ret) {
				break;
			}
		}
	}
	if (ret == 0) {
		ret = LiveBooster_json_add_section_end(_LiveBooster_msg_buf, LB_JSON_BUF_SZ);
		if (ret) {
		}
	}

	if (ret == 0) {
		ret = LiveBooster_json_add_name_int("cid", pParamUpdateSet->cid, _LiveBooster_msg_buf,
		LB_JSON_BUF_SZ);
	}

	if (ret == 0) {
		ret = LiveBooster_json_end(_LiveBooster_msg_buf, LB_JSON_BUF_SZ);
	}
	return (ret == 0) ? _LiveBooster_msg_buf : NULL;
}


/* --------------------------------------------------------------------------------- */
/*  */
static const char* lib_res[] = {
	"Invalid",
	"Bad format",
	"Not supported",
	"Not processed"
};

const char* LiveBooster_msg_encode_cmd_result(int32_t cid, int result) {
	int ret;

	if (cid == 0) {
		return NULL;
	}

	ret = LiveBooster_json_begin_section(_LiveBooster_msg_buf, LB_JSON_BUF_SZ, "res");

	if (ret == 0) {
		if (result < 0) {
			int err_idx = -result - 1;
			ret = LiveBooster_json_add_name_int("LiveBooster_err_code", result, _LiveBooster_msg_buf,
			LB_JSON_BUF_SZ);

			if ((ret == 0) && (err_idx >= 0) && (err_idx < 4)) {
				ret = LiveBooster_json_add_name_str("LiveBooster_error", lib_res[err_idx], _LiveBooster_msg_buf,
				LB_JSON_BUF_SZ);
			}
		}
		else if (result >= 0) { // User code
			ret = LiveBooster_json_add_name_str("Result", "OK", _LiveBooster_msg_buf, LB_JSON_BUF_SZ);
		}
	}

	if (ret == 0) {
		ret = LiveBooster_json_add_section_end(_LiveBooster_msg_buf, LB_JSON_BUF_SZ);
	}

	if (ret == 0) {
		ret = LiveBooster_json_add_name_int("cid", cid, _LiveBooster_msg_buf, LB_JSON_BUF_SZ);
	}

	if (ret == 0) {
		ret = LiveBooster_json_end(_LiveBooster_msg_buf, LB_JSON_BUF_SZ);
	}
	return (ret == 0) ? _LiveBooster_msg_buf : NULL;
}

/* ================================================================================= */

/* --------------------------------------------------------------------------------- */
/*  */
const char* LiveBooster_msg_encode_cmd_resp(int32_t cid, const LiveBooster_Data_t* data_ptr, int data_nb) {

	const char *p_msg;
	p_msg = LiveBooster_msg_encode_cmd_resp_buf(_LiveBooster_msg_buf, LB_JSON_BUF_SZ, cid, data_ptr, data_nb);
	return p_msg;
}

/* --------------------------------------------------------------------------------- */
/*  */

const char* LiveBooster_msg_encode_status(const LiveBooster_ArrayOfData_t* pObjSet) {
	const char *p_msg;

	if (pObjSet == NULL) {
		return NULL;
	}
	if ((pObjSet->data_nb == 0) || (pObjSet->data_ptr == NULL)) {
		return NULL;
	}

	p_msg = LiveBooster_msg_encode_status_buf(_LiveBooster_msg_buf, LB_JSON_BUF_SZ, pObjSet);

	return p_msg;
}

/* --------------------------------------------------------------------------------- */
/*  */
const char* LiveBooster_msg_encode_data(const LiveBooster_SetOfData_t* pSetData) {
	const char *p_msg;

	if ((pSetData == NULL) || (pSetData->stream_id[0] == 0))
		return NULL;

	if ((pSetData->data_set.data_nb == 0) || (pSetData->data_set.data_ptr == NULL))
		return NULL;

	p_msg = LiveBooster_msg_encode_data_buf(_LiveBooster_msg_buf, LB_JSON_BUF_SZ, pSetData);

	return p_msg;
}

/* --------------------------------------------------------------------------------- */
/*  */

const char* LiveBooster_msg_encode_resources(const LiveBooster_SetOfResources_t* pSetResources) {
	const char *p_msg;

	if (pSetResources == NULL) {
		return NULL;
	}
	if ((pSetResources->rsc_nb == 0) || (pSetResources->rsc_ptr == NULL)) {
		return NULL;
	}

	p_msg = LiveBooster_msg_encode_resources_buf(_LiveBooster_msg_buf, LB_JSON_BUF_SZ, pSetResources);

	return p_msg;
}

/* --------------------------------------------------------------------------------- */
/*  */
const char* LiveBooster_msg_encode_params_all(const LiveBooster_ArrayOfParams_t* params_array, int32_t cid) {
	const char *p_msg;
	if (params_array == NULL) {
		return NULL;
	}
	if ((params_array->param_nb == 0) || (params_array->param_ptr == NULL)) {
		return NULL;
	}

	p_msg = LiveBooster_msg_encode_params_all_buf(_LiveBooster_msg_buf, LB_JSON_BUF_SZ, params_array, cid);

	return p_msg;
}
