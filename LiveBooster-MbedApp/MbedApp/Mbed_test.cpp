/*
 * === Soft Serial Test for NUCLEO L476RG board ===
 *
 * Pins configuration :
 *    o Pins SERIAL_TX, SERIAL_RX used for link to PC.
 *    o Pins D8 (SerialPort_TX),  D2 (SerialPort_RX) use for link to Heracles board (NUCLEO_L476RG).
 *    o Pins PE_8 (SerialPort_TX), PE_7 (SerialPort_RX) use for link to Heracles board (NUCLEO_F746ZG, NUCLEO_F429ZI).
 *    o Pin D4 set to HIGHT (reset of Heracles shield board).
*/


#include "LiveBooster-C-Library/LiveBooster.h"

#include "MbedSerialImpl.h"
#include "MbedTimerImpl.h"
#include "MbedDebugImpl.h"

/* define pin for SerialPort (NUCLEO_L476RG board) */
//PinName SerialPortTX = D8;
//PinName SerialPortRX = D2;

/* define pin for SerialPort (NUCLEO_F746ZG or NUCLEO_F429ZI board) */
PinName SerialPortTX = PE_8;
PinName SerialPortRX = PE_7;

unsigned long tmp;
DigitalOut PinRst(D4);
DigitalOut myled(LED1);

Serial DebugSerial (SERIAL_TX,SERIAL_RX,115200);
#define PRINTF DebugSerial.printf


/* Parameter to connect LiveObject */
char deviceId[]  = "urn:lo:nsid:LiveBooster:test";
unsigned long long apiKeyP1 = 0x123456789ABCDEFG; // Force compilation error (user must be change the API key)
unsigned long long apiKeyP2 = 0xFEDCBA9876543210;

const char* appv_version = "Sample_Mbed_test V00.01";

/* ===> DATA <=== */

int32_t  appv_measures_temp_grad = -1;
float    appv_measures_volt_grad = -0.2;

// Contains a counter incremented after each data sent
uint32_t appv_measures_counter = 0;

// Contains the temperature level
int32_t  appv_measures_temp = 20;

// Contains the battery level
float    appv_measures_volt = 5.0;

// Set of Collected data (published on a data stream)
LiveBooster_Data_t appv_set_measures[] = {
  { LB_TYPE_UINT32, "counter" ,        &appv_measures_counter, 1 },
  { LB_TYPE_INT32,  "temperature" ,    &appv_measures_temp, 1 },
  { LB_TYPE_FLOAT,  "battery_level" ,  &appv_measures_volt, 1 }
};
#define SET_MEASURES_NB (sizeof(appv_set_measures) / sizeof(LiveBooster_Data_t))

int      appv_hdl_data = -1;
LiveBooster_GpsFix_t gpsData = {1, 45.0, 5.0};
char timestamp[] = "2018-09-25T13:22:30Z";

/* ===> PARAMETERS <=== */

/* Definition parameters */
// definition of identifier for each kind of parameters
#define PARM_IDX_CFG_STR 1
#define PARM_IDX_CFG_I32 2
#define PARM_IDX_CFG_U32 3
#define PARM_IDX_CFG_F64 4
#define PARM_IDX_CFG_BIN 5

// Set of configuration parameters
typedef struct  {
	char cfg_str[20];
	long int cfg_i32;
	unsigned long int cfg_u32;
	float cfg_f64;
	char cfg_bin[400];
} ParamSet_t;

ParamSet_t appv_conf = { "Hello LiveBooster !", -1, 4294967295,  -123.987654321, "7878686"};

LiveBooster_Param_t setOfParam[] = {
{ PARM_IDX_CFG_STR, { LB_TYPE_STRING_C, "cfg_str" , appv_conf.cfg_str, 1 } },
{ PARM_IDX_CFG_I32, { LB_TYPE_INT32, "cfg_i32" , &appv_conf.cfg_i32, 1 } },
{ PARM_IDX_CFG_U32, { LB_TYPE_UINT32, "cfg_u32", &appv_conf.cfg_u32, 1 } },
{ PARM_IDX_CFG_F64, { LB_TYPE_FLOAT, "cfg_f64" , &appv_conf.cfg_f64, 1 } },
{ PARM_IDX_CFG_BIN, { LB_TYPE_BIN, "cfg_bin" , appv_conf.cfg_bin, 1 } }
};
#define SET_PARAM_NB (sizeof(setOfParam) / sizeof(LiveBooster_Param_t))


