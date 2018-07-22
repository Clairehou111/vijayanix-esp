/*******************************************************************************
 * Copyright (c) 2014, 2017 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *   Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
 *   Ian Craggs - fix for #96 - check rem_len in readPacket
 *   Ian Craggs - add ability to set message handler separately #6
 *******************************************************************************/
#include "MQTTClient.h"

#include <stdio.h>
#include <string.h>
#include <string.h>
xTaskHandle MQTTTaskHandle;

static void NewMessageData(MessageData* md, MQTTString* aTopicName,
		MQTTMessage* aMessage) {
	md->topicName = aTopicName;
	md->message = aMessage;
}

static int getNextPacketId(MQTTClient *c) {
	return c->next_packetid =
			(c->next_packetid == MAX_PACKET_ID) ? 1 : c->next_packetid + 1;
}

static int sendPacket(MQTTClient* c, int length, Timer* timer) {
	int rc = FAILURE, sent = 0;

	if (TimerIsExpired(timer)) {
		ESP_DBG("TimerIsExpired\n");
	}

	while (sent < length && !TimerIsExpired(timer)) {
		/*ESP_DBG("sendPacket, c is null = %d\n",  c == NULL);
		ESP_DBG("sendPacket, c->ipstack is null = %d \n",  c->ipstack == NULL);*/
		ESP_DBG("sendPacket, c->ipstack->mqttwrite is null = %d\n",  c->ipstack->mqttwrite == NULL);

		rc = c->ipstack->mqttwrite(c->ipstack, &c->buf[sent], length,
				TimerLeftMS(timer));
		if (rc < 0)  // there was an error writing the data
			break;
		sent += rc;
	}

	if (sent == length) {
		TimerCountdown(&c->last_sent, c->keepAliveInterval); // record the fact that we have successfully sent the packet
		rc = SUCCESS;

		ESP_DBG("%d bytes have been sent out\n",length);
	} else
		rc = FAILURE;
	return rc;
}

/*static int sendPacket2(MQTTClient* c, int length, Timer* timer) {
	int rc = FAILURE, sent = 0;

	if (TimerIsExpired(timer)) {
		ESP_DBG("TimerIsExpired\n");
	}

	ESP_DBG("before while\n");

	while (sent < length && !TimerIsExpired(timer)) {
		ESP_DBG(" c->ipstack address = %X\n",  c->ipstack);
		ESP_DBG(" c->ipstack->mqttwrite address = %X\n",  c->ipstack->mqttwrite);
		ESP_DBG(" c->buf address = %X\n",  c->buf);
		ESP_DBG(" &c->buf[sent] address = %X\n",  &c->buf[sent]);

		ESP_DBG("before get values, network\n");
		Network *network = c->ipstack;

		ESP_DBG("network get values, network, %d\n",c->ipstack->my_socket);

		unsigned char *cha =  &c->buf[sent];
		ESP_DBG("cha get values: %s\n",cha);

		unsigned int time = TimerLeftMS(timer);
		ESP_DBG("time get values,%d\n",time);

		ESP_DBG("before mqttwrite\n");
		rc = c->ipstack->mqttwrite(network, "aa", 2,
				2);
		rc = c->ipstack->mqttwrite(network, cha, length,
				time);
		ESP_DBG("after mqttwrite\n");
		if (rc < 0)  // there was an error writing the data
			break;
		sent += rc;
	}

	ESP_DBG("after while\n");
	if (sent == length) {
		TimerCountdown(&c->last_sent, c->keepAliveInterval); // record the fact that we have successfully sent the packet
		rc = SUCCESS;

		ESP_DBG("%d bytes have been sent out\n",length);
	} else
		rc = FAILURE;
	return rc;
}*/

