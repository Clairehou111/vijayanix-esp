/*
 * user_datastreams_plug.h
 *
 *  Created on: 2018Äê6ÔÂ15ÈÕ
 *      Author: hxhoua
 */

#ifndef INCLUDE_USER_DATASTREAMS_TXT_
#define INCLUDE_USER_DATASTREAMS_TXT_

#define MESSAGE_SIZE 500




typedef enum {
	SUB_DATASTREAM_ALL = 0,
	SUB_DATASTREAM_DEVICE_TYPE = 0x1,
	SUB_DATASTREAM_WATCHMAN_LIGHTA = 0x2,
	SUB_DATASTREAM_WATCHMAN_LIGHTB = 0x3,

} sub_datastream;


typedef enum {
	 SUB_TOPIC_ALL = 0,
	 SUB_TOPIC_DEVICE_TYPE = 0x1,
	 SUB_TOPIC_WATCHMAN_LIGHTA = 0x2,
	 SUB_TOPIC_WATCHMAN_LIGHTB = 0x3,
	 SUB_TOPIC_WILL = 0xFFFF,


}sub_topic;

int get_subtopic(char *topic);
int topic_to_datastream(char *topic);
char* datastream_to_topic(int subDatastream);



#endif /* INCLUDE_USER_DATASTREAMS_TXT_ */