int paramUdpdateCb (const LiveBooster_Param_t *ptrParam, const void *value, int len) {

	int paramIsOk = true;

	if (ptrParam == NULL) {
		return REFUSE;
	}
	switch (ptrParam->parm_uref) {
		case PARM_IDX_CFG_STR: {
			PRINTF("PARM_IDX_CFG_STR\n");
		    strcpy(appv_conf.cfg_str,(const char*)value);
			if (paramIsOk) {
				return OK;
			}
			break;
		}
		case PARM_IDX_CFG_I32: {
			PRINTF("PARM_IDX_CFG_I32\n");
			if (paramIsOk) {
				return OK;
			}
			break;
		}
	case PARM_IDX_CFG_U32: {
		    PRINTF("PARM_IDX_CFG_U32\n");
			if (paramIsOk) {
				return OK;
			}
			break;
		}
	case PARM_IDX_CFG_F64: {
	         PRINTF("PARM_IDX_CFG_F64\n");
			if (paramIsOk) {
				return OK;
			}
			break;
	    }
	case PARM_IDX_CFG_BIN: {
	    PRINTF("PARM_IDX_CFG_BIN\n");
	    strcpy(appv_conf.cfg_bin,(const char*)value);
		if (paramIsOk) {
			return OK;
		}
		break;
	    }
	}
	return REFUSE;
}

/* ===> COMMANDS <=== */

/* Definition parameters */
// definition of identifier for each kind of parameters
#define CMD_IDX_RESET 1
#define CMD_IDX_COLOR 2
#define CMD_IDX_DATA 3

typedef struct  {
	int R;
	int V;
	int B;
} Color_t;

typedef struct  {
	int ToPublish;
	unsigned int PeriodInMs;
} Data_t;

#define NOT_PUBLISH_DATA 0
#define PUBLISH_DATA 1

static Color_t ColorLed = {255, 255, 255};
static Data_t Data = {NOT_PUBLISH_DATA, 60000};

LiveBooster_Command_t setOfcommands[] = {
{ CMD_IDX_RESET, "RESET", 0 },
{ CMD_IDX_COLOR, "COLOR", 0 },
{ CMD_IDX_DATA, "DATA", 0 }
};
#define SET_COMMANDS_NB (sizeof(setOfcommands) / sizeof(LiveBooster_Command_t))

static int main_cmd_reset(void) {
    PRINTF("*** RESET (but do nothing).\n");
    return OK;
}

static int main_cmd_color(const LiveBooster_CommandRequestBlock_t* pCmdReqBlk) {
    Color_t valColor = ColorLed;

    if (pCmdReqBlk->hd.cmd_args_nb == 0) {
      PRINTF("main_cmd_color: No ARG\n");
      return -2; /* Bad Format */
    }
    else {
      unsigned int i;
      for (i = 0; i < pCmdReqBlk->hd.cmd_args_nb; i++) {
    	if ((pCmdReqBlk->args_array[i].arg_type == 0) &&
    	     (atoi(pCmdReqBlk->args_array[i].arg_value) >= 0) &&
    	     (atoi(pCmdReqBlk->args_array[i].arg_value) <= 255)) {
    	   if ( !strncasecmp("R",pCmdReqBlk->args_array[i].arg_name,1) )
     	    	 valColor.R = atoi(pCmdReqBlk->args_array[i].arg_value);
     	   else if ( !strncasecmp("V",pCmdReqBlk->args_array[i].arg_name,1) )
    	     valColor.V = atoi(pCmdReqBlk->args_array[i].arg_value);
    	   else if ( !strncasecmp("B",pCmdReqBlk->args_array[i].arg_name,1) )
    	     valColor.B = atoi(pCmdReqBlk->args_array[i].arg_value);
    	   else
    		 return -2;  /* Bad Format */
    	}
    	else {
    		return -1; /* Invalid */
        }
    	ColorLed = valColor;
       }
    }
    char trace[150];
   return OK;
}

static int main_cmd_data(const LiveBooster_CommandRequestBlock_t* pCmdReqBlk) {
    if (pCmdReqBlk->hd.cmd_args_nb == 0) {
      PRINTF("main_cmd_data: No ARG\n");
      return -2; /* Bad Format */
    }
    else {
      unsigned int i;
      for (i = 0; i < pCmdReqBlk->hd.cmd_args_nb; i++) {
    	if (pCmdReqBlk->args_array[i].arg_type == 0) {
    	   if ( !strncasecmp("Publish",pCmdReqBlk->args_array[i].arg_name,7) )
     	    	 Data.ToPublish = !(atoi(pCmdReqBlk->args_array[i].arg_value)) ? NOT_PUBLISH_DATA : PUBLISH_DATA;
     	   else if ( !strncasecmp("PeriodInMs",pCmdReqBlk->args_array[i].arg_name,10) )
     		  Data.PeriodInMs = atoi(pCmdReqBlk->args_array[i].arg_value);
    	   else
    		 return -2;  /* Bad Format */
    	}
    	else {
    		return -1; /* Invalid */
        }
      }
    }
   return OK;
}


