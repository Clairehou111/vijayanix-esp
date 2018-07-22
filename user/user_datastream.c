/*
 * user_datastream.c
 *
 *  Created on: 2018Äê7ÔÂ13ÈÕ
 *      Author: hxhoua
 */
#include "esp_common.h"
#include "user_mqtt.h"
#include "user_datastream.h"

int get_subtopic(char *topic) {

	char* appTopic = get_app_topic();
	if (topic == NULL || strcmp(topic, appTopic) == 0) {
		return SUB_TOPIC_ALL;
	}
	char appTopic_sub[30];
	sprintf(appTopic_sub, "%s%s", appTopic, TOPIC_LEVEL_SEPARATOR);

	int len = strlen(appTopic_sub);
	char sub[8];

	strcpy(sub, topic + len);

	return atoi(sub);

}

int topic_to_datastream(char *topic) {

	int subTopic = get_subtopic(topic);

	switch (subTopic) {
	case SUB_TOPIC_ALL:
		return SUB_DATASTREAM_ALL;
	case SUB_TOPIC_WATCHMAN_LIGHTA:
		return SUB_DATASTREAM_WATCHMAN_LIGHTA;
	case SUB_TOPIC_WATCHMAN_LIGHTB:
		return SUB_DATASTREAM_WATCHMAN_LIGHTB;
	default:
		return SUB_DATASTREAM_ALL;
	}

}

char* datastream_to_topic(int subDatastream) {

	switch (subDatastream) {
	case SUB_DATASTREAM_ALL:
		return get_device_topic();
	case SUB_DATASTREAM_WATCHMAN_LIGHTA:
		return get_device_sub_topic(SUB_TOPIC_WATCHMAN_LIGHTA);
	case SUB_DATASTREAM_WATCHMAN_LIGHTB:
		return get_device_sub_topic(SUB_TOPIC_WATCHMAN_LIGHTB);

	}

}
