/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include <stddef.h>

#include "mqtt/MQTTClient.h"

#include "user_config.h"
#include "esp_common.h"
#include "json/cJSON.h"
#include "user_config.h"
#include "user_cgi.h"
#include "user_mqtt.h"

#include "../include/user_datastream.h"
#include "user_device_data_broadcast.h"



LOCAL xTaskHandle mqttc_client_handle;

MQTTClient* client;

#define MQTT_CLIENT_THREAD_NAME         "mqtt_client_thread"
#define MQTT_CLIENT_THREAD_STACK_WORDS  2048
#define MQTT_CLIENT_THREAD_PRIO         8
#define MQTT_DATASTREAMS_STACK_WORDS  400
#define RETRY_TIMES 2
xQueueHandle DatastreamsQueue,receivedMessageQueue;
portBASE_TYPE receivedMessageTask, mqttPublishTask;

LOCAL xTaskHandle xReceivedMessageHandle, xTaskMqttPublishHandle;




LOCAL void publish_systerm_datastreams(MQTTClient* client, char *t_json) {

	int payload_len;
	char *t_payload;
	unsigned short json_len;
	json_len = strlen(t_json) / sizeof(char);

	printf("%hu\n", json_len);

	payload_len = 1 + 2 + json_len; //payload+1字节格式说明+2字节长度说明

	t_payload = (char *) zalloc(payload_len);
	if (t_payload == NULL) {
		printf("t_payload malloc error\r\n");
		return;
	}

	//type
	t_payload[0] = '\x01';  //1： json格式1字符串

	//length
	t_payload[1] = (json_len & 0xFF00) >> 8; //固定两字节长度高位字节
	t_payload[2] = json_len & 0xFF; //固定两字节长度低位字节

	printf("%x,%x\n", t_payload[1], t_payload[2]);

	strcpy(t_payload + 3, t_json);

	printf("MQTT: report datapoints:\n");
	printf("%s", t_payload + 3);
	printf("\n");

	printf("payload_len=%d\n", payload_len);

	MQTTMessage message;
	message.qos = QOS0;
	message.retained = 0;
	message.payload = t_payload;
	message.payloadlen = payload_len;

//		const char *topicName = "$dp";
	int rc = 0, count = 0;

	if ((rc = MQTTPublish(client, ONENET_REPORT_DATA_TOPIC, &message)) != 0) {
		printf("MQTT publish topic %s,Return code from MQTT publish is %d\n",
		ONENET_REPORT_DATA_TOPIC, rc);
	} else {
		printf("MQTT publish topic %s, message number is %d\n",
		ONENET_REPORT_DATA_TOPIC, count++);
	}

	free(t_payload);
}

LOCAL char* get_mqtt_client_id(void) {
	return MQTT_CLIENT_ID;
}



char* get_app_topic(void) {

	char *topic = (char *) zalloc(30);
	sprintf(topic, "%s%s", APP_TOPIC, get_mqtt_client_id());

	return topic;
}

LOCAL char* get_app_topic_wildcards(void) {

	char *topic = (char *) zalloc(30);
	sprintf(topic, "%s%s%s%s", APP_TOPIC, get_mqtt_client_id() ,"/",TOPIC_WILDCARDS);

	return topic;
}

LOCAL char* get_app_sub_topic(int subTopic) {

	if(SUB_TOPIC_ALL == subTopic){
			return get_app_topic();
	}

	char *topic = (char *) zalloc(30);
	sprintf(topic, "%s%s%s%d", APP_TOPIC, get_mqtt_client_id(), "/",subTopic);

	return topic;
}


char* get_device_topic(void){
	char *topic = (char *) zalloc(30);
    sprintf(topic, "%s%s", DEVICE_TOPIC, get_mqtt_client_id());
	return topic;
}