void MQTTClientInit(MQTTClient* c, Network* network,
		unsigned int command_timeout_ms, unsigned char* sendbuf,
		size_t sendbuf_size, unsigned char* readbuf, size_t readbuf_size) {
	int i;
	c->ipstack = network;

	for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
		c->messageHandlers[i].topicFilter = 0;
	c->command_timeout_ms = command_timeout_ms;
	c->buf = sendbuf;
	c->buf_size = sendbuf_size;
	c->readbuf = readbuf;
	c->readbuf_size = readbuf_size;
	c->isconnected = 0;
	c->cleansession = 0;
	c->ping_outstanding = 0;
	c->defaultMessageHandler = NULL;
	c->next_packetid = 1;

	TimerInit(&c->last_sent);
	TimerInit(&c->last_received);
#if defined(MQTT_TASK)
	MutexInit(&c->mutex);

#endif
}

static int decodePacket(MQTTClient* c, int* value, int timeout) {
	unsigned char i;
	int multiplier = 1;
	int len = 0;
	const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

	*value = 0;
	int value1 = 0;

	do {
		int rc = MQTTPACKET_READ_ERROR;

		if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES) {
			rc = MQTTPACKET_READ_ERROR; /* bad data */
			goto exit;
		}
		rc = c->ipstack->mqttread(c->ipstack, &i, 1, timeout);
		if (rc != 1)
			goto exit;

		value1 += (i & 127) * multiplier;
		multiplier *= 128;
	} while ((i & 128) != 0);
	exit:

	*value = value1;
	return len;
}

static int readPacket(MQTTClient* c, Timer* timer) {
	MQTTHeader header = { 0 };
	int len = 0;
	int rem_len = 0;

	/* 1. read the header byte.  This has the packet type in it */
	int rc = c->ipstack->mqttread(c->ipstack, c->readbuf, 1,
			TimerLeftMS(timer));

	if (rc != 1)
		goto exit;

	len = 1;
	/* 2. read the remaining length.  This is variable in itself */
	decodePacket(c, &rem_len, TimerLeftMS(timer));
	len += MQTTPacket_encode(c->readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

	if (rem_len > (c->readbuf_size - len)) {
		rc = BUFFER_OVERFLOW;
		goto exit;
	}

	/* 3. read the rest of the buffer using a callback to supply the rest of the data */
	if (rem_len > 0
			&& (rc = c->ipstack->mqttread(c->ipstack, c->readbuf + len, rem_len,
					TimerLeftMS(timer)) != rem_len)) {
		rc = 0;
		goto exit;
	}

	header.byte = c->readbuf[0];
	rc = header.bits.type;
	ESP_DBG("message type is %d \n", rc);
	if (c->keepAliveInterval > 0)
		TimerCountdown(&c->last_received, c->keepAliveInterval); // record the fact that we have successfully received a packet
	exit: return rc;
}

// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
static char isTopicMatched(char* topicFilter, MQTTString* topicName) {
	char* curf = topicFilter;
	char* curn = topicName->lenstring.data;
	char* curn_end = curn + topicName->lenstring.len;

	while (*curf && curn < curn_end) {
		if (*curn == '/' && *curf != '/')
			break;
		if (*curf != '+' && *curf != '#' && *curf != *curn)
			break;
		if (*curf == '+') { // skip until we meet the next separator, or end of string
			char* nextpos = curn + 1;
			while (nextpos < curn_end && *nextpos != '/')
				nextpos = ++curn + 1;
		} else if (*curf == '#')
			curn = curn_end - 1;    // skip until end of string
		curf++;
		curn++;
	};

	return (curn == curn_end) && (*curf == '\0');
}

int deliverMessage(MQTTClient* c, MQTTString* topicName, MQTTMessage* message) {
	int i;
	int rc = FAILURE;

	// we have to find the right message handler - indexed by topic
	for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i) {
		if (c->messageHandlers[i].topicFilter != 0
				&& (MQTTPacket_equals(topicName,
						(char*) c->messageHandlers[i].topicFilter)
						|| isTopicMatched(
								(char*) c->messageHandlers[i].topicFilter,
								topicName))) {
			if (c->messageHandlers[i].fp != NULL) {
				MessageData md;
				NewMessageData(&md, topicName, message);
				c->messageHandlers[i].fp(&md);
				rc = SUCCESS;
			}
		}



	}

	if (rc == FAILURE && c->defaultMessageHandler != NULL) {
		MessageData md;
		NewMessageData(&md, topicName, message);
		c->defaultMessageHandler(&md);
		rc = SUCCESS;
	}

	return rc;
}