int mainCommand (const LiveBooster_CommandRequestBlock_t *pCmdReqBlk) {

    int res= OK;
    const LiveBooster_Command_t*  cmd_ptr;

    if ((pCmdReqBlk == NULL) || (pCmdReqBlk->hd.cmd_ptr == NULL) || (pCmdReqBlk->hd.cmd_cid == 0) ) {
    	PRINTF("*** COMMAND : ERROR, Invalid parameter\n");
      return REFUSE;
    }

    cmd_ptr = pCmdReqBlk->hd.cmd_ptr;

	switch (cmd_ptr->cmd_uref) {
		case CMD_IDX_RESET: {
	        res = main_cmd_reset();
			break;
		}
		case CMD_IDX_COLOR: {
	        res = main_cmd_color(pCmdReqBlk);
			break;
		}
		case CMD_IDX_DATA: {
	        res = main_cmd_data(pCmdReqBlk);
			break;
		}
	}
	return res;
}

/* ===> RESOURCES <=== */

#define RSC_IDX_MESSAGE     1
#define RSC_IDX_IMAGE       2
#define RSC_IDX_BINARY      3

char appv_rv_image[10]    = "01.00";
char appv_rsc_image[10 * 1024] = "";

char appv_rv_binary[10]   = "01.00";
char appv_rsc_binary[1024];

char appv_rv_message[10]  = "01.00";
char appv_rsc_message[1024];

// Set of resources
LiveBooster_Resource_t appv_set_resources[] = {
  { RSC_IDX_IMAGE,   "image",   appv_rv_image,   sizeof(appv_rv_image) - 1 },
  { RSC_IDX_BINARY,  "binary",  appv_rv_binary,  sizeof(appv_rv_binary) - 1 },
  { RSC_IDX_MESSAGE, "message", appv_rv_message, sizeof(appv_rv_message) - 1 } // resource used to update appv_status_message
};
#define SET_RESOURCES_NB (sizeof(appv_set_resources) / sizeof(LiveBooster_Resource_t))

// variables used to process the current resource transfer
uint32_t appv_rsc_size = 0;
uint32_t appv_rsc_offset = 0;

// ----------------------------------------------------------
// RESOURCE Callback Functions
//

//  Called (by the LiveObjects thread) to notify either,
//  - state = 0  : the begin of resource request
//  - state = 1  : the end without error
//  - state != 1 : the end with an error

LiveBooster_ResourceRespCode_t main_cb_rsc_ntfy (uint8_t state,
		                                         const LiveBooster_Resource_t* rsc_ptr,
                                                 const char* version_old, const char* version_new,
												 uint32_t size) {
	char trace[100];

	LiveBooster_ResourceRespCode_t ret = RSC_RSP_OK; // OK to update the resource

	sprintf(trace,"\n*** rsc_ntfy: ...\n"); PRINTF(trace);

  if ((rsc_ptr) && (rsc_ptr->rsc_uref > 0) && (rsc_ptr->rsc_uref <= SET_RESOURCES_NB)) {
    sprintf(trace,"***   user ref     = %d\n", rsc_ptr->rsc_uref); PRINTF(trace);
    sprintf(trace,"***   name         = %s\n", rsc_ptr->rsc_name); PRINTF(trace);
    sprintf(trace,"***   version_old  = %s\n", version_old); PRINTF(trace);
    sprintf(trace,"***   version_new  = %s\n", version_new); PRINTF(trace);
    sprintf(trace,"***   size         = %u\n", (unsigned int)size); PRINTF(trace);
    if (state) {
      if (state == 1) { // Completed without error
    	  sprintf(trace,"***   state   = COMPLETED without error\n"); PRINTF(trace);
        // Update version
    	  sprintf(trace," => UPDATE - version %s to %s\n", rsc_ptr->rsc_version_ptr, version_new); PRINTF(trace);
        strncpy((char*)rsc_ptr->rsc_version_ptr, version_new, rsc_ptr->rsc_version_sz);
        if (rsc_ptr->rsc_uref == RSC_IDX_IMAGE) {
          PRINTF("\n\n");
          PRINTF(appv_rsc_image);
          PRINTF("\n");
        }
      }
      else {
    	  sprintf(trace,"***   state    = COMPLETED with error !!\n"); PRINTF(trace);
      	  ret = RSC_RSP_ERR_INTERNAL_ERROR;
      }
      appv_rsc_offset = 0;
      appv_rsc_size = 0;
    }
    else { // Begin of resource request
      appv_rsc_offset = 0;
      ret = RSC_RSP_ERR_NOT_AUTHORIZED;
      switch (rsc_ptr->rsc_uref ) {
        case RSC_IDX_MESSAGE:
          if (size < (sizeof(appv_rsc_message) - 1)) {
            ret = RSC_RSP_OK;
          }
          break;
        case RSC_IDX_IMAGE:
          if (size < (sizeof(appv_rsc_image) - 1)) {
            ret = RSC_RSP_OK;
          }
          break;
        case RSC_IDX_BINARY:
          ret = RSC_RSP_OK;
          break;
      }
      if (ret == RSC_RSP_OK) {
        appv_rsc_size = size;
        sprintf(trace,"***   state        = START - ACCEPTED\n"); PRINTF(trace);
      }
      else {
        appv_rsc_size = 0;
        sprintf(trace,"***   state        = START - REFUSED\n"); PRINTF(trace);
      }
    }
  }
  else {
	  sprintf(trace,"***  UNKNOWN USER REF (x%p %d)  in state=%d\n", rsc_ptr, rsc_ptr->rsc_uref, state); PRINTF(trace);
    ret = RSC_RSP_ERR_INVALID_RESOURCE;
  }
  return ret;
}