char* get_device_sub_topic(int subTopic) {

	if(SUB_TOPIC_ALL == subTopic){
		return get_device_topic();
	}
	char *topic = (char *) zalloc(30);
	sprintf(topic, "%s%s%s%d", DEVICE_TOPIC, get_mqtt_client_id(), "/",subTopic);
	return topic;
}


/**
 * publish its datapoints
 */
LOCAL int publish_device_topic(MQTTClient* client,char *topic, char *t_payload) {

	printf("MQTT publish topic topic is %s, payload is: \n%s\n",topic,t_payload);

	int payload_len;
	payload_len = strlen(t_payload) / sizeof(char);
	MQTTMessage message;
	message.qos = QOS0;
	message.retained = 0;
	message.payload = t_payload;
	message.payloadlen = payload_len;

	int rc = 0, count = 0;



	if ((rc = MQTTPublish(client, topic, &message)) != 0) {
		printf("failed to MQTT publish topic %s,Return code from MQTT publish is %d\n", topic, rc);
	} else {
		printf("Succeed to MQTT publish topic  %s\n",topic);
	}


	return rc;
}

/**
 * send a command to datastreams queue
 * xTicksToWait better be zero here, otherwise the caller funtion runs too long will cause software watchdog reset.
 */
signed portBASE_TYPE send_datastreams_to_queue(void *topic, void* payload) {
	if (DatastreamsQueue == NULL) {
		return -1;
	}

	MqttMessage *message = zalloc(sizeof(MqttMessage));
	message->topic = (char *) topic;
	message->payload = (char *) payload;

	ESP_DBG("send_datastreams_to_queue, topic is %s, payload is \n%s \n",topic,payload)
	;

	return xQueueSend(DatastreamsQueue, &message, 100/portTICK_RATE_MS);

}

LOCAL void publish_datastreams_Task(void *pvParameters) {

	printf("publish_datastreams_Task starts\n");

	MQTTClient *c = pvParameters;

	//char *t_json = zalloc(sizeof(char) * MESSAGE_SIZE);

	MqttMessage *message;

	int count = 0;

	/*portTickType xLastWakeTime;
	 xLastWakeTime = xTaskGetTickCount();*/
	for (;;) {

		if (MQTTNetworkIsDestoryed(c)) {
			//break;
			ESP_DBG("publish_datastreams_Task will Suspend\n")
			;
			vTaskSuspend(NULL);
		}


		printf("publish_datastreams_Task %d words, heap %d bytes\n", (int) uxTaskGetStackHighWaterMark(NULL), system_get_free_heap_size());

		/* 从队列中获取内容 */
		if ( xQueueReceive(DatastreamsQueue, &message,
				100/portTICK_RATE_MS) == pdPASS) {
			ESP_DBG(
					"publish_datastreams_Task: the %d time recevied from DatastreamsQueue:\n%s\n",
					count, message->payload);
			ESP_DBG("publish_datastreams_Task, topic is %s, payload is \n%s \n",message->topic, message->payload);
			publish_device_topic(pvParameters,message->topic, message->payload);

		}

		//vTaskDelayUntil(&xLastWakeTime, 5*1000 / portTICK_RATE_MS);
		vTaskDelay(5 * 1000 / portTICK_RATE_MS);
	}

	if (DatastreamsQueue != NULL) {
		DatastreamsQueue = NULL;
	}

	//free(t_json);
	ESP_DBG("publish_datastreams_Task is deleted\n")
	;
	vTaskDelete(NULL);

}



LOCAL int publish_topic_now(MQTTClient *client, char *topic, char *json) {
	int ret = 0;
	int RETYR_TIMES = 2;
	int retry = 0;

	do {
		ret = publish_device_topic(client, topic, json);

		retry++;
	} while (ret != 0 && retry < RETYR_TIMES);

	return ret;
}

LOCAL int process_get(char* topic) {

	int subTopic = get_subtopic(topic);
	char *publishTopic = get_device_sub_topic(subTopic);

	char *message = get_datapoints(topic);

	int ret = publish_topic_now(client, publishTopic, message);

	free(message);
	free(publishTopic);

	return ret;

}

