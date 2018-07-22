/*
 * user_device_status_broadcast.c
 *
 *  Created on: 2018年6月16日
 *      Author: hxhoua
 */

#include "user_device_data_broadcast.h"
#include "mqtt/MQTTFreeRTOS.h"

#include "MQTTClient.h"
#include "httpd.h"


LOCAL xQueueHandle DatastreamsQueue;

//#define MAX_CONN 8


LOCAL int sendPacket(int fd, char *buf, int length, Timer* timer) {
/*	int rc = FAILURE, sent = 0;

	if (TimerIsExpired(timer)) {
		ESP_DBG("TimerIsExpired\n");
	}

	Network network;

	NetworkInit(&network);
    network.my_socket = fd;

	while (sent < length && !TimerIsExpired(timer)) {

		rc = network.mqttwrite(&network, &buf[sent], length, TimerLeftMS(timer));

		if (rc < 0)  // there was an error writing the data
			break;
		sent += rc;
	}
	if (sent == length) {
		rc = SUCCESS;

		ESP_DBG("%d bytes have been sent out\n", length);
	} else
		rc = FAILURE;
	return rc;*/
}


void user_device_data_broadcast(char *json) {
	//broadcast datastatus
/*
	int index;
	int length = strlen(json);

	Timer timer;
	TimerInit(&timer);
	TimerCountdownMS(&timer, 1000);

	int *ret = calloc(MAX_CONN,sizeof(int));
	httpdFindConnFd(ret);

	for (index = 0; index < MAX_CONN; index++) {
		//find all valid handle
		if (ret[index]>=0) {
			sendPacket(ret[index], json, length, &timer);
		}
	}
*/

}


void device_data_broadcast_task(void *parameter) {
/*	printf("device_data_broadcast_task starts\n");

	char *t_json = zalloc(sizeof(char) * 200);
	int count = 0;

	for (;;) {

		ESP_DBG("device_data_broadcast_task is running\n");

		ESP_DBG("device_data_broadcast_task is running ,pass the check\n");

		 printf("device_data_broadcast_task %d words, heap %d bytes\n",(int)uxTaskGetStackHighWaterMark(NULL), system_get_free_heap_size());

		 从队列中获取内容
		if ( xQueueReceive(DatastreamsQueue, t_json,
				100/portTICK_RATE_MS) == pdPASS) {
			ESP_DBG(
					"device_data_broadcast_task: the %d time recevied from DatastreamsQueue:\n%s\n",
					count, t_json);
			user_device_data_broadcast(t_json);
		}

		vTaskDelay(5*1000 / portTICK_RATE_MS);
		count++;
	}

	if (DatastreamsQueue != NULL) {
		DatastreamsQueue = NULL;
	}

	free(t_json);
	ESP_DBG("device_data_broadcast_task is deleted\n");
	vTaskDelete(NULL);*/

}





void   user_device_data_broadcast_start(void)
 {
	//datapoints queue
	/*	DatastreamsQueue = xQueueCreate(5, sizeof(char)*MESSAGE_SIZE);

	    uint16_t usTaskStackSize = (configMINIMAL_STACK_SIZE * 5);
	    unsigned portBASE_TYPE uxTaskPriority = uxTaskPriorityGet(NULL);  set the priority as the same as the calling task


			// task for generate datastreams
			 xTaskCreate(device_data_broadcast_task,
					( signed portCHAR * ) "user_device_data_broadcast_start",
					usTaskStackSize, NULL, uxTaskPriority,
					NULL);
			ESP_DBG("user_device_data_broadcast_start is created\n");*/


 }


signed portBASE_TYPE send_broadcast_data_to_queue(void *message) {
/*	if (DatastreamsQueue == NULL) {
		return -1;
	}
	return xQueueSend(DatastreamsQueue, message, 0);*/
	return 0;
}


