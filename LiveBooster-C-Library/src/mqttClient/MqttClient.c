/*
 * Copyright (C) 2018 Orange
 *
 * This software is distributed under the terms and conditions of the GNU Lesser
 * General Public License (LGPL-3.0) which can be found in the file 'LICENSE.txt'
 * in this package distribution.
 */

#include "MqttClient.h"

#include <stdio.h>
#include <string.h>

static void NewMessageData(MessageData* md, MQTTString* aTopicName, MQTTMessage* aMessage) {
    md->topicName = aTopicName;
    md->message = aMessage;
}


static int getNextPacketId(MQTTClient *c) {
    return c->next_packetid = (c->next_packetid == MAX_PACKET_ID) ? 1 : c->next_packetid + 1;
}


static int sendPacket(MQTTClient* c, int length)
{
    int rc = ERR_MQTT_SENT_PACKET;
    int sent = 0;

    while (sent < length )
    {
        rc = c->tcpLayer->write(c->tcpLayer, &c->buf[sent], length);
        if (rc < 0)  // there was an error writing the data
            break;
        sent += rc;
    }
    if (sent == length)
    {
    	c->lastSentTimeInMs = c->timer->millis() + 1000*c->keepAliveIntervalInSec; // record the fact that we have MQTT_SUCCESSy sent the packet
        rc = MQTT_SUCCESS;
    }
    else
        rc = ERR_MQTT_SENT_PACKET;
    return rc;
}

void MQTTClientInit(MQTTClient* c, SerialInterface* serial, TimerInterface* timer, DebugInterface *debug)
{
    HeraclesTcpClient__Init(&c->heraclesTcpClient, serial, timer, debug, 1);

	int i;
    c->tcpLayer = (TcpClientInterface*)&c->heraclesTcpClient;
    c->timer = timer;
	c->debug = debug;
    c->lastSentTimeInMs = timer->millis();
    c->lastReceivedTimeInMs = c->lastSentTimeInMs;
    c->timeOutInMs = c->lastSentTimeInMs;

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        c->messageHandlers[i].topicFilter = 0;
    c->isconnected = 0;
    c->cleansession = 0;
    c->ping_outstanding = 0;
    c->defaultMessageHandler = NULL;
	c->next_packetid = 1;
}


static int decodePacket(MQTTClient* c, int* value, int timeout)
{
    unsigned char i;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        int rc = MQTTPACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
        rc = c->tcpLayer->read(c->tcpLayer, &i, 1, timeout);
        if (rc != 1)
            goto exit;
        *value += (i & 127) * multiplier;
        multiplier *= 128;
    } while ((i & 128) != 0);
exit:
    return len;
}