LOCAL int process_post(char *datapoints, char *topic) {

	int ret = set_datapoints(datapoints, topic);

	if (ret != 0) {
		printf("command executed failed.\n%s\n", datapoints);
		return -1;
	}

	ESP_DBG("process_post, before get_datapoints\n");
	char *json = get_datapoints(topic);

	ESP_DBG("process_post, before get_device_topic\n");
	char *topicPublish = get_device_topic();

	ESP_DBG("process_post, before publish_topic_now\n");
	ret = publish_topic_now(client, topicPublish, json);

	ESP_DBG("process_post, before publish_topic_now\n");
	int ret1 = send_broadcast_data_to_queue(json); //broadcast in LAN

	if (ret1 != 0) {
		ESP_DBG("send broadcast data  failed\n")
						;
	}

	free(json);
	free(topicPublish);

	return ret | ret1;
}

int parse_app_topic(char *topic, char *payload) {

	printf("parse_app_topic, the topic =%s payload=%s\n", topic, payload);

	cJSON * pJsonSub = NULL;
	cJSON * pJson = cJSON_Parse(payload);
	char *method = NULL;
	int ret = 0;
	if (pJson) {
		method = cJSON_GetObjectItem(pJson, "method")->valuestring;

	} else {
		printf("parse_app_topic, pJson=null\n");
		return -1;
	}

	if (strcmp(method, "GET") == 0) {
		ESP_DBG("parse_app_topic, method is GET\n")
		;

		ret = process_get(topic);

	} else if (strcmp(method, "POST") == 0) {
		ESP_DBG("parse_app_topic, method is POST\n")
		;

		pJsonSub = cJSON_GetObjectItem(pJson, "body");
		char *body = NULL;
		if (NULL != pJsonSub) {
			body = cJSON_PrintUnformatted(pJsonSub);
			ret = process_post(body, topic);
			free(body);

		}
	} else {
		ret = -1;
	}

	if (pJsonSub) {
		cJSON_Delete(pJsonSub);
	}

	if (pJson) {
		cJSON_Delete(pJson);
	}

	return ret;

}

LOCAL void received_massage_Task(void *pvParameters) {

	printf("received_massage_Task starts\n");
	MQTTClient *c = pvParameters;
	MqttMessage *data;
	for (;;) {

		if (MQTTNetworkIsDestoryed(c)) {
			//break;
			ESP_DBG("received_massage_Task will Suspend\n")
								;
			vTaskSuspend(NULL);
		}

		printf("received_massage_Task %d words, heap %d bytes\n", (int) uxTaskGetStackHighWaterMark(NULL), system_get_free_heap_size());

		/*char *temp = get_device_datastreams();

		 strcpy(t_json, temp);*/

		/* 从队列中获取内容 */
		if ( xQueueReceive(receivedMessageQueue, (void *)&data,
				100/portTICK_RATE_MS) == pdPASS) {



			ESP_DBG("the address of data is %X\n",data)
			;

			parse_app_topic(data->topic, data->payload);

		}

		vTaskDelay(1* 1000 / portTICK_RATE_MS);
	}

	if (xReceivedMessageHandle != NULL) {
		xReceivedMessageHandle = NULL;
	}

	ESP_DBG("received_massage_Task is deleted\n")
				;
	vTaskDelete(NULL);
}

