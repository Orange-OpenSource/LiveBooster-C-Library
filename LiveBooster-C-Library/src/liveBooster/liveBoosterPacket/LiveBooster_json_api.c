/*
 * Copyright (C) 2016 Orange
 *
 * This software is distributed under the terms and conditions of the 'BSD-3-Clause'
 * license which can be found in the file 'LICENSE.txt' in this package distribution
 * or at 'https://opensource.org/licenses/BSD-3-Clause'.
 */
/**
 * @file  LiveBooster_json_api.c
 * @brief Basic JSON functions
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "LiveBooster_json_api.h"
#include "LiveBooster_code_b64.h"

static const char* _LiveBooster_json_dataTypeStr[LB_TYPE_MAX_NOT_USED] = {
		"unknown", "i32", "u32", "str", "f64", "bin"
		/*, "max" */
};

/* --------------------------------------------------------------------------------- */
/*  */
static const char* LB_dataType(LiveBooster_Type_t data_type) {
	switch (data_type) {
	case LB_TYPE_UNKNOWN:
		return "xxx";
	case LB_TYPE_INT32:
		return "i32";
	case LB_TYPE_UINT32:
		return "u32";
	case LB_TYPE_STRING_C:
		return "str";
	case LB_TYPE_FLOAT:
		return "f64";
	case LB_TYPE_BIN:
		return "bin";
	case LB_TYPE_MAX_NOT_USED:
		return "max";
	}
	return "Unknown";
}

/* --------------------------------------------------------------------------------- */
/*  */
int LB_objTypeCheck(void) {
	int data_type;
	int err = 0;
	for (data_type = 1; data_type < LB_TYPE_MAX_NOT_USED; data_type++) {
		const char* p = LB_dataType((LiveBooster_Type_t)data_type);
		if (strcmp(_LiveBooster_json_dataTypeStr[data_type], p)) {
			err++;
		}
	}
	return err;
}

/* --------------------------------------------------------------------------------- */
/*  */
const char* LB_getDataTypeToStr(LiveBooster_Type_t data_type) {
	if (data_type < LB_TYPE_MAX_NOT_USED) {
		return _LiveBooster_json_dataTypeStr[data_type];
	}
	return "BAD";
}