int keepalive(MQTTClient* c) {
	int rc = SUCCESS;

	if (c->keepAliveInterval == 0)
		goto exit;

	if (TimerIsExpired(&c->last_sent) || TimerIsExpired(&c->last_received)) {
		if (c->ping_outstanding)
			rc = FAILURE; /* PINGRESP not received in keepalive interval */
		else {
			Timer timer;
			TimerInit(&timer);
			TimerCountdownMS(&timer, 1000);
			int len = MQTTSerialize_pingreq(c->buf, c->buf_size);
			if (len > 0 && (rc = sendPacket(c, len, &timer)) == SUCCESS) // send the ping packet
				c->ping_outstanding = 1;
		}
	}

	exit: return rc;
}

void MQTTCleanSession(MQTTClient* c) {
	int i = 0;

	for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
		c->messageHandlers[i].topicFilter = NULL;
}

void MQTTCloseSession(MQTTClient* c) {
	c->ping_outstanding = 0;
	c->isconnected = 0;
	if (c->cleansession)
		MQTTCleanSession(c);
}

int cycle(int count, MQTTClient* c, Timer* timer) {
	if (count) {
		ESP_DBG("the %d time coming to cycle---------------------\n", count);

	}
	int len = 0, rc = SUCCESS;

	int packet_type = readPacket(c, timer); /* read the socket, see what work is due */

	switch (packet_type) {
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
		goto exit;
	case PUBLISH: {
		MQTTString topicName;
		MQTTMessage msg;
		int intQoS;
		msg.payloadlen = 0; /* this is a size_t, but deserialize publish sets this as int */
		if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id,
				&topicName, (unsigned char**) &msg.payload,
				(int*) &msg.payloadlen, c->readbuf, c->readbuf_size) != 1)
			goto exit;
		msg.qos = (enum QoS) intQoS;

		deliverMessage(c, &topicName, &msg);
		if (msg.qos != QOS0) {
			if (msg.qos == QOS1)
				len = MQTTSerialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id);
			else if (msg.qos == QOS2)
				len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id);
			if (len <= 0)
				rc = FAILURE;
			else
				rc = sendPacket(c, len, timer);
			if (rc == FAILURE)
				goto exit;
			// there was a problem
		}
		break;
	}
	case PUBREC:
	case PUBREL: {
		unsigned short mypacketid;
		unsigned char dup, type;
		if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf,
				c->readbuf_size) != 1)
			rc = FAILURE;
		else if ((len = MQTTSerialize_ack(c->buf, c->buf_size,
				(packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid))
				<= 0)
			rc = FAILURE;
		else if ((rc = sendPacket(c, len, timer)) != SUCCESS) // send the PUBREL packet
			rc = FAILURE; // there was a problem
		if (rc == FAILURE)
			goto exit;
		// there was a problem
		break;
	}

	case PUBCOMP:
		break;
	case PINGRESP:
		c->ping_outstanding = 0;
		break;
	}

	if (keepalive(c) != SUCCESS) {
		//check only keepalive FAILURE status so that previous FAILURE status can be considered as FAULT
		rc = FAILURE;
	}

	exit: if (rc == SUCCESS)
		rc = packet_type;
	else if (c->isconnected){
		ESP_DBG("in cycle, before MQTTCloseSession\n");
		MQTTCloseSession(c);
	}

	return rc;
}

int MQTTYield(MQTTClient* c, int timeout_ms) {
	int rc = SUCCESS;
	Timer timer;

	TimerInit(&timer);
	TimerCountdownMS(&timer, timeout_ms);

	do {
		if (cycle(0, c, &timer) < 0) {
			rc = FAILURE;
			break;
		}
	} while (!TimerIsExpired(&timer));

	return rc;
}