// Called (by the LiveObjects thread) to request the user
// to read data from current resource transfer.
int main_cb_rsc_data (const LiveBooster_Resource_t* rsc_ptr, uint32_t offset)
{
  int ret;
  char trace [100];

  if (rsc_ptr->rsc_uref == RSC_IDX_MESSAGE) {
    char buf[40];
    if (offset > (sizeof(appv_rsc_message) - 1)) {
    	sprintf(trace,"*** rsc_data: rsc[%d]='%s' offset=%u > %d - OUT OF ARRAY\n",
             rsc_ptr->rsc_uref, rsc_ptr->rsc_name, (unsigned int)offset, sizeof(appv_rsc_message) - 1); PRINTF(trace);
      return -1;
    }
    ret = LiveBooster_GetResources(rsc_ptr, buf, sizeof(buf) - 1);
    if (ret > 0) {
      if ((offset + ret) > (sizeof(appv_rsc_message) - 1)) {
    	  sprintf(trace,"*** rsc_data: rsc[%d]='%s' offset=%u - read=%d => %d > %d - OUT OF ARRAY\n",
               rsc_ptr->rsc_uref, rsc_ptr->rsc_name, (unsigned int)offset, ret, (unsigned int)(offset + ret), sizeof(appv_rsc_message) - 1); PRINTF(trace);
        return -1;
      }
      appv_rsc_offset += ret;
      memcpy(&appv_rsc_message[offset], buf, ret);
      appv_rsc_message[offset + ret] = 0;
      sprintf(trace,"*** rsc_data: rsc[%d]='%s' offset=%u - read=%d/%d '%s'\n",
             rsc_ptr->rsc_uref, rsc_ptr->rsc_name, (unsigned int)offset, ret, sizeof(buf) - 1, appv_rsc_message); PRINTF(trace);
    }
  }
  else if (rsc_ptr->rsc_uref == RSC_IDX_IMAGE) {
    if (offset > (sizeof(appv_rsc_image) - 1)) {
    	sprintf(trace,"*** rsc_data: rsc[%d]='%s' offset=%u > %d - OUT OF ARRAY\n",
             rsc_ptr->rsc_uref, rsc_ptr->rsc_name, (unsigned int)offset, sizeof(appv_rsc_image) - 1); PRINTF(trace);
      return -1;
    }
    int data_len = sizeof(appv_rsc_image) - offset - 1;
    ret = LiveBooster_GetResources(rsc_ptr, &appv_rsc_image[offset], data_len);
    if (ret > 0) {
      if ((offset + ret) > (sizeof(appv_rsc_image) - 1)) {
    	  sprintf(trace,"*** rsc_data: rsc[%d]='%s' offset=%u - read=%d => %d > %d - OUT OF ARRAY\n",
               rsc_ptr->rsc_uref, rsc_ptr->rsc_name, (unsigned int)offset, ret, (unsigned int)(offset + ret), sizeof(appv_rsc_image) - 1); PRINTF(trace);
        return -1;
      }
      appv_rsc_offset += ret;
      sprintf(trace,"*** rsc_data: rsc[%d]='%s' offset=%u - read=%d/%d - %u/%u\n",
              rsc_ptr->rsc_uref, rsc_ptr->rsc_name, (unsigned int)offset, ret, data_len, (unsigned int)appv_rsc_offset, (unsigned int)appv_rsc_size); PRINTF(trace);
    }
    else {
    	sprintf(trace,"*** rsc_data: rsc[%d]='%s' offset=%u - read error (%d) - %u/%u\n",
             rsc_ptr->rsc_uref, rsc_ptr->rsc_name, (unsigned int)offset, ret, (unsigned int)appv_rsc_offset, (unsigned int)appv_rsc_size); PRINTF(trace);
    }
  }
  else if (rsc_ptr->rsc_uref == RSC_IDX_BINARY) {
    int data_len = sizeof(appv_rsc_binary) - 1;
    int index;
    ret = LiveBooster_GetResources(rsc_ptr, appv_rsc_binary, data_len);
    if (ret > 0) {
      appv_rsc_offset += ret;
      for (index=0; index < data_len; index++) {
    	  sprintf(trace,"%X", appv_rsc_binary[index]); PRINTF(trace);
      }
    }
    else {
      sprintf(trace,"*** rsc_data: rsc[%d]='%s' offset=%u - read error (%d) - %u/%u\n",
             rsc_ptr->rsc_uref, rsc_ptr->rsc_name, (unsigned int)offset, ret, (unsigned int)appv_rsc_offset, (unsigned int)appv_rsc_size); PRINTF(trace);
    }
  }
  else {
    ret = -1;
  }
  return ret;
}



