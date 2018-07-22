/******************************************************************************
 * Copyright (C) 2014 -2016  Espressif System
 *
 * FileName: user_esp_platform.c
 *
 * Description: The client mode configration.
 *              Check your hardware connection with the host while use this mode.
 *
 * Modification history:
 * 2015/7/1, v1.0 create this file.
 *******************************************************************************/

#include "esp_common.h"

#include "MQTTFreeRTOS.h"

#include "lwip/lwip/sys.h"
#include "lwip/lwip/ip_addr.h"
#include "lwip/lwip/netdb.h"
#include "lwip/lwip/sockets.h"
#include "lwip/lwip/err.h"

#include "smartconfig.h"

#include "upgrade.h"

#include "user_config.h"
#include "user_iot_version.h"

#include "user_esp_platform_timer.h"
#include "user_esp_wifi.h"
#include "include_device.h"


#define REBOOT_MAGIC  (12345)

LOCAL int user_boot_flag;

struct esp_platform_saved_param esp_param;

LOCAL uint8 device_status;

LOCAL struct rst_info rtc_info;

LOCAL uint8 iot_version[20];


/*void user_esp_platform_set_active(uint8 activeflag);
 void user_esp_platform_set_token(char *token);
 void user_esp_platform_init(void);
 sint8   user_esp_platform_deinit(void);*/

/******************************************************************************
 * FunctionName : smartconfig_done
 * Description  : callback function which be called during the samrtconfig process
 * Parameters   : status -- the samrtconfig status
 *                pdata --
 * Returns      : none
 *******************************************************************************/

void smartconfig_done(sc_status status, void *pdata) {
	switch (status) {
	case SC_STATUS_WAIT:
		printf("SC_STATUS_WAIT\n");
		user_link_led_output(LED_1HZ);
		break;
	case SC_STATUS_FIND_CHANNEL:
		printf("SC_STATUS_FIND_CHANNEL\n");
		user_link_led_output(LED_1HZ);
		break;
	case SC_STATUS_GETTING_SSID_PSWD:
		printf("SC_STATUS_GETTING_SSID_PSWD\n");
		user_link_led_output(LED_20HZ);

		sc_type *type = pdata;
		if (*type == SC_TYPE_ESPTOUCH) {
			printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
		} else {
			printf("SC_TYPE:SC_TYPE_AIRKISS\n");
		}
		break;
	case SC_STATUS_LINK:
		printf("SC_STATUS_LINK\n");
		struct station_config *sta_conf = pdata;
		wifi_station_set_config_current(sta_conf);
		wifi_station_disconnect();
		wifi_station_connect();
		break;
	case SC_STATUS_LINK_OVER:
		printf("SC_STATUS_LINK_OVER\n");
		if (pdata != NULL) {
			uint8 phone_ip[4] = { 0 };
			memcpy(phone_ip, (uint8*) pdata, 4);
			printf("Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1],
					phone_ip[2], phone_ip[3]);
		}
		smartconfig_stop();
		wifi_station_set_config(sta_conf);
		wifi_set_opmode(STATIONAP_MODE);
		user_link_led_output(LED_OFF);
		device_status = DEVICE_GOT_IP;
		break;
	}

}

/******************************************************************************
 * FunctionName : smartconfig_task
 * Description  : start the samrtconfig proces and call back 
 * Parameters   : pvParameters
 * Returns      : none
 *******************************************************************************/
void smartconfig_task(void *pvParameters) {

	printf("smartconfig_task start\n");

	Timer* timer = (Timer*) pvParameters;

	smartconfig_start(smartconfig_done);

	while (device_status != DEVICE_GOT_IP) {

		if (TimerIsExpired(timer)) {

			ESP_DBG("Timer Is Expired, stop trying smartconfig\n")
			;
			smartconfig_stop();
			break;
		} else {
			ESP_DBG("configing...\n")
			;
			vTaskDelay(2000 / portTICK_RATE_MS);
		}

	}

	vTaskDelete(NULL);
}

LOCAL void smartconfig(void *pvParameters) {

	bool ret = 0, ret1 = 1;
	printf("smartconfig_task start\n");
	Timer* timer = (Timer*) pvParameters;
	smartconfig_start(smartconfig_done);

	while (device_status != DEVICE_GOT_IP) {

		if (TimerIsExpired(timer)) {

			ESP_DBG("Timer Is Expired, stop trying smartconfig\n")
			;
			smartconfig_stop();

			user_link_led_output(LED_OFF);


			break;

		} else {
			ESP_DBG("configing...\n")
			;
			vTaskDelay(2000 / portTICK_RATE_MS);
		}

	}

}