void MQTTRun(void* parm) {
	printf("MQTTRun\n");
	Timer timer;
	MQTTClient* c = (MQTTClient*) parm;

	TimerInit(&timer);

	int count = 0;

	while (1) {

	    ESP_DBG("MQTTRun is runnning \n");

		if (MQTTNetworkIsDestoryed(c)) {
			//break;
			ESP_DBG("MQTTRun will Suspend\n");
			vTaskSuspend(NULL);
		}

		//ESP_DBG("MQTTRun is running ,pass the check\n");

#if defined(MQTT_TASK)
		ESP_DBG("MQTTRun before get lock\n");
		MutexLock(&c->mutex);
		ESP_DBG("MQTTRun after get lock\n");
#endif
		TimerCountdownMS(&timer, 500); /* Don't wait too long if no traffic is incoming */
		count++;
		cycle(count, c, &timer);
#if defined(MQTT_TASK)
		ESP_DBG("MQTTRun before release lock\n");
		MutexUnlock(&c->mutex);
		ESP_DBG("MQTTRun after release lock\n");
#endif
		vTaskDelay(1000 / portTICK_RATE_MS);
	}

	ESP_DBG("MQTTRun is deleted\n");
	vTaskDelete(NULL);

}



int MQTTListenerStart(xTaskHandle *taskHandle, void (*fn)(void*), void* arg)
{
    int rc = 0;
    uint16_t usTaskStackSize = (configMINIMAL_STACK_SIZE * 5);
    unsigned portBASE_TYPE uxTaskPriority = uxTaskPriorityGet(NULL); /* set the priority as the same as the calling task*/

    ESP_DBG("MQTTRun, the priority is %d \n",uxTaskPriority);

   // printf("this is before xTaskCreate\n");
    rc = xTaskCreate(fn,    /* The function that implements the task. */
                     "MQTTTask",         /* Just a text name for the task to aid debugging. */
                     usTaskStackSize,    /* The stack size is defined in FreeRTOSIPConfig.h. */
                     arg,                /* The task parameter, not used in this case. */
                     uxTaskPriority,     /* The priority assigned to the task is defined in FreeRTOSConfig.h. */
                     taskHandle);     /* The task handle is not used. */
    ESP_DBG("MQTTRun is created\n");
    return rc;
}


#if defined(MQTT_TASK)
int MQTTStartTask(MQTTClient* client) {

	if(MQTTTaskHandle!= NULL){
		ESP_DBG("MQTTRun hanlde is not null\n");
		vTaskResume(MQTTTaskHandle);
		ESP_DBG("MQTTRun is resumed\n");
		return pdPASS;
	}
	return MQTTListenerStart(&MQTTTaskHandle, &MQTTRun, client);
}
#endif

int waitfor(MQTTClient* c, int packet_type, Timer* timer) {
	int rc = FAILURE;

	ESP_DBG("waitfor starts\n");

	do {
		if (TimerIsExpired(timer))
			break; // we timed out
		rc = cycle(0, c, timer);
	} while (rc != packet_type && rc >= 0);

	return rc;
}

