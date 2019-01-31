# LiveBooster library Errors

| Code     | LABEL  (tag used in coding)          | Descriptions                                             |
| -------- | ------------------------------------ | -------------------------------------------------------- |
| 0        | MQTT_SUCCESS or LB_SUCCESS           | No Error                                                 |
| -8       | ERR_MQTT_BUFFER_OVERFLOW             | Incorrect buffer size                                    |
| -9       | FAILURE_PINGRESP_NOT_RECEIVED        | No response on KeepAlive request                         |
| -10      | ERR_MQTT_CONNECT                     | Error on MQTT connection                                 |
| -11      | ERR_MQTT_WAIT_FOR_CONNACT            | Acknowledge connection ("CONNACT") not received          |
| -12      | ERR_MQTT_DESERIALIZE_CONNACT         | De-serialization "CONNACT" data incorrect                |
| -13      | ERR_MQTT_SET_MESSAGE_HANDLER         | Initialization handler message failure                   |
| -14      | ERR_MQTT_SUBSCRIBE                   | Subscription on MQTT topic incorrect                     |
| -15      | ERR_MQTT_UNSUBSCRIBE                 | Unsubscription from MQTT topic incorrect                 |
| -16      | ERR_MQTT_PUBLISH                     | Publication failure on MQTT topic                        |
| -17      | ERR_MQTT_DESERIALIZE_ACK             | De-serialization Acknowledge data incorrect              |
| -18      | ERR_MQTT_WAIT_FOR_PUBACT             | Acknowledge publication ("PUBACT") not received          |
| -19      | ERR_MQTT_WAIT_FOR_PUBCOMP            | Acknowledge publication complete ("PUBOMP") not received |
| -20      | ERR_MQTT_DISCONNECT                  | Problem MQTT disconnection                               |
| -21      | ERR_MQTT_SENT_PACKET                 | Problem MQTT packet sending                              |
| -30      | ERR_LB_CYCLE                         | MQTT is disconnected                                     |
| -31      | ERR_LB_ATTACH_DATA                   | Configuration of the data handler incorrect              |
| -32      | ERR_LB_PUSH_DATA                     | Handler data unknown                                     |
| -33      | ERR_LB_GET_RESOURCES                 | No resources received                                    |
| -34      | ERR_LB_HANDLER_PROCESS_GET_RSC       | Received resources incorrect                             |
| -40      | ERR_LB_HTTP_READ_LINE_NULL           | Empty line in resources header                           |
| -41      | ERR_LB_HTTP_READ_LINE_SMALL_BUFFER   | Incorrect buffer length                                  |
| -42      | ERR_LB_HTTP_READ_LINE                | Error while reading the HTTP GET response                |
| -43      | ERR_LB_HTTP_QUERY_WRITE              | Error while sending HTTP GET query                       |
| -44      | ERR_LB_HTTP_QUERY_INCORRECT_ANSWER   | Not a correct HTTP answer                                |
| -45      | ERR_LB_HTTP_QUERY_INCORRECT_CODE     | Unexpected HTTP Resp code                                |
| -46      | ERR_LB_HTTP_NULL_CONTENT_LENGTH      | Null resources content size                              |
| -47      | ERR_LB_HTTP_INCORRECT_CONTENT_LENGTH | Incorrect resources content size                         |
| -48      | ERR_LB_HTTP_START_NULL               | Null data to start HTTP                                  |
| -49      | ERR_LB_HTTP_START_URI_ERROR          | HTTP URL incorrect                                       |
| -50      | ERR_LB_HTTP_START_PORT_NOT_FOUND     | PORT unknown                                             |
| -51      | ERR_LB_HTTP_START_URL_NOT_FOUND      | HTTP URL unknown                                         |
| -52      | ERR_LB_HTTP_START_FAIL_CONNEXION     | HTTP connection fail                                     |
| -53      | ERR_LB_HTTP_DATA_DISCONNECTED        | HTTP disconnected                                        |
| -54      | ERR_LB_HTTP_STOPPED                  | Data not read, HTTP stopped                              |
| -1       | FAILURE                              | Others errors (not detailed)                             |
| -2 to -6 | *Not defined*                        | Decoded messages errors                                  |
