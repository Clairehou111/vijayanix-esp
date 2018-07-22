#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

//#include "esp_libc.h"
//#include <stdio.h>
#include "esp_common.h"
/*support one platform at the same project*/
#define ESP_PLATFORM            0
#define LEWEI_PLATFORM          0
#define ONENET_PLATFORM		    1
#define DIRECTLYCONFIG  		0
/*support one server at the same project*/
#define HTTPD_SERVER            1
#define WEB_SERVICE             0

/*support one device at the same project*/
#define PLUG_DEVICE             0
#define LIGHT_DEVICE            0
#define SENSOR_DEVICE           0 //TBD
#define PLUG_DEVICE2           0 //TBD
#define WATCHMAN_DEVICE           1 //TBD


#if LIGHT_DEVICE
#define USE_US_TIMER
#endif

#define RESTORE_KEEP_TIMER 0

#if ONENET_PLATFORM

#if PLUG_DEVICE || LIGHT_DEVICE
#define BEACON_TIMEOUT  150000000
#define BEACON_TIME     50000
#endif

#if SENSOR_DEVICE
#define HUMITURE_SUB_DEVICE         1
#define FLAMMABLE_GAS_SUB_DEVICE    0
#endif

#if SENSOR_DEVICE
#define SENSOR_DEEP_SLEEP

#if HUMITURE_SUB_DEVICE
#define SENSOR_DEEP_SLEEP_TIME    30000000
#elif FLAMMABLE_GAS_SUB_DEVICE
#define SENSOR_DEEP_SLEEP_TIME    60000000
#endif
#endif

#define AP_CACHE
#ifdef AP_CACHE
#define AP_CACHE_NUMBER    5
#endif


#define SOFTAP_ENCRYPT    1


#ifdef SOFTAP_ENCRYPT
#define MYPASSWORD    "0123456789"
#endif

/*
#define USE_DNS
#define ESP_DOMAIN      "iot.espressif.cn"
*/

/*SSL not Ready*/
//#define SERVER_SSL_ENABLE 
//#define CLIENT_SSL_ENABLE
//#define UPGRADE_SSL_ENABLE




#define SSID         "clairehouhot2"        /* Wi-Fi SSID */
#define PASSWORD     "0123456789"     /* Wi-Fi Password */

/*#define SSID         "902"         Wi-Fi SSID
#define PASSWORD     "2281129902"      Wi-Fi Password */

#define MQTT_BROKER  "183.230.40.39"  /* MQTT Broker Address*/
#define MQTT_PORT    6002             /* MQTT Port*/

#ifndef MQTT_TASK
#define MQTT_TASK
#endif

#define MQTT_CLEAN_SESSION    0 // interval for PING message to be sent (seconds)
#define MQTT_KEEP_ALIVE_INTERVAL    60 // interval for PING message to be sent (seconds)
#define MQTT_WILL_FLAG    1
#define MQTT_WILL_STRUCT_ID    "MQTW"
#define MQTT_WILL_STRUCT_VERSION    0
#define MQTT_WILL_RETAINED   0
#define MQTT_VERSION    4
#define MQTT_USER     "142577"

#if PLUG_DEVICE||PLUG_DEVICE2
#define MQTT_CLIENT_ID    "31589734"
#define MQTT_PASS     "socket1"
#endif

#if WATCHMAN_DEVICE
#define MQTT_CLIENT_ID    "35838489"
#define MQTT_PASS     "watchman"
#endif

#define MQTT_REPORT_DATA_INTERVAL 30 //interval for datapoints to be sent(seconds)








#define ENALBE_GDBDEBUG 1
#ifdef ENALBE_GDBDEBUG
#define GDB_IRAM_ATTR __attribute__((section(".iram.text")))
#else
#define GDB_IRAM_ATTR
#endif


#define DEBUG_ON  1

#if defined(DEBUG_ON)
#define ESP_DBG os_printf
#else
#define ESP_DBG
#endif


#elif LEWEI_PLATFORM

#endif

#endif

