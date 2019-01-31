/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#ifndef __mqttClient_h
#define __mqttClient_h

#include <stdio.h>

#include "MQTTPacket/MQTTPacket.h"
#include "../heraclesGsm/TcpClientInterface.h"
#include "../heraclesGsm/HeraclesTcpClient.h"
#include "../serial/SerialInterface.h"
#include "../timer/TimerInterface.h"
#include "../traceDebug/DebugInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PACKET_ID 65535 /* according to the MQTT specification - do not change! */

#define MAX_MESSAGE_HANDLERS 5 /* redefinable - how many subscriptions do you want? */

#define MQTT_DEFAULT_SEND_SIZE    (260)
#define MQTT_DEFAULT_RECV_SIZE    (260)

#define ACK_COMMAND_TIMEOUT_IN_MS  20000

enum QoS { QOS0, QOS1, QOS2, SUBFAIL=0x80 };

/* all failure return codes must be negative */
enum returnCodeMqtt {
	                  ERR_MQTT_SENT_PACKET = -21,
                      ERR_MQTT_DISCONNECT = -20,
	                  ERR_MQTT_WAIT_FOR_PUBCOMP = -19,
	                  ERR_MQTT_WAIT_FOR_PUBACT = -18,
                      ERR_MQTT_DESERIALIZE_ACK = -17,
                      ERR_MQTT_PUBLISH = -16,
                      ERR_MQTT_UNSUBSCRIBE = -15,
                      ERR_MQTT_SUBSCRIBE = -14,
                      ERR_MQTT_SET_MESSAGE_HANDLER = -13,
                      ERR_MQTT_DESERIALIZE_CONNACT = -12,
                      ERR_MQTT_WAIT_FOR_CONNACT = -11,
                      ERR_MQTT_CONNECT = -10,
	                  FAILURE_PINGRESP_NOT_RECEIVED = -9,
					  ERR_MQTT_BUFFER_OVERFLOW = -8,
				      FAILURE = -1,
				      MQTT_SUCCESS = 0 };

typedef struct MQTTMessage
{
    enum QoS qos;
    unsigned char retained;
    unsigned char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
} MQTTMessage;

typedef struct MessageData
{
    MQTTMessage* message;
    MQTTString* topicName;
} MessageData;

typedef struct MQTTConnackData
{
    unsigned char rc;
    unsigned char sessionPresent;
} MQTTConnackData;

typedef struct MQTTSubackData
{
    enum QoS grantedQoS;
} MQTTSubackData;

typedef void (*messageHandler)(MessageData*);

typedef struct _MQTTClient
{
    unsigned int next_packetid;

    unsigned char buf[MQTT_DEFAULT_SEND_SIZE];
    unsigned char readbuf[MQTT_DEFAULT_RECV_SIZE];

    unsigned int keepAliveIntervalInSec;
    char ping_outstanding;
    int isconnected;
    int cleansession;

    struct MessageHandlers
    {
        const char* topicFilter;
        void (*fp) (MessageData*);
    } messageHandlers[MAX_MESSAGE_HANDLERS];      /* Message handlers are indexed by subscription topic */

    void (*defaultMessageHandler) (MessageData*);

    TimerInterface *timer;
    DebugInterface *debug;

    unsigned long lastSentTimeInMs;
    unsigned long lastReceivedTimeInMs;
    unsigned long timeOutInMs;

    HeraclesTcpClient heraclesTcpClient;
    TcpClientInterface *tcpLayer;

} MQTTClient;


/**
 * MQTT Init - Reset and initialize the Heracles Modem.
 * @param c - the client object to use
 * @param serial - serial object to use
 * @param timer - timer object to use
 * @param serial - debug object to use
 */
void MQTTClientInit(MQTTClient* c, SerialInterface* serial, TimerInterface* timer, DebugInterface *debug);

/** MQTT Connect - send an MQTT connect packet down the network and wait for a Connack
 *  The network object must be connected to the network endpoint before calling this
 *  @param client - the client object to use
 *  @param options - connect options
 *  @param data - connack data
 *  @param host - network address
 *  @param port - port number
 *  @param sslEnabled - Secure socket layer used or not
 *  @return success code (= 0) or negative values if failure occurs
 */
int MQTTConnectWithResults(MQTTClient* client,
		                   MQTTPacket_connectData* options,
                           MQTTConnackData* data,
						   const char *host,
						   unsigned short port,
						   unsigned int sslEnabled);

/** MQTT Connect - send an MQTT connect packet down the network and wait for a Connack
 *  The network object must be connected to the network endpoint before calling this
 *  @param client - the client object to use
 *  @param options - connect options
 *  @param host - network address
 *  @param port - port number
 *  @param sslEnabled - Secure socket layer used or not
 *  @return success code (= 0) or negative values if failure occurs
 */
int MQTTConnect(MQTTClient* client,
		        MQTTPacket_connectData* options,
				const char *host,
				unsigned short port,
		        unsigned int sslEnabled);

/** MQTT Publish - send an MQTT publish packet and wait for all acks to complete for all QoSs
 *  @param client - the client object to use
 *  @param topicName - the topic to publish to
 *  @param message - the message to send
 *  @return success code (= 0) or negative values if failure occurs
 */
int MQTTPublish(MQTTClient* client, const char* topicName, MQTTMessage* message);

/** MQTT SetMessageHandler - set or remove a per topic message handler
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter set the message handler for
 *  @param messageHandler - pointer to the message handler function or NULL to remove
 *  @return success code (= 0) or negative values if failure occurs
 */
int MQTTSetMessageHandler(MQTTClient* c, const char* topicFilter, messageHandler messageHandler);

/** MQTT Subscribe - send an MQTT subscribe packet and wait for suback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to subscribe to
 *  @param qos - Quality of service MQTT
 *  @param messageHandler - the message to send
 *  @return success code (= 0) or negative values if failure occurs
 */
int MQTTSubscribe(MQTTClient* client, const char* topicFilter, enum QoS qos, messageHandler messageHandler);

/** MQTT Subscribe - send an MQTT subscribe packet and wait for suback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to subscribe to
 *  @param qos - Quality of service MQTT
 *  @param messageHandler - the message to send
 *  @param data - suback granted QoS returned
 *  @return success code (= 0) or negative values if failure occurs
 */
int MQTTSubscribeWithResults(MQTTClient* client, const char* topicFilter, enum QoS qos, messageHandler messageHandler, MQTTSubackData* data);

/** MQTT Subscribe - send an MQTT unsubscribe packet and wait for unsuback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to unsubscribe from
 *  @return success code (= 0) or negative values if failure occurs
 */
int MQTTUnsubscribe(MQTTClient* client, const char* topicFilter);

/** MQTT Disconnect - send an MQTT disconnect packet and close the connection
 *  @param client - the client object to use
 *  @return success code (positive values) or negative values if failure occurs
 */
int MQTTDisconnect(MQTTClient* client);

/** MQTT Yield - MQTT background
 *  @param client - the client object to use
 *  @param time - the time, in milliseconds, to yield for
 *  @return success code (= 0) or negative values if failure occurs
 */
int MQTTYield(MQTTClient* client, int time);

/** MQTT isConnected
 *  @param client - the client object to use
 *  @return truth value (= 1) indicating whether the client is connected to the server
 */
int MQTTIsConnected(MQTTClient* client);

#ifdef __cplusplus
}
#endif

#endif