LOCAL void publish_datastreams_Task1(void *pvParameters) {

	printf("publish_datastreams_Task starts\n");

	MQTTClient *c = pvParameters;

	char *t_json = zalloc(sizeof(char) * MESSAGE_SIZE);
	int count = 0;

	portTickType xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	for (;;) {

		ESP_DBG("publish_datastreams_Task is running\n")
		;

		if (MQTTNetworkIsDestoryed(c)) {
			//break;
			ESP_DBG("publish_datastreams_Task will Suspend\n")
			;
			vTaskSuspend(NULL);
		}

		ESP_DBG("publish_datastreams_Task is running ,pass the check\n")
		;

		printf("publish_datastreams_Task %d words, heap %d bytes\n", (int) uxTaskGetStackHighWaterMark(NULL), system_get_free_heap_size());

		/* 从队列中获取内容 */
		if ( xQueueReceive(DatastreamsQueue, t_json,
				100/portTICK_RATE_MS) == pdPASS) {
			ESP_DBG(
					"publish_datastreams_Task: the %d time recevied from DatastreamsQueue:\n%s\n",
					count, t_json)
			;
			//publish_systerm_datastreams(pvParameters, t_json);
			//publish_device_topic(pvParameters, t_json);
		}

		vTaskDelayUntil(&xLastWakeTime, 5 * 1000 / portTICK_RATE_MS);
	}

	if (DatastreamsQueue != NULL) {
		DatastreamsQueue = NULL;
	}

	free(t_json);
	ESP_DBG("publish_datastreams_Task is deleted\n")
	;
	vTaskDelete(NULL);

}



void start_report_data_task(MQTTClient *c) {


	if (c->ipstack->network_status != TCP_CONNECTED) {
		return;
	}

	//datapoints queue
	DatastreamsQueue = xQueueCreate(5, sizeof(MqttMessage * ));
	receivedMessageQueue = xQueueCreate(5, sizeof(MqttMessage * ));

	unsigned portBASE_TYPE uxTaskPriority = uxTaskPriorityGet(NULL);

	// task for handle received mqtt message
	 if(xReceivedMessageHandle != NULL){
		 vTaskResume(xReceivedMessageHandle);
		 ESP_DBG("received_massage_Task is resumed\n");
	 }else{

		 receivedMessageTask = xTaskCreate(received_massage_Task, ( signed portCHAR * ) "received_massage_Task",	 MQTT_DATASTREAMS_STACK_WORDS, c, uxTaskPriority, &xReceivedMessageHandle);
		 ESP_DBG("received_massage_Task is created\n");
	 }


	 // task for publish topic
	if (xTaskMqttPublishHandle != NULL) {
		vTaskResume(xTaskMqttPublishHandle);
		ESP_DBG("publish_datastreams_Task is resumed\n")
		;
	} else {

		mqttPublishTask = xTaskCreate(publish_datastreams_Task, ( signed portCHAR * ) "publish_datastreams_Task", MQTT_DATASTREAMS_STACK_WORDS, c, uxTaskPriority, &xTaskMqttPublishHandle);
		ESP_DBG("publish_datastreams_Task is created\n")
		;
	}


}











/**
 * subit the message to queue and process it asyn,  so that the read socket funtion wont be blocked.
 */
void messageArrived(MessageData* data)
{
	MqttMessage *message = malloc(sizeof(MqttMessage));

	int topicLen = data->topicName->lenstring.len;
	int payloadLen = data->message->payloadlen;

	message->topic = malloc(topicLen + 1);
	message->payload = malloc(payloadLen + 1);

	strncpy(message->topic, data->topicName->lenstring.data, topicLen);
	message->topic[topicLen] ='\0';
	strcpy(message->payload, data->message->payload);

	ESP_DBG("messageArrived, topic is %s\n",message->topic);
	xQueueSend(receivedMessageQueue, (void * )&message, 100/portTICK_RATE_MS);
}



/*void messageArrived(MessageData* data) {

	int topicLen = data->topicName->lenstring.len;
	char topic[topicLen+1];
	strncpy(topic,data->topicName->lenstring.data,topicLen);
	topic[topicLen] = '\0';

	ESP_DBG("messageArrived,topic =  %s\n", topic);



	char *topic2 = (char*)zalloc(topicLen+1);

	int i=0;
	  for (i = 0; i < topicLen; ++i){
				    	sprintf(topic2,"%c",data->topicName->lenstring.data[i]);
				    	topic2++;
				    }



	ESP_DBG("messageArrived,topic =  %s\n", topic2-topicLen);




    parse_app_topic(topic,(char *) data->message->payload);
	//free(topic);
}*/