/******************************************************************************
 * FunctionName : user_esp_platform_ap_change
 * Description  : add the user interface for changing to next ap ID.
 * Parameters   :
 * Returns      : none
 *******************************************************************************/
LOCAL void user_esp_ap_change(void) {
	uint8 current_id;
	uint8 i = 0;

	current_id = wifi_station_get_current_ap_id();
	ESP_DBG("current ap id =%d\n", current_id)
	;

	if (current_id == AP_CACHE_NUMBER - 1) {
		i = 0;
	} else {
		i = current_id + 1;
	}
	while (wifi_station_ap_change(i) != true) {
		//try it out until universe collapses
		ESP_DBG("try ap id =%d\n", i)
		;
		vTaskDelay(3000 / portTICK_RATE_MS);
		i++;
		if (i == AP_CACHE_NUMBER - 1) {
			i = 0;
		}
	}

}



/******************************************************************************
 * FunctionName : user_esp_platform_param_recover
 * Description  : espconn struct parame init when get ip addr
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/

LOCAL void user_esp_platform_param_recover(void) {
	struct ip_info sta_info;
	struct dhcp_client_info dhcp_info;

	sprintf(iot_version, "%s%d.%d.%dt%d(%s)", VERSION_TYPE, IOT_VERSION_MAJOR,
	IOT_VERSION_MINOR, IOT_VERSION_REVISION, device_type, UPGRADE_FALG);
	printf("IOT VERSION:%s\n", iot_version);

	system_rtc_mem_read(0, &rtc_info, sizeof(struct rst_info));
	if (rtc_info.reason == 1 || rtc_info.reason == 2) {
		ESP_DBG("flag = %d,epc1 = 0x%08x,epc2=0x%08x,epc3=0x%08x,excvaddr=0x%08x,depc=0x%08x,\nFatal \
    exception (%d): \n",rtc_info.reason,rtc_info.epc1,rtc_info.epc2,rtc_info.epc3,rtc_info.excvaddr,rtc_info.depc,rtc_info.exccause)
		;
	}
	struct rst_info info = { 0 };
	system_rtc_mem_write(0, &info, sizeof(struct rst_info));

	system_param_load(ESP_PARAM_START_SEC, 0, &esp_param, sizeof(esp_param));

	/***add by tzx for saving ip_info to avoid dhcp_client start****/
	/*
	 system_rtc_mem_read(64,&dhcp_info,sizeof(struct dhcp_client_info));
	 if(dhcp_info.flag == 0x01 ) {
	 printf("set default ip ??\n");
	 sta_info.ip = dhcp_info.ip_addr;
	 sta_info.gw = dhcp_info.gw;
	 sta_info.netmask = dhcp_info.netmask;
	 if ( true != wifi_set_ip_info(STATION_IF,&sta_info)) {
	 printf("set default ip wrong\n");
	 }
	 }
	 memset(&dhcp_info,0,sizeof(struct dhcp_client_info));
	 system_rtc_mem_write(64,&dhcp_info,sizeof(struct rst_info));
	 */
	system_rtc_mem_read(70, &user_boot_flag, sizeof(user_boot_flag));

	int boot_flag = 0xffffffff;
	system_rtc_mem_write(70, &boot_flag, sizeof(boot_flag));

	/*restore system timer after reboot, note here not for power off */
	user_platform_timer_restore();
}

/******************************************************************************
 * FunctionName : user_esp_platform_param_recover
 * Description  : espconn struct parame init when get ip addr
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/

void user_stationap_enable(void) {

#if SOFTAP_ENCRYPT //should debug here
	char macaddr[6];
	char password[33];
	struct softap_config config;

	wifi_softap_get_config(&config);
	wifi_get_macaddr(SOFTAP_IF, macaddr);

	memset(config.password, 0, sizeof(config.password));
	//sprintf(password, MACSTR "_%s", MAC2STR(macaddr), PASSWORD);
	memcpy(config.password, MYPASSWORD, strlen(password));
	config.authmode = AUTH_WPA_WPA2_PSK;
	wifi_softap_set_config(&config);

#endif

	wifi_set_opmode(SOFTAP_MODE);
}

void connect_to_exist_ap(Timer *timer,struct ip_info *ipconfig) {
#define MAX_AP_RETRY_CNT 200
	u8 single_ap_retry_count = 0;


	while(ipconfig->ip.addr == 0
			|| wifi_station_get_connect_status() != STATION_GOT_IP){

		if(TimerIsExpired(timer)){
			ESP_DBG("Timer Is Expired, stop trying connect_to_exist_ap\n");
			break;
		}

		ESP_DBG("connecting...\n")	;

		wifi_station_connect();

		printf("ipconfig->ip.addr = %u,wifi_station_get_connect_status()=%d\n",
					ipconfig->ip.addr, wifi_station_get_connect_status());


		/* if there are wrong while connecting to some AP, change to next and reset counter */
		if (wifi_station_get_connect_status() == STATION_WRONG_PASSWORD
				|| wifi_station_get_connect_status() == STATION_NO_AP_FOUND
				|| wifi_station_get_connect_status() == STATION_CONNECT_FAIL
				|| (single_ap_retry_count++ > MAX_AP_RETRY_CNT)) {

		ESP_DBG("try other APs ...\n");
		user_esp_ap_change();

        single_ap_retry_count = 0;
		}

		vTaskDelay(3000 / portTICK_RATE_MS);
		wifi_get_ip_info(STATION_IF, ipconfig);

	}

}