static int readPacket(MQTTClient* c)
{
    MQTTHeader header = {0};
    int len = 0;
    int rem_len = 0;

    /* 1. read the header byte.  This has the packet type in it */
    unsigned int remainingTime = c->timeOutInMs - c->timer->millis();
    int rc = c->tcpLayer->read(c->tcpLayer, c->readbuf, 1, remainingTime);
    if (rc != 1) {
        goto exit;
    }

    len = 1;
    /* 2. read the remaining length.  This is variable in itself */
    remainingTime = c->timeOutInMs - c->timer->millis();
    decodePacket(c, &rem_len, remainingTime);
    len += MQTTPacket_encode(c->readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

    if (rem_len > (MQTT_DEFAULT_RECV_SIZE - len))
    {
        rc = ERR_MQTT_BUFFER_OVERFLOW;
        goto exit;
    }

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    remainingTime = c->timeOutInMs - c->timer->millis();
    if (rem_len > 0 && (rc = c->tcpLayer->read(c->tcpLayer, c->readbuf + len, rem_len, remainingTime) != rem_len)) {
        rc = 0;
        goto exit;
    }

    header.byte = c->readbuf[0];
    rc = header.bits.type;
    if (c->keepAliveIntervalInSec > 0) {
        /* record the fact that we have MQTT_SUCCESSy received a packet */
    	c->lastReceivedTimeInMs = c->timer->millis() + 1000*c->keepAliveIntervalInSec;
    }
exit:
    return rc;
}


// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
static char isTopicMatched(char* topicFilter, MQTTString* topicName)
{
    char* curf = topicFilter;
    char* curn = topicName->lenstring.data;
    char* curn_end = curn + topicName->lenstring.len;

    while (*curf && curn < curn_end)
    {
        if (*curn == '/' && *curf != '/')
            break;
        if (*curf != '+' && *curf != '#' && *curf != *curn)
            break;
        if (*curf == '+')
        {   // skip until we meet the next separator, or end of string
            char* nextpos = curn + 1;
            while (nextpos < curn_end && *nextpos != '/')
                nextpos = ++curn + 1;
        }
        else if (*curf == '#')
            curn = curn_end - 1;    // skip until end of string
        curf++;
        curn++;
    };

    return (curn == curn_end) && (*curf == '\0');
}


int deliverMessage(MQTTClient* c, MQTTString* topicName, MQTTMessage* message)
{
    int i;
    int rc = FAILURE;

    // we have to find the right message handler - indexed by topic
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (c->messageHandlers[i].topicFilter != 0 && (MQTTPacket_equals(topicName, (char*)c->messageHandlers[i].topicFilter) ||
                isTopicMatched((char*)c->messageHandlers[i].topicFilter, topicName)))
        {
            if (c->messageHandlers[i].fp != NULL)
            {
                MessageData md;
                NewMessageData(&md, topicName, message);
                c->messageHandlers[i].fp(&md);
                rc = MQTT_SUCCESS;
            }
        }
    }

    if (rc == FAILURE && c->defaultMessageHandler != NULL)
    {
        MessageData md;
        NewMessageData(&md, topicName, message);
        c->defaultMessageHandler(&md);
        rc = MQTT_SUCCESS;
    }

    return rc;
}


int keepalive(MQTTClient* c)
{
    int rc = MQTT_SUCCESS;

    if (c->keepAliveIntervalInSec == 0)
        goto exit;

    if ((c->timer->millis() >= c->lastSentTimeInMs) && (c->timer->millis() >= c->lastReceivedTimeInMs))
    {
    	if (c->ping_outstanding)
            rc = FAILURE_PINGRESP_NOT_RECEIVED; /* PINGRESP not received in keepalive interval */
        else
        {
            c->timeOutInMs = c->timer->millis() + 1000;
            int len = MQTTSerialize_pingreq(c->buf, MQTT_DEFAULT_SEND_SIZE);
            if (len > 0 && (rc = sendPacket(c, len)) == MQTT_SUCCESS) // send the ping packet
                c->ping_outstanding = 1;
        }
    }

exit:
    return rc;
}


void MQTTCleanSession(MQTTClient* c)
{
    int i = 0;

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        c->messageHandlers[i].topicFilter = NULL;
}


void MQTTCloseSession(MQTTClient* c)
{
    c->ping_outstanding = 0;
    c->isconnected = 0;
    if (c->cleansession)
        MQTTCleanSession(c);
}