LOCAL int connect_to_network(Network *network) {
	
	int rc = 0;
	char* address = MQTT_BROKER;
	while ((rc = NetworkConnect(network, address, MQTT_PORT)) != 0) {
		printf("Return code from network connect is %d\n", rc);
	}
	
	network->network_status = TCP_CONNECTED;
	printf("connected to internet, fd is %d\n", network->my_socket);
	
	return rc;


}

void init_mqtt_client(MQTTClient *client, Network *network) {
//	 unsigned char sendbuf[MESSAGE_SIZE], readbuf[MESSAGE_SIZE];
	
	char *sendbuf = zalloc(sizeof(char) * MESSAGE_SIZE);
	char *readbuf = zalloc(sizeof(char) * MESSAGE_SIZE);
	
	MQTTClientInit(client, network, 30000, sendbuf, MESSAGE_SIZE, readbuf,
	MESSAGE_SIZE);
}

void reinit_mqtt_client(void) {
	//	 unsigned char sendbuf[MESSAGE_SIZE], readbuf[MESSAGE_SIZE];

	char *sendbuf = zalloc(sizeof(char) * MESSAGE_SIZE);
	char *readbuf = zalloc(sizeof(char) * MESSAGE_SIZE);
	MutexLock(&client->mutex);
	client->buf = sendbuf;
	client->buf_size = MESSAGE_SIZE;
	client->readbuf = readbuf;
	client->readbuf_size = MESSAGE_SIZE;
	MutexUnlock(&client->mutex);

}

/**
 * once the device disconnected from server,will publish the will topic.
 * In oreder to let all other clients relealize this device disconnected.
 */

void init_mqtt_will(MQTTPacket_willOptions *will) {
	strcpy(will->struct_id, MQTT_WILL_STRUCT_ID);
	will->struct_version = MQTT_WILL_STRUCT_VERSION;
	MQTTString topic = {get_device_sub_topic(SUB_TOPIC_WILL),{0,NULL}};
	will->topicName = topic;
	will->qos = QOS0;
	MQTTString message = {"{\"response\":{\"offline\":1}}",{0,NULL}};
	will->message = message;
	will->retained = MQTT_WILL_RETAINED;
}



LOCAL int connect_mqtt_server(MQTTClient *client) {
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
	
	connectData.MQTTVersion = MQTT_VERSION;
	connectData.clientID.cstring = MQTT_CLIENT_ID; // you client's unique identifier
	connectData.username.cstring = MQTT_USER;
	connectData.password.cstring = MQTT_PASS;
	connectData.keepAliveInterval = MQTT_KEEP_ALIVE_INTERVAL; // interval for PING message to be sent (seconds)
	connectData.cleansession = MQTT_CLEAN_SESSION;
	connectData.willFlag = MQTT_WILL_FLAG;
	MQTTPacket_willOptions will = MQTTPacket_willOptions_initializer;
	init_mqtt_will(&will);
	ESP_DBG("init_mqtt_will finished,struct_id is %s\n",will.struct_id);
	connectData.will = will;
	ESP_DBG("before MQTTConnect,the will topic is %s,payload is %s\n",connectData.will.topicName.cstring,connectData.will.message.cstring);
	int rc;
	while ((rc = MQTTConnect(client, &connectData)) != 0) {
		
		printf("Return code from MQTT connect is %d\n", rc);
	}
	
	printf("MQTT Connected\n");

	return rc;

}


LOCAL int publish_all_datapoint(MQTTClient *client) {

	//publish device info it connects
	char *topic = get_device_sub_topic(SUB_TOPIC_DEVICE_TYPE);
	char *appTopic = get_app_sub_topic(SUB_TOPIC_DEVICE_TYPE);
	char *json = get_datapoints(appTopic);
	int rc = 0, retry = 0;
	do{
		rc = publish_device_topic(client, topic, json);
		retry++;
	}while (rc ==-1 && retry < RETRY_TIMES);


	return rc;
}


