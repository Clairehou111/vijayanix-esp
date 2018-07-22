/*
 * user_mqtt.h
 *
 *  Created on: 2018Äê7ÔÂ3ÈÕ
 *      Author: hxhoua
 */

#ifndef INCLUDE_USER_MQTT_H_
#define INCLUDE_USER_MQTT_H_



#define ONENET_COMMAND_TOPIC 	"$creq/#"
#define TOPIC_WILDCARDS  "#"
#define APP_TOPIC 	"app/device/"
#define APP_ALL_DEVICE_TOPIC 	"app/device/all"
#define DEVICE_TOPIC 	"device/device/"
#define SUB_TOPIC_TYPE
#define ONENET_REPORT_DATA_TOPIC 	"$dp"
#define TOPIC_LEVEL_SEPARATOR  "/"

typedef struct MqttMessage {
    char* payload;
    char* topic;
} MqttMessage;

void user_conn_deinit(void);
void user_conn_init(void);
signed portBASE_TYPE send_datastreams_to_queue(void *topic,void* payload);
char* get_device_topic(void);
char* get_device_sub_topic(int subTopic);
char* get_app_topic(void);


#endif /* INCLUDE_USER_MQTT_H_ */