int cycle(MQTTClient* c)
{
    int len = 0,
        rc = MQTT_SUCCESS,
		keepAliveRes = MQTT_SUCCESS;

    int packet_type = readPacket(c);     /* read the socket, see what work is due */

    switch (packet_type)
    {
        default:
            /* no more data to read, unrecoverable. Or read packet fails due to unexpected network error */
            rc = packet_type;
            goto exit;
        case 0: /* timed out reading packet */
            break;
        case CONNACK:
        case PUBACK:
        case SUBACK:
        case UNSUBACK:
            break;
        case PUBLISH:
        {
            MQTTString topicName;
            MQTTMessage msg;
            int intQoS;
            msg.payloadlen = 0; /* this is a size_t, but de-serialize publish sets this as int */
            if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
               (unsigned char**)&msg.payload, (int*)&msg.payloadlen, c->readbuf, MQTT_DEFAULT_RECV_SIZE) != 1)
                goto exit;
            msg.qos = (enum QoS)intQoS;
            deliverMessage(c, &topicName, &msg);
            if (msg.qos != QOS0)
            {
                if (msg.qos == QOS1)
                    len = MQTTSerialize_ack(c->buf, MQTT_DEFAULT_SEND_SIZE, PUBACK, 0, msg.id);
                else if (msg.qos == QOS2)
                    len = MQTTSerialize_ack(c->buf, MQTT_DEFAULT_SEND_SIZE, PUBREC, 0, msg.id);
                if (len <= 0)
                    rc = FAILURE;
                else
                    rc = sendPacket(c, len);
                if (rc == FAILURE)
                    goto exit; // there was a problem
            }
            break;
        }
        case PUBREC:
        case PUBREL:
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, MQTT_DEFAULT_RECV_SIZE) != 1)
                rc = FAILURE;
            else if ((len = MQTTSerialize_ack(c->buf, MQTT_DEFAULT_SEND_SIZE,
                (packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid)) <= 0)
                rc = FAILURE;
            else if ((rc = sendPacket(c, len)) != MQTT_SUCCESS) // send the PUBREL packet
                rc = FAILURE; // there was a problem
            if (rc == FAILURE)
                goto exit; // there was a problem
            break;
        }

        case PUBCOMP:
            break;
        case PINGRESP:
            c->ping_outstanding = 0;
            break;
    }

    keepAliveRes = keepalive(c);
    if (keepAliveRes != MQTT_SUCCESS) {
        //check only keep-alive FAILURE status so that previous FAILURE status can be considered as FAULT
        rc = keepAliveRes;
    }

exit:
    if (rc == MQTT_SUCCESS)
        rc = packet_type;
    else if (c->isconnected)
        MQTTCloseSession(c);
    return rc;
}


int MQTTYield(MQTTClient* c, int timeout_ms)
{
    int rc = MQTT_SUCCESS;
    c->timeOutInMs = c->timer->millis() + timeout_ms;

	do
    {
        rc = cycle(c);
        if (rc < 0) {
            break;
        }
  	} while (c->timer->millis() < c->timeOutInMs);

    return rc;
}

int MQTTIsConnected(MQTTClient* client)
{
  return client->isconnected;
}

int waitfor(MQTTClient* c, int packet_type)
{
    int rc = FAILURE;

    do
    {
        if (c->timer->millis() >= c->timeOutInMs) {

            break; // we timed out
        }
        rc = cycle(c);
    }
    while (rc != packet_type && rc >= 0);

    return rc;
}


int MQTTConnectWithResults(MQTTClient* c, MQTTPacket_connectData* options, MQTTConnackData* data, const char *host, unsigned short port, unsigned int sslEnabled)
{
	int rc = ERR_MQTT_CONNECT;
    MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
    int len = 0;
	if (c->isconnected) { /* don't send connect packet again if we are already connected */
	    goto exit;
	}

	/* At first, open TCP session to server */
	len = c->tcpLayer->connect (c->tcpLayer, host, port, sslEnabled);
	if (len != 1) {
		goto exit;
	}

	c->timeOutInMs = c->timer->millis() + ACK_COMMAND_TIMEOUT_IN_MS;

    if (options == 0) {
        options = &default_options; /* set default options if none were supplied */
    }

    c->keepAliveIntervalInSec = options->keepAliveIntervalInSec;
    c->cleansession = options->cleansession;

    c->lastReceivedTimeInMs = c->timer->millis() + 1000*c->keepAliveIntervalInSec;

    if ((len = MQTTSerialize_connect(c->buf, MQTT_DEFAULT_SEND_SIZE, options)) <= 0) {
        goto exit;
    }
    if ((rc = sendPacket(c, len)) != MQTT_SUCCESS) { // send the connect packet
        goto exit; // there was a problem
    }

    // this will be a blocking call, wait for the connack
    if (waitfor(c, CONNACK) == CONNACK)
    {
        data->rc = 0;
        data->sessionPresent = 0;
        if (MQTTDeserialize_connack(&data->sessionPresent, &data->rc, c->readbuf, MQTT_DEFAULT_RECV_SIZE) == 1)
            rc = data->rc;
        else
            rc = ERR_MQTT_DESERIALIZE_CONNACT;
    }
    else
        rc = ERR_MQTT_WAIT_FOR_CONNACT;

exit:
    if (rc == MQTT_SUCCESS)
    {
        c->isconnected = 1;
        c->ping_outstanding = 0;
    }

    return rc;
}


