/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"
#include "user_config.h"
//#include "esp_common.h"
//
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "httpdserver.h"
#include "include_device.h"

#include "user_esp_wifi.h"
#include "user_devicefind.h"
#include "user_mqtt.h"

int wifi_status = 0;


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void wifi_event_handler_cb(System_Event_t *event)
{
	ESP_DBG("wifi_event_handler_cb\n");
    if (event == NULL) {
        return;
    }

    switch (event->event_id) {
        case EVENT_STAMODE_GOT_IP:
            os_printf("sta got ip ,create task and free heap size is %d\n", system_get_free_heap_size());
            user_conn_init();
            wifi_status =  1;
            break;

        case EVENT_STAMODE_CONNECTED:
            os_printf("sta connected\n");
            break;

        case EVENT_STAMODE_DISCONNECTED:
        	os_printf("sta disconnected\n");

        	//this means last time the status was connected
        	if(wifi_status == 1){
        		user_conn_deinit();
        	}

        	wifi_status =  -1;
            wifi_station_connect();

            break;

        default:
            break;
    }
}


LOCAL void print_reset_reason(void){
    struct rst_info *rtc_info = system_get_rst_info();
    os_printf( "reset reason: %x\n", rtc_info->reason);
       if (rtc_info->reason == REASON_WDT_RST ||
           rtc_info->reason == REASON_EXCEPTION_RST ||
           rtc_info->reason == REASON_SOFT_WDT_RST)
       {
           if (rtc_info->reason == REASON_EXCEPTION_RST)
           {
        	   os_printf("Fatal exception (%d):\n", rtc_info->exccause);
           }
           os_printf( "epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
                   rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);
       }
}


LOCAL void connect2Wifi(void){

	    wifi_set_opmode(STATION_MODE);
	    struct station_config config;
	    bzero(&config, sizeof(struct station_config));
	    sprintf(config.ssid, SSID);
	    sprintf(config.password, PASSWORD);
	    wifi_station_set_config(&config);

	    wifi_set_event_handler_cb(wifi_event_handler_cb);

	    wifi_station_connect();
}

LOCAL void init_device(void){
#if PLUG_DEVICE

	user_plug_init();
#elif PLUG_DEVICE2
	user_plug_init2();
#elif WATCHMAN_DEVICE
	user_watchman_init();
#endif
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{

    os_printf("SDK version:%s %d\n", system_get_sdk_version(), system_get_free_heap_size());

    print_reset_reason();
	wifi_set_event_handler_cb(wifi_event_handler_cb);
	user_stationap_enable();

	init_device();

# if DIRECTLYCONFIG
	connect2Wifi();
#endif


#if ONENET_PLATFORM
	/*Initialization of the peripheral drivers*/
	/*For light demo , it is user_light_init();*/
	/* Also check whether assigned ip addr by the router.If so, connect to ESP-server  */
	user_esp_wifi_task();

#endif

    /*Establish a udp socket to receive local device detect info.*/
    /*Listen to the port 1025, as well as udp broadcast.
    /*If receive a string of device_find_request, it rely its IP address and MAC.*/
    user_devicefind_start();

#if WEB_SERVICE
   /* Establish a TCP server for http(with JSON) POST or GET command to communicate with the device.
    You can find the command in "2B-SDK-Espressif IoT Demo.pdf" to see the details.*/
    user_webserver_start();

#elif HTTPD_SERVER
	//Initialize DNS server for captive portal
	captdnsInit();

	//Initialize espfs containing static webpages
    espFsInit((void*)(webpages_espfs_start));

	//Initialize webserver
	httpdInit(builtInUrls, 80);

	//user_device_data_broadcast_start();
#endif


}