/*int MQTTConnectWithResults(MQTTClient* c, MQTTPacket_connectData* options, MQTTConnackData* data)
 {
 Timer connect_timer;
 int rc = FAILURE;
 MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
 int len = 0;

 #if defined(MQTT_TASK)
 MutexLock(&c->mutex);
 #endif
 if (c->isconnected)  don't send connect packet again if we are already connected
 goto exit;

 TimerInit(&connect_timer);
 TimerCountdownMS(&connect_timer, c->command_timeout_ms);

 if (options == 0)
 options = &default_options;  set default options if none were supplied

 c->keepAliveInterval = options->keepAliveInterval;
 c->cleansession = options->cleansession;
 TimerCountdown(&c->last_received, c->keepAliveInterval);
 if ((len = MQTTSerialize_connect(c->buf, c->buf_size, options)) <= 0)
 ESP_DBG("MQTTConnectWithResults,len=%d\n",len);
 goto exit;
 if ((rc = sendPacket(c, len, &connect_timer)) != SUCCESS)  // send the connect packet
 ESP_DBG("MQTTConnectWithResults,rc=%d\n",rc);
 goto exit; // there was a problem

 // this will be a blocking call, wait for the connack
 if (waitfor(c, CONNACK, &connect_timer) == CONNACK)
 {
 data->rc = 0;
 data->sessionPresent = 0;
 if (MQTTDeserialize_connack(&data->sessionPresent, &data->rc, c->readbuf, c->readbuf_size) == 1)
 rc = data->rc;
 else
 rc = FAILURE;
 }
 else{
 rc = FAILURE;
 ESP_DBG("MQTTConnectWithResults,didnt get connack in timeout\n");
 }


 exit:
 if (rc == SUCCESS)
 {
 c->isconnected = 1;
 c->ping_outstanding = 0;
 }

 #if defined(MQTT_TASK)
 MutexUnlock(&c->mutex);
 #endif

 return rc;
 }*/
int MQTTConnectWithResults(MQTTClient* c, MQTTPacket_connectData* options,
		MQTTConnackData* data) {
	Timer connect_timer;
	int rc = FAILURE;
	MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
	int len = 0;

#if defined(MQTT_TASK)
	ESP_DBG("MQTTConnectWithResults before get lock\n");
	MutexLock(&c->mutex);
	ESP_DBG("MQTTConnectWithResults after get lock\n");
#endif

	if (c->isconnected) /* don't send connect packet again if we are already connected */
		goto exit;

	TimerInit(&connect_timer);
	TimerCountdownMS(&connect_timer, c->command_timeout_ms);

	if (options == 0)
		options = &default_options; /* set default options if none were supplied */

	c->keepAliveInterval = options->keepAliveInterval;
	c->cleansession = options->cleansession;
	TimerCountdown(&c->last_received, c->keepAliveInterval);

	len = MQTTSerialize_connect(c->buf, c->buf_size, options);
	if (len <= 0)
		goto exit;
	rc = sendPacket(c, len, &connect_timer);

	 ESP_DBG("sendPacket,the result is %d \n",rc);
	if (rc != SUCCESS)  // send the connect packet
		goto exit;
	// there was a problem

	// this will be a blocking call, wait for the connack
	if (waitfor(c, CONNACK, &connect_timer) == CONNACK) {
		data->rc = 0;
		data->sessionPresent = 0;
		if (MQTTDeserialize_connack(&data->sessionPresent, &data->rc,
				c->readbuf, c->readbuf_size) == 1) {
			rc = data->rc;
		}

		else {
			rc = FAILURE;
		}

	} else {
		rc = FAILURE;
	}

	exit: if (rc == SUCCESS) {
		c->isconnected = 1;
		c->ping_outstanding = 0;
	}

#if defined(MQTT_TASK)
	ESP_DBG("MQTTConnectWithResults before release lock\n");
	MutexUnlock(&c->mutex);
	ESP_DBG("MQTTConnectWithResults after release lock\n");
#endif

	return rc;
}

int MQTTConnect(MQTTClient* c, MQTTPacket_connectData* options) {
	MQTTConnackData data;
	return MQTTConnectWithResults(c, options, &data);
}

int MQTTSetMessageHandler(MQTTClient* c, const char* topicFilter,
		messageHandler messageHandler) {
	int rc = FAILURE;
	int i = -1;

	/* first check for an existing matching slot */
	for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i) {
		if (c->messageHandlers[i].topicFilter != NULL
				&& strcmp(c->messageHandlers[i].topicFilter, topicFilter)
						== 0) {
			if (messageHandler == NULL) /* remove existing */
			{
				c->messageHandlers[i].topicFilter = NULL;
				c->messageHandlers[i].fp = NULL;
			}
			rc = SUCCESS; /* return i when adding new subscription */
			break;
		}
	}
	/* if no existing, look for empty slot (unless we are removing) */
	if (messageHandler != NULL) {
		if (rc == FAILURE) {
			for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i) {
				if (c->messageHandlers[i].topicFilter == NULL) {
					rc = SUCCESS;
					break;
				}
			}
		}
		if (i < MAX_MESSAGE_HANDLERS) {
			c->messageHandlers[i].topicFilter = topicFilter;
			c->messageHandlers[i].fp = messageHandler;
		}
	}
	return rc;
}