LOCAL int subscribe_device_topic(MQTTClient *client) {

	int rc = 0, retry = 0;
	char *topic = get_app_topic_wildcards();

	do {
		rc = MQTTSubscribe(client, topic, QOS0, messageArrived);
		retry++;
	} while (rc == -1 && retry < RETRY_TIMES);

	if (rc == -1) {
		return rc;
	}

	ESP_DBG("topic =%s is scessfully subscribed\n",topic);

	char *topic1 = get_app_topic();

	do {
		rc = MQTTSubscribe(client, topic1, QOS0, messageArrived);
		retry++;
	} while (rc == -1 && retry < RETRY_TIMES);

	if(rc != -1){
		ESP_DBG("topic =%s is scessfully subscribed\n",topic1);
	}
	return rc;

}

/*LOCAL void subscribe_device_topic(MQTTClient *client) {
 int rc;
 if ((rc = MQTTSubscribe(client, COMMAND_TOPIC, QOS0, messageArrived))
 != 0) {
 printf("Return code from MQTT subscribe %s is %d\n",
 ONENET_COMMAND_TOPIC, rc);
 } else {
 printf("MQTT subscribe to topic %s\n", ONENET_COMMAND_TOPIC);
 }

 if ((rc = (MQTTSetMessageHandler(client, ONENET_COMMAND_TOPIC,
 messageArrived))) != SUCCESS) {
 ESP_DBG("%s handler register failed\n", ONENET_COMMAND_TOPIC);
 }
 }*/


LOCAL int connect(void){

	ESP_DBG("mqtt connect starts\n");

	 Network *network=  malloc(sizeof(Network));
	
	NetworkInit(network);
	
	//MQTTClient client;
	client = malloc(sizeof(MQTTClient));
	
	init_mqtt_client(client, network);
	
	int rc = 0;

	if((rc = connect_to_network(network)) == -1){
		goto exitFuc;
	}
	if((rc = connect_mqtt_server(client)) == -1){
		goto exitFuc;
	}
	exitFuc:
	return rc;
}

LOCAL int reconnect(void){

	ESP_DBG("mqtt reconnect starts\n");

	reinit_mqtt_client();

	Network *network = client->ipstack;

	int rc = 0;

	if((rc = connect_to_network(network)) == -1){
		goto exitFuc;
	}
	if((rc = connect_mqtt_server(client)) == -1){
		goto exitFuc;
	}
	exitFuc:
	return rc;
}

void mqtt_client_thread(void* pvParameters) {
	printf("mqtt client thread starts\n");

	//in case the old resources still exitst when new resorces started.

	int rc;
	if (client != NULL) {
		user_conn_deinit();
		rc = reconnect();
	}else{
		rc = connect();
	}

	if(rc == -1)
		goto exitFuc;

	if((rc = subscribe_device_topic(client)) == -1){
		printf("failed to subscribe topic  after established connection\n");
	}else{
		ESP_DBG("Succeed to subscribe topic  after established connection\n");
	}
	if((rc = publish_all_datapoint(client)) == -1){
		printf("failed to  publish the first topic  after established connection\n");
	}else{
		ESP_DBG("Succeed to publish the first topic  after established connection\n");
	}

	#if defined(MQTT_TASK)

		if ((rc = MQTTStartTask(client)) != pdPASS) {
			printf("Return code from start tasks is %d\n", rc);
		} else {
			printf("Use MQTTStartTask\n");
		}

		start_report_data_task(client);

	#endif

exitFuc:
	
	printf("mqtt_client_thread going to be deleted\n");
	vTaskDelete(NULL);
}