/* --------------------------------------------------------------------------------- */
/*  */
LiveBooster_Type_t LB_getDataTypeFromStrL(const char* p, uint32_t len) {
	if ((p) &&(len > 0) && (len < 7)) {
		int i;
		for (i = 1; i < LB_TYPE_MAX_NOT_USED; i++) {
			if (!strncmp(_LiveBooster_json_dataTypeStr[i], p, len)) {
				return ((LiveBooster_Type_t) i);
			}
		}
	}
	return LB_TYPE_UNKNOWN;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_json_begin(char *pbuf, uint32_t sz) {
	int rc;
	rc = snprintf(pbuf, sz, "{");
	if (rc != 1) {
		return -1;
	}
	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_json_end(char *pbuf, uint32_t sz) {
	int rc;
	int len = sz - strlen(pbuf);
	char* pcur = pbuf + strlen(pbuf);
	if (*(pcur - 1) == ',') {
		pcur--;
		len++;
	}
	rc = snprintf(pcur, sz, "}");
	if (rc != 1) {
		return -1;
	}
	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_json_add_section_start(const char* section_name, char *pbuf, uint32_t sz) {
	int rc;
	int len = sz - strlen(pbuf);
	char* pcur = pbuf + strlen(pbuf);
	rc = snprintf(pcur, len, "\"%s\": {", section_name);
	if (rc < 0) {
		return -1;
	}
	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_json_add_section_end(char *pbuf, uint32_t sz) {
	int rc;
	int len = sz - strlen(pbuf);
	char* pcur = pbuf + strlen(pbuf);
	if (*(pcur - 1) == ',') {
		pcur--;
		len++;
	}
	rc = snprintf(pcur, len, "},");
	if (rc != 2) {
		return -1;
	}
	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_json_begin_section(char *pbuf, uint32_t sz, const char* section_name) {
	int rc;
	rc = snprintf(pbuf, sz, "{\"%s\":{", section_name);
	if (rc <= 0) {
		return -1;
	}
	if (rc == (int) sz) {
		return -1;
	}
	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_json_end_section(char *pbuf, uint32_t sz) {
	int rc;
	int len = sz - strlen(pbuf);
	char* pcur = pbuf + strlen(pbuf);
	if (*(pcur - 1) == ',') {
		pcur--;
		len++;
	}
	rc = snprintf(pcur, len, "}}");
	if (rc != 2) {
		return -1;
	}
	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_json_add_name_int(const char* name, int32_t value, char *pbuf, uint32_t sz) {
	int rc;
	int len = sz - strlen(pbuf);
	char* pcur = pbuf + strlen(pbuf);

	rc = snprintf(pcur, len, "\"%s\":%"PRIi32",", name, value);
	if (rc < 0) {
		return -1;
	}
	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_json_add_name_str(const char* name, const char* value, char *pbuf, uint32_t sz) {
	int rc;
	int len = sz - strlen(pbuf);
	char* pcur = pbuf + strlen(pbuf);

	rc = snprintf(pcur, len, "\"%s\":\"%s\",", name, value);
	if (rc < 0) {
		return -1;
	}
	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_json_add_name_array(const char* name, const char* array, char *pbuf, uint32_t sz) {
	int rc;
	int len = sz - strlen(pbuf);
	char* pcur = pbuf + strlen(pbuf);

	rc = snprintf(pcur, len, "\"%s\":[%s],", name, array);
	if (rc < 0) {
		return -1;
	}
	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_json_add_item(const LiveBooster_Data_t* data_ptr, char *pbuf, uint32_t sz) {
	int rc;
	short i;
	short dim;
	char* data_value_ptr;
	int len = sz - strlen(pbuf);
	char* pcur = pbuf + strlen(pbuf);

	/* Check input parameters */
	if (data_ptr == NULL) {
		return -1;
	}
	if ((data_ptr->data_name == NULL) || (data_ptr->data_value == NULL) || (data_ptr->data_dim <= 0)) {
		return -1;
	}
	
	/* Add data name */
	rc = snprintf(pcur, len, "\"%s\":", data_ptr->data_name);
	if (rc < 0) {
		return -1;
	}
	len = sz - strlen(pbuf);
	pcur = pbuf + strlen(pbuf);
	if (len < 4) { /* at least 4 free bytes remaining in buffer */
		return -1;
	}

	/* Open array if needed */
	dim = data_ptr->data_dim;
	if (dim > 1) {
		*pcur++ = '[';
		len--;
	}
	else if (dim <= 0) {
		dim = 1;
	}

	data_value_ptr = (char*)data_ptr->data_value;

	/* Add value(s) */
	for (i=0; i<dim; i++) {
		switch (data_ptr->data_type) {
		case LB_TYPE_INT32:
			rc = snprintf(pcur, len, "%"PRIi32",", *((int32_t*) data_value_ptr));
			data_value_ptr += sizeof(int32_t);
			break;
		case LB_TYPE_UINT32:
			rc = snprintf(pcur, len, "%"PRIu32",", *((uint32_t*)data_value_ptr));
			data_value_ptr += sizeof(uint32_t);
			break;
		case LB_TYPE_FLOAT:
#if defined(ARDUINO_ARCH_AVR)
		    /* Need 1 for sign + 1 digit + 1 decimal-point + 6 digits + 'e' + sign + 2 for exponent */
		    if (len < 13) {
		        return -1;
		    }
	        if (dtostre((double)(*((float*)data_value_ptr)), pcur, 6, 0) != pcur) {
	            return -1;
	        }
#else
            rc = snprintf(pcur, len, "%f,", *((float*)data_value_ptr));
#endif
            data_value_ptr += sizeof(float);
		    break;
		case LB_TYPE_STRING_C:
			rc = snprintf(pcur, len, "\"%s\",", (const char*)data_value_ptr);
			data_value_ptr += sizeof(char*);
			break;
		default:
			return -1;
		}
		/* Update remaining buffer size */
		if (dim > 1) {
			len = sz - strlen(pbuf);
			pcur = pbuf + strlen(pbuf);
			if (len < 2) { /* at least 2 free bytes remaining in buffer */
				return -1;
			}		
		}
	}

	/* Close array if needed */
	if (dim > 1) {
		pcur--;
		if ((*pcur != ',')  || (len < 2)){
			return -1;
		}
		*pcur++ = ']';
		*pcur++ = ',';
		*pcur = 0;
	}

	return 0;
}

/* --------------------------------------------------------------------------------- */
/*  */
int LiveBooster_json_add_param(const LiveBooster_Data_t* data_ptr, char *pbuf, uint32_t sz) {

    if ((data_ptr->data_type == LB_TYPE_INT32) || (data_ptr->data_type == LB_TYPE_UINT32) ||
        (data_ptr->data_type == LB_TYPE_STRING_C) || (data_ptr->data_type == LB_TYPE_FLOAT) ||
        (data_ptr->data_type == LB_TYPE_BIN))
    {
        int rc;
        int len = sz - strlen(pbuf);
        char* pcur = pbuf + strlen(pbuf);

    	char bufB64[550];
    	char *pbufB64 = bufB64;

        /* Add param name */
        rc = snprintf(pcur, len, "\"%s\":{", data_ptr->data_name);
        if (rc < 0) {
            return -1;
        }

        len = sz - strlen(pbuf);
        pcur = pbuf + strlen(pbuf);

        /* Add param type and value */
        switch (data_ptr->data_type) {
        case LB_TYPE_INT32:
            rc = snprintf(pcur, len, "\"t\":\"i32\",\"v\":%d},", *((int*) data_ptr->data_value));
            break;
        case LB_TYPE_UINT32:
            rc = snprintf(pcur, len, "\"t\":\"u32\",\"v\":%u},", *((unsigned int*) data_ptr->data_value));
            break;
        case LB_TYPE_FLOAT:
#if defined(ARDUINO_ARCH_AVR)
            /* Need 14 for param type
             *     + 1 for sign + 1 digit + 1 decimal-point + 6 digits + 'e' + sign + 2 for exponent
             *     + 2 for ending brace and comma */
            if (len < (14+13+2)) {
                return -1;
            }

            strcpy(pcur, "\"t\":\"f64\",\"v\":");
            pcur += 14;
            len -= 14;
            if (dtostre((double)(*((float*) data_ptr->data_value)), pcur, 6, DTOSTR_ALWAYS_SIGN) != pcur) {
                return -1;
            }
            pcur += 13;
            len -= 13;
            strcpy(pcur, "},");
#else
           rc = snprintf(pcur, len, "\"t\":\"f64\",\"v\":%f},", *((float*) data_ptr->data_value));
#endif
            break;
        case LB_TYPE_STRING_C:
            rc = snprintf(pcur, len, "\"t\":\"str\",\"v\":\"%s\"},", (const char*) data_ptr->data_value);
            break;
        case LB_TYPE_BIN:
        	b64_encode((const char*)data_ptr->data_value, bufB64, strlen(data_ptr->data_value));
            rc = snprintf(pcur, len, "\"t\":\"bin\",\"v\":\"%s\"},", bufB64);
            break;
        default:
            return -1;
        }
        return 0;
    }
    return -1;
}