/* ==> APPLICATION <== */

/* contains the user application */
static void thread_main(const void * args)
{
    int res;
    unsigned long timeInMs = 0;
    unsigned int timeToStopInMs;
    char trace[30];

    PinRst = 1;

    /* 1 - Initializing LiveBooster Instance */
    PRINTF ("\nLiveBooster Init:\n");
    res = LiveBooster_Init(deviceId, apiKeyP1, apiKeyP2, &mbedSerialImpl, &mbedTimerImpl, &mbedDebugImpl);

    /* 2 -  Attach parameters, commands, resources, status, data*/
    if (res == OK) {
        PRINTF ("\nAttach parameters, commands:\n");
    	res = LiveBooster_AttachCfgParameters  (setOfParam, SET_PARAM_NB, paramUdpdateCb);
    	res = LiveBooster_AttachCommands(setOfcommands, SET_COMMANDS_NB, mainCommand);


    	res = LiveBooster_AttachResources(appv_set_resources, SET_RESOURCES_NB,
    			                          main_cb_rsc_ntfy, main_cb_rsc_data);

    	appv_hdl_data = LiveBooster_AttachData(deviceId, "mV1", "\"Valence\"", NULL,
    			                               &gpsData, appv_set_measures, SET_MEASURES_NB);
    }

    /* 3 - Connect to LiveObject server */
    if (res == OK) {
       PRINTF ("\nLiveBooster Connect:\n");
       res = LiveBooster_Connect();
    }

    /* 4 - Survey data from LiveObject platform */
    if (res == OK) {
	   PRINTF ("\nStart LiveBooster cycle:\n");
	   /* Run one hour */
	   timeToStopInMs = mbedTimerImpl.millis() + 3600000;

	   while (mbedTimerImpl.millis() < timeToStopInMs) {
			 if (res < 0) {
				 sprintf(trace,"ERROR number : %d  ==> LiveBooster re-Connect:\n",res);
				 PRINTF (trace);
				 res = LiveBooster_Connect();
			 }
			 res = LiveBooster_Cycle(1000);
			 /* User application
			  *
			  * .... */
			// Simulate measures : Voltage and Temperature ...
			if (appv_measures_volt <= 0.0)       appv_measures_volt_grad = 0.2;
			else if (appv_measures_volt >= 5.0)  appv_measures_volt_grad = -0.3;

			if (appv_measures_temp <= -3)        appv_measures_temp_grad = 1;
			else if (appv_measures_temp >= 20)  appv_measures_temp_grad = -1;

			appv_measures_volt += appv_measures_volt_grad;
			appv_measures_temp += appv_measures_temp_grad;
			appv_measures_counter++;

			if ((Data.ToPublish == PUBLISH_DATA) &&
					( (mbedTimerImpl.millis() - timeInMs) >= Data.PeriodInMs)) {
				timeInMs = mbedTimerImpl.millis();
				res = LiveBooster_PushData(appv_hdl_data);
			}
    	}
    }
}

/* Stack size fixed to 16K octets */
osThreadDef(thread_main, osPriorityNormal, 16 * 1024);

int main(void)
{
    /* Create and start thread */
	osThreadCreate(osThread(thread_main), NULL);
}