/*
void mqtt_client_thread(void* pvParameters) {
	printf("mqtt client thread starts\n");

	//in case the old resources still exitst when new resorces started.
	if (client != NULL) {
		user_conn_deinit();
	}

	Network network;

	NetworkInit(&network);

	//MQTTClient client;
	client = malloc(sizeof(MQTTClient));

	init_mqtt_client(client, &network);

	int rc = 0;

	if((rc = connect_to_network(&network)) == -1){
		goto exitFuc;
	}
	if((rc = connect_mqtt_server(client)) == -1){
		goto exitFuc;
	}


	if((rc = subscribe_device_topic(client)) == -1){
		printf("failed to subscribe topic  after established connection\n");
	}else{
		ESP_DBG("Succeed to subscribe topic  after established connection\n");
	}

	if((rc = publish_all_datapoint(client)) == -1){
		printf("failed to  publish the first topic  after established connection\n");
	}else{
		ESP_DBG("Succeed to publish the first topic  after established connection\n");
	}

	#if defined(MQTT_TASK)

		if ((rc = MQTTStartTask(client)) != pdPASS) {
			printf("Return code from start tasks is %d\n", rc);
		} else {
			printf("Use MQTTStartTask\n");
		}

		start_report_data_task(client);

	#endif

exitFuc:

	printf("mqtt_client_thread going to be deleted\n");
	vTaskDelete(NULL);
}*/



void user_conn_init(void) {
	int ret;
	ret = xTaskCreate(mqtt_client_thread, MQTT_CLIENT_THREAD_NAME, MQTT_CLIENT_THREAD_STACK_WORDS, NULL, MQTT_CLIENT_THREAD_PRIO, &mqttc_client_handle);
	
	if (ret != pdPASS) {
		printf("mqtt create client thread %s failed\n",
		MQTT_CLIENT_THREAD_NAME);
	}
	
}

/*
void user_conn_deinit(void) {
	
	printf("user_conn_deinit starts\n");
	
	MQTTClient* c = client;
	
	ESP_DBG("user_conn_deinit before get lock\n");
	MutexLock(&c->mutex);
	ESP_DBG("user_conn_deinit after get lock\n");
	
	if (c != NULL) {
		
		if (c->ipstack != NULL) {
			//set network_status, while all tasks related to mqtt loops will check the status, once the status is not connected, they will delete themselves.
			c->ipstack->network_status = TCP_DISCONNECTED;

			//disconnect from mqtt server

					ESP_DBG("user_conn_deinit,will do MQTTDisconnect");
			 MQTTDisconnect(client1);
			 ESP_DBG("user_conn_deinit,have done MQTTDisconnect");
			 //close connection
			c->ipstack->disconnect(c->ipstack);

			//reset the socket fd
			NetworkDeinit(c->ipstack);

			//free memory
			free(c->ipstack);
			c->ipstack = NULL;

		}
		
		free(c->buf);
		c->buf = NULL;
		free(c->readbuf);
		c->readbuf = NULL;
		free(c);
		c = NULL;
		
	}
	
	ESP_DBG("user_conn_deinit before release lock\n");
	MutexUnlock(&c->mutex);
	ESP_DBG("user_conn_deinit after release lock\n");
}
*/

void user_conn_deinit(void) {

	printf("user_conn_deinit starts\n");

	MQTTClient* c = client;


	if (!MQTTNetworkIsDestoryed(c)) {


		ESP_DBG("user_conn_deinit before get lock\n");
		MutexLock(&c->mutex);
		ESP_DBG("user_conn_deinit after get lock\n");

			//set network_status, while all tasks related to mqtt loops will check the status, once the status is not connected, they will suspend themselves.
			c->isconnected = 0;

			 //close connection
			c->ipstack->disconnect(c->ipstack);

			//reset the socket fd
			NetworkDeinit(c->ipstack);

			free(c->buf);
			c->buf = NULL;
			free(c->readbuf);
			c->readbuf = NULL;

			ESP_DBG("user_conn_deinit before release lock\n");
			MutexUnlock(&c->mutex);
			ESP_DBG("user_conn_deinit after release lock\n");
		}



	}