int MQTTConnect(MQTTClient* c, MQTTPacket_connectData* options, const char *host, unsigned short port, unsigned int sslEnabled)
{
    MQTTConnackData data;
    return MQTTConnectWithResults(c, options, &data, host, port, sslEnabled);
}


int MQTTSetMessageHandler(MQTTClient* c, const char* topicFilter, messageHandler messageHandler)
{
    int rc = ERR_MQTT_SET_MESSAGE_HANDLER;
    int i = -1;

    /* first check for an existing matching slot */
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (c->messageHandlers[i].topicFilter != NULL && strcmp(c->messageHandlers[i].topicFilter, topicFilter) == 0)
        {
            if (messageHandler == NULL) /* remove existing */
            {
                c->messageHandlers[i].topicFilter = NULL;
                c->messageHandlers[i].fp = NULL;
            }
            rc = MQTT_SUCCESS; /* return i when adding new subscription */
            break;
        }
    }
    /* if no existing, look for empty slot (unless we are removing) */
    if (messageHandler != NULL) {
        if (rc == ERR_MQTT_SET_MESSAGE_HANDLER)
        {
            for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
            {
                if (c->messageHandlers[i].topicFilter == NULL)
                {
                    rc = MQTT_SUCCESS;
                    break;
                }
            }
        }
        if (i < MAX_MESSAGE_HANDLERS)
        {
            c->messageHandlers[i].topicFilter = topicFilter;
            c->messageHandlers[i].fp = messageHandler;
        }
    }
    return rc;
}


int MQTTSubscribeWithResults(MQTTClient* c, const char* topicFilter, enum QoS qos,
       messageHandler messageHandler, MQTTSubackData* data)
{
    int rc = ERR_MQTT_SUBSCRIBE;
    int len = 0;
    int qos_tab = (int)qos;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicFilter;

	if (!c->isconnected)
		    goto exit;

	c->timeOutInMs = c->timer->millis() + ACK_COMMAND_TIMEOUT_IN_MS;

    len = MQTTSerialize_subscribe(c->buf, MQTT_DEFAULT_SEND_SIZE, 0, getNextPacketId(c), 1, &topic, (int*)&qos_tab);
    if (len <= 0)
        goto exit;
    if ((rc = sendPacket(c, len)) != MQTT_SUCCESS) // send the subscribe packet
        goto exit;             // there was a problem

    if (waitfor(c, SUBACK) == SUBACK)      // wait for suback
    {
        int count = 0;
        unsigned short mypacketid;
        data->grantedQoS = QOS0;
        if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&data->grantedQoS, c->readbuf, MQTT_DEFAULT_RECV_SIZE) == 1)
        {
            if (data->grantedQoS != 0x80)
                rc = MQTTSetMessageHandler(c, topicFilter, messageHandler);
        }
    }
    else
        rc = ERR_MQTT_SUBSCRIBE;

exit:
    if (rc == FAILURE)
        MQTTCloseSession(c);
    return rc;
}


