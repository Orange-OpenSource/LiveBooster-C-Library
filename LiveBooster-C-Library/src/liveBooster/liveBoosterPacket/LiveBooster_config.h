/*
 * Copyright (C) 2016 Orange
 *
 * This software is distributed under the terms and conditions of the 'BSD-3-Clause'
 * license which can be found in the file 'LICENSE.txt' in this package distribution
 * or at 'https://opensource.org/licenses/BSD-3-Clause'.
 */

/**
 * @file  LiveBooster_Config.h
 * @brief Default configuration for LiveObjects Client library
 *
 * To implement or not a Live Objects Feature, set to 1 or 0 (by default 1, it is implemented):
 * - LB_FEATURE_LO_STATUS    'Status/Info' feature.
 * - LB_FEATURE_LO_PARAMS    'Configuration Parameters' feature.
 * - LB_FEATURE_LO_DATA      'Collected Data' feature.
 * - LB_FEATURE_LO_COMMANDS  'Commands' feature.
 * - LB_FEATURE_LO_RESOURCES 'Resources' feature.
 * And
 *  - LB_MQTT_DUMP_MSG        Dump MQTT message - set to 1 = text only, 2 = hexa only, 3 = text+hexa
 *
 * Tunable parameters:

 * - LB_MAX_OF_DATA_SET  Max Number of collected data streams (or also named 'data sets')  (default: 5 data streams)
 * - LB_MAX_OF_PARSED_PARAMS Max Number of parsed parameters in a same received update param request (default: 5)
 * - LB_JSON_BUF_SZ  Size (in bytes) of static JSON buffer used to encode the JSON payload to be sent (default: 1 K bytes)
 * - LB_BIN64_BUF_SZ  Size (in bytes) of Bin64 configuration parameter buffer used to decode the JSON payload to be receive (default: 550 bytes)
 * - LB_SETOFDATA_STREAM_ID_SZ Max Size(in bytes) of Data Stream Id (default: 80 bytes)
 * - LB_SETOFDATA_MODEL_SZ Max Size(in bytes) of Data Model field (default: 80 bytes). It can be set to 0 : disabled.
 * - LB_SETOFDATA_TAGS_SZ Max Size(in bytes) of Data Tag field (default: 80 bytes). It can be set to 0 : disabled.
 *
 */

#ifndef __LiveBooster_Config_H_
#define __LiveBooster_Config_H_

#ifndef LB_MAX_OF_DATA_SET
#define LB_MAX_OF_DATA_SET                  5
#endif

#ifndef LB_MAX_OF_PARSED_PARAMS
#define LB_MAX_OF_PARSED_PARAMS             5
#endif

#ifndef LB_JSON_BUF_SZ
#define LB_JSON_BUF_SZ                      1024
#endif

#ifndef LB_BIN64_BUF_SZ
#define LB_BIN64_BUF_SZ                      550
#endif

#ifndef LB_SETOFDATA_STREAM_ID_SZ
#define LB_SETOFDATA_STREAM_ID_SZ            80
#endif

#ifndef LB_SETOFDATA_MODEL_SZ
#define LB_SETOFDATA_MODEL_SZ                80
#endif

#ifndef LB_SETOFDATA_TAGS_SZ
#define LB_SETOFDATA_TAGS_SZ                 80
#endif


#endif /* __LiveBooster_Config_H_ */