int MQTTSubscribeWithResults(MQTTClient* c, const char* topicFilter,
		enum QoS qos, messageHandler messageHandler, MQTTSubackData* data) {
	int rc = FAILURE;
	Timer timer;
	int len = 0;
	MQTTString topic = MQTTString_initializer;
	topic.cstring = (char *) topicFilter;

#if defined(MQTT_TASK)
	ESP_DBG("MQTTSubscribeWithResults before get lock\n");
	MutexLock(&c->mutex);
	ESP_DBG("MQTTSubscribeWithResults after get lock\n");
#endif
	if (!c->isconnected){
		ESP_DBG("MQTTSubscribeWithResults, c->isconnected= %d, goto exit\n",c->isconnected);
		goto exit;
	}


	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);

	len = MQTTSerialize_subscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1,
			&topic, (int*) &qos);
	if (len <= 0){
		ESP_DBG("MQTTSubscribeWithResults, len= %d, goto exit\n",len);
			goto exit;
	}

	if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
	{
		ESP_DBG("MQTTSubscribeWithResults, rc= %d, goto exit\n",rc);
			goto exit;
	}
	// there was a problem

	ESP_DBG("MQTTSubscribeWithResults, before waitfor\n");
	if (waitfor(c, SUBACK, &timer) == SUBACK)      // wait for suback
			{
		int count = 0;
		unsigned short mypacketid;
		data->grantedQoS = QOS0;
		if (MQTTDeserialize_suback(&mypacketid, 1, &count,
				(int*) &data->grantedQoS, c->readbuf, c->readbuf_size) == 1) {
			if (data->grantedQoS != 0x80)
				rc = MQTTSetMessageHandler(c, topicFilter, messageHandler);
		}
	} else{
		rc = FAILURE;
	}


	exit: if (rc == FAILURE){
		ESP_DBG("in MQTTSubscribeWithResults, before MQTTCloseSession\n");
		MQTTCloseSession(c);
	  }
#if defined(MQTT_TASK)
	ESP_DBG("MQTTSubscribeWithResults before release lock\n");
	MutexUnlock(&c->mutex);
	ESP_DBG("MQTTSubscribeWithResults after release lock\n");
#endif
	return rc;
}

int MQTTSubscribe(MQTTClient* c, const char* topicFilter, enum QoS qos,
		messageHandler messageHandler) {
	MQTTSubackData data;
	return MQTTSubscribeWithResults(c, topicFilter, qos, messageHandler, &data);
}

int MQTTUnsubscribe(MQTTClient* c, const char* topicFilter) {
	int rc = FAILURE;
	Timer timer;
	MQTTString topic = MQTTString_initializer;
	topic.cstring = (char *) topicFilter;
	int len = 0;

#if defined(MQTT_TASK)
	ESP_DBG("MQTTUnsubscribe before get lock\n");
	MutexLock(&c->mutex);
	ESP_DBG("MQTTUnsubscribe after get lock\n");
#endif
	if (!c->isconnected)
		goto exit;

	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);

	if ((len = MQTTSerialize_unsubscribe(c->buf, c->buf_size, 0,
			getNextPacketId(c), 1, &topic)) <= 0)
		goto exit;
	if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
		goto exit;
	// there was a problem

	if (waitfor(c, UNSUBACK, &timer) == UNSUBACK) {
		unsigned short mypacketid;  // should be the same as the packetid above
		if (MQTTDeserialize_unsuback(&mypacketid, c->readbuf, c->readbuf_size)
				== 1) {
			/* remove the subscription message handler associated with this topic, if there is one */
			MQTTSetMessageHandler(c, topicFilter, NULL);
		}
	} else
		rc = FAILURE;

	exit: if (rc == FAILURE){
		ESP_DBG("in MQTTUnsubscribe, before MQTTCloseSession\n");
				MQTTCloseSession(c);
	}