int MQTTSubscribe(MQTTClient* c, const char* topicFilter, enum QoS qos,
       messageHandler messageHandler)
{
    MQTTSubackData data;
    return MQTTSubscribeWithResults(c, topicFilter, qos, messageHandler, &data);
}


int MQTTUnsubscribe(MQTTClient* c, const char* topicFilter)
{
    int rc = ERR_MQTT_UNSUBSCRIBE;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicFilter;
    int len = 0;

	if (!c->isconnected)
		  goto exit;

    c->timeOutInMs = c->timer->millis() + ACK_COMMAND_TIMEOUT_IN_MS;

    if ((len = MQTTSerialize_unsubscribe(c->buf, MQTT_DEFAULT_SEND_SIZE, 0, getNextPacketId(c), 1, &topic)) <= 0)
        goto exit;
    if ((rc = sendPacket(c, len)) != MQTT_SUCCESS) // send the subscribe packet
        goto exit; // there was a problem

    if (waitfor(c, UNSUBACK) == UNSUBACK)
    {
        unsigned short mypacketid;  // should be the same as the packetid above
        if (MQTTDeserialize_unsuback(&mypacketid, c->readbuf, MQTT_DEFAULT_RECV_SIZE) == 1)
        {
            /* remove the subscription message handler associated with this topic, if there is one */
            MQTTSetMessageHandler(c, topicFilter, NULL);
        }
    }
    else
        rc = ERR_MQTT_UNSUBSCRIBE;

exit:
    if (rc <= FAILURE)
        MQTTCloseSession(c);
    return rc;
}


int MQTTPublish(MQTTClient* c, const char* topicName, MQTTMessage* message)
{
    int rc = ERR_MQTT_PUBLISH;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicName;
    int len = 0;

	if (!c->isconnected)
		    goto exit;

    c->timeOutInMs = c->timer->millis() + ACK_COMMAND_TIMEOUT_IN_MS;

    if (message->qos == QOS1 || message->qos == QOS2)
        message->id = getNextPacketId(c);

    len = MQTTSerialize_publish(c->buf, MQTT_DEFAULT_SEND_SIZE, 0, message->qos, message->retained, message->id,
              topic, (unsigned char*)message->payload, message->payloadlen);
    if (len <= 0)
        goto exit;
    if ((rc = sendPacket(c, len)) != MQTT_SUCCESS) // send the subscribe packet
        goto exit; // there was a problem

    if (message->qos == QOS1)
    {
        if (waitfor(c, PUBACK) == PUBACK)
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, MQTT_DEFAULT_RECV_SIZE) != 1)
                rc = ERR_MQTT_DESERIALIZE_ACK;
        }
        else
            rc = ERR_MQTT_WAIT_FOR_PUBACT;
    }
    else if (message->qos == QOS2)
    {
        if (waitfor(c, PUBCOMP) == PUBCOMP)
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, MQTT_DEFAULT_RECV_SIZE) != 1)
                rc = ERR_MQTT_DESERIALIZE_ACK;
        }
        else
            rc = ERR_MQTT_WAIT_FOR_PUBCOMP;
    }

exit:
    if (rc <= FAILURE)
        MQTTCloseSession(c);
    return rc;
}


int MQTTDisconnect(MQTTClient* c)
{
    int rc = ERR_MQTT_DISCONNECT;
    int len = 0;

    c->timeOutInMs = c->timer->millis() + ACK_COMMAND_TIMEOUT_IN_MS; // we might wait for incomplete incoming publishes to complete

    len = MQTTSerialize_disconnect(c->buf, MQTT_DEFAULT_SEND_SIZE);
    if (len > 0)
        rc = sendPacket(c, len); // send the disconnect packet
    MQTTCloseSession(c);

    /* At end, close TCP session */
    c->tcpLayer->stop(c->tcpLayer);

    return rc;
}
