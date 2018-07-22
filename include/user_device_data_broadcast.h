/*
 * user_device_data_broadcast.h
 *
 *  Created on: 2018Äê6ÔÂ16ÈÕ
 *      Author: hxhoua
 */

#ifndef INCLUDE_USER_DEVICE_DATA_BROADCAST_H_
#define INCLUDE_USER_DEVICE_DATA_BROADCAST_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

signed portBASE_TYPE send_broadcast_data_to_queue(void *message);


#endif /* INCLUDE_USER_DEVICE_DATA_BROADCAST_H_ */