#if defined(MQTT_TASK)
	ESP_DBG("MQTTUnsubscribe before release lock\n");
	MutexUnlock(&c->mutex);
	ESP_DBG("MQTTUnsubscribe after release lock\n");
#endif
	return rc;
}

int MQTTPublish(MQTTClient* c, const char* topicName, MQTTMessage* message) {
	int rc = FAILURE;
	Timer timer;
	MQTTString topic = MQTTString_initializer;
	topic.cstring = (char *) topicName;
	int len = 0;



#if defined(MQTT_TASK)
	ESP_DBG("MQTTPublish before get lock\n");
	MutexLock(&c->mutex);
	ESP_DBG("MQTTPublish after get lock\n");
#endif



	if (!c->isconnected)
		goto exit;


	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);

	if (message->qos == QOS1 || message->qos == QOS2)
		message->id = getNextPacketId(c);

	len = MQTTSerialize_publish(c->buf, c->buf_size, 0, message->qos,
			message->retained, message->id, topic,
			(unsigned char*) message->payload, message->payloadlen);
	if (len <= 0)
		goto exit;
	if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
		goto exit;
	// there was a problem

	if (message->qos == QOS1) {
		if (waitfor(c, PUBACK, &timer) == PUBACK) {
			unsigned short mypacketid;
			unsigned char dup, type;
			if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf,
					c->readbuf_size) != 1)
				rc = FAILURE;
		} else
			rc = FAILURE;
	} else if (message->qos == QOS2) {
		if (waitfor(c, PUBCOMP, &timer) == PUBCOMP) {
			unsigned short mypacketid;
			unsigned char dup, type;
			if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf,
					c->readbuf_size) != 1)
				rc = FAILURE;
		} else
			rc = FAILURE;
	}

	exit: if (rc == FAILURE){
		ESP_DBG("in MQTTPublish, before MQTTCloseSession\n");
			MQTTCloseSession(c);
	}

#if defined(MQTT_TASK)
	ESP_DBG("MQTTPublish before release lock\n");

	MutexUnlock(&c->mutex);

	ESP_DBG("MQTTPublish after release lock\n");

#endif
	return rc;
}

int MQTTDisconnect(MQTTClient* c) {
	int rc = FAILURE;
	Timer timer;  // we might wait for incomplete incoming publishes to complete
	int len = 0;

#if defined(MQTT_TASK)
	ESP_DBG("MQTTDisconnect before get lock\n");
	MutexLock(&c->mutex);
	ESP_DBG("MQTTDisconnect after get lock\n");
#endif
	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);

	len = MQTTSerialize_disconnect(c->buf, c->buf_size);
	if (len > 0)
		rc = sendPacket(c, len, &timer);           // send the disconnect packet
	ESP_DBG("in MQTTDisconnect, before MQTTCloseSession\n");
	MQTTCloseSession(c);


#if defined(MQTT_TASK)
	ESP_DBG("MQTTDisconnect before release lock\n");
	MutexUnlock(&c->mutex);
	ESP_DBG("MQTTDisconnect after release lock\n");
#endif
	return rc;
}

int MQTTNetworkIsDestoryed(MQTTClient* c) {

	ESP_DBG("c is null = %d\n",c ==NULL);
	ESP_DBG("ipstack is null =  %d\n",c->ipstack == NULL);
	ESP_DBG("connecte = %d\n",c->isconnected);
	int ret = 1;
	if (c != NULL) {
		MutexLock(&c->mutex);
		ret = c->ipstack == NULL || !MQTTIsConnected(c);
		MutexUnlock(&c->mutex);
	}

	return ret;

//	return c == NULL || c->ipstack == NULL || c->ipstack->network_status != TCP_CONNECTED;
}