LOCAL bool station_connect(void) {

	wifi_set_opmode(STATION_MODE);

	struct ip_info ipconfig;

	memset(&ipconfig, 0, sizeof(ipconfig));
	wifi_get_ip_info(STATION_IF, &ipconfig);

	device_status = DEVICE_CONNECTING;

	if (ipconfig.ip.addr == 0
			|| wifi_station_get_connect_status() != STATION_GOT_IP) {

		struct station_config *sta_config5 = (struct station_config *) zalloc(
				sizeof(struct station_config) * 5);
		int ap_num = wifi_station_get_ap_info(sta_config5);
		free(sta_config5);
		printf("wifi_station_get_ap num %d\n", ap_num);

		Timer timer;
		TimerInit(&timer);


		if (0 == ap_num) { /*AP_num == 0, no ap cached,start smartcfg*/

			TimerCountdown(&timer, 60*60*24); //10mins ,20s for test only

			ESP_DBG("will create smartconfig_task\n")
			;
			/*xTaskCreate(smartconfig_task, "smartconfig_task", 256, &timer, 2,
			 NULL);
			 */
			smartconfig(&timer);

			ESP_DBG("return back from smartconfig_task\n")
			;

		} else {

			TimerCountdown(&timer, 60*60*24); //10mins ,20s for test only

			ESP_DBG("will connect Exist AP\n")
			;
			connect_to_exist_ap(&timer,&ipconfig);
			ESP_DBG("return back from connect Exist AP\n")
			;
		}

	}
	ESP_DBG("return back from user_esp_station_connect\n")
	;
	ESP_DBG("wifi_station_get_connect_status=%d,device_status=%u\n",wifi_station_get_connect_status(),device_status)
	;
	return wifi_station_get_connect_status() == STATION_GOT_IP;

}

void user_esp_wifi_connect(void *pvParameters) {

	user_esp_platform_param_recover();

#ifdef AP_CACHE
	wifi_station_ap_number_set(AP_CACHE_NUMBER);
#endif

	printf("user_boot_flag is %d ,user_get_key_status is %c \n", user_boot_flag,
			user_get_key_status);
	//printf("rtc_info.reason 000-%d, 000-%d reboot, flag %d\n",rtc_info.reason,(REBOOT_MAGIC == user_boot_flag),user_boot_flag);

	/*if not reboot back, power on with key pressed, enter stationap mode*/
	if ( REBOOT_MAGIC != user_boot_flag && 0 == user_get_key_status()) {
		/*device power on with stationap mode defaultly, neednt config again*/
		user_stationap_enable();
		printf("enter softap+station mode\n");
#if (PLUG_DEVICE2 || SENSOR_DEVICE)
		user_link_led_output(LED_ON); //gpio 12
#endif

	} else {

		//failed to connect to ap, set to softap mode.
		if (!station_connect()) {

			ESP_DBG("failed to connect to ap, set to softap mode...\n");
			user_stationap_enable();
			//user_link_led_output(LED_20HZ);

		} else {
			device_status = DEVICE_GOT_IP;
			ESP_DBG("connect to ap succesffuly\n");
		}
	}

	vTaskDelete(NULL);
}

void user_esp_wifi_task(void) {
#ifdef CLIENT_SSL_ENABLE
	xTaskCreate(user_esp_wifi_connect, "user_esp_wifi_connect", 640, NULL, 5, NULL); //ssl need much more stack
#else
	xTaskCreate(user_esp_wifi_connect, "user_esp_wifi_connect", 384, NULL, 5,
			NULL); //512, 274 left,384
#endif

}

