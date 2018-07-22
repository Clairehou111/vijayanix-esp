/******************************************************************************
 * Copyright (C) 2014 -2016  Espressif System
 *
 * FileName: user_watchman.c
 *
 * Description: watchman demo's function realization
 *
 * Modification history:
 * 2015/7/1, v1.0 create this file.
*******************************************************************************/
#include "esp_common.h"
#include "user_config.h"
#include "user_device_data_broadcast.h"
#include "user_mqtt.h"
#include "bitwise_ops.h"
#include "user_cgi.h"
#include "user_datastream.h"

#if WATCHMAN_DEVICE
#include "user_watchman.h"


LOCAL Watchman *watchman;
LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[WATCHMAN_KEY_NUM];
LOCAL os_timer_t link_led_timer;
LOCAL uint8 link_led_level = 0;

/******************************************************************************
 * FunctionName : user_watchman_get_status
 * Description  : get watchman's status, 0x00 or 0x01
 * Parameters   : none
 * Returns      : uint8 - watchman's status
*******************************************************************************/
Watchman*
user_watchman_get_status(void)
{
    return watchman;
}




void
user_watchman_set_lightA(bool lightA)
{
	  if (lightA != watchman->lightA) {
	    	watchman->lightA = lightA;
	    	WATCHMAN_STATUS_OUTPUT(WATCHMAN_RELAY_LED_IO_NUM, lightA);
	    }

}

void
user_watchman_set_lightB(bool lightB)
{
    if (lightB != watchman->lightB) {
    	watchman->lightB = lightB;
    	WATCHMAN_STATUS_OUTPUT(WATCHMAN_RELAY_LED_IO_NUM1, lightB);
    }
}


/******************************************************************************
 * FunctionName : user_watchman_set_status
 * Description  : set watchman's status, 0x00 or 0x01
 * Parameters   : uint8 - status
 * Returns      : none
*******************************************************************************/
void
user_watchman_set_status(bool lightA,bool lightB)
{
	ESP_DBG("user_watchman_set_status starts\n");
	ESP_DBG("lightA=%d,lightB=%d\n",lightA,lightB);
	user_watchman_set_lightA(lightA);
	user_watchman_set_lightB(lightB);
}





LOCAL void flash_param(void){
	  spi_flash_erase_sector(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE);
	    spi_flash_write((PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
	    		(uint32 *)watchman, sizeof(Watchman));

}

/******************************************************************************
 * FunctionName : user_watchman_short_press
 * Description  : key's short press function, needed to be installed
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
LOCAL void
user_watchman_short_press(void)
{
	printf("key0 is short pressed,present watchman_param.status =%d\n ", watchman->lightA);

	user_watchman_set_lightA(!watchman->lightA);

	flash_param();

	char *json = get_datapoints(get_app_topic());
	char *topic = get_device_sub_topic(SUB_TOPIC_WATCHMAN_LIGHTA);

	ESP_DBG("user_watchman_short_press, key0, topic is %s, payload is \n%s \n",topic,json)
	;
	send_datastreams_to_queue(topic, json);	//publish topic
	//send_broadcast_data_to_queue(json); //broadcast in LAN

}

/******************************************************************************
 * FunctionName : user_watchman_long_press
 * Description  : key's long press function, needed to be installed
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
LOCAL void  
user_watchman_long_press(void)
{
    int boot_flag=12345;
    //user_esp_platform_set_active(0);
    system_restore();
    
    system_rtc_mem_write(70, &boot_flag, sizeof(boot_flag));
    printf("long_press boot_flag %d  \n",boot_flag);
    system_rtc_mem_read(70, &boot_flag, sizeof(boot_flag));
    printf("long_press boot_flag %d  \n",boot_flag);

#if RESTORE_KEEP_TIMER
    user_platform_timer_bkup();
#endif 

    system_restart();
}


/******************************************************************************
 * FunctionName : user_watchman_short_press
 * Description  : key's short press function, needed to be installed
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
LOCAL void
user_watchman_short_press2(void)
{
	printf("key1 is short pressed,present watchman_param.status =%d\n ", watchman->lightB);
	user_watchman_set_lightB(!watchman->lightB);
	flash_param();

	char *json = get_datapoints(get_app_topic());
	char *topic = get_device_sub_topic(SUB_TOPIC_WATCHMAN_LIGHTB);

	ESP_DBG("user_watchman_short_press, key1, topic is %s, payload is \n%s \n",topic,json)
				;
	send_datastreams_to_queue(topic, json);	//publish topic
	//send_broadcast_data_to_queue(json); //broadcast in LAN
}


/******************************************************************************
 * FunctionName : user_link_led_init
 * Description  : int led gpio
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
LOCAL void  
user_link_led_init(void)
{
    PIN_FUNC_SELECT(WATCHMAN_LINK_LED_IO_MUX, WATCHMAN_LINK_LED_IO_FUNC);
}

LOCAL void  
user_link_led_timer_cb(void)
{
    link_led_level = (~link_led_level) & 0x01;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(WATCHMAN_LINK_LED_IO_NUM), link_led_level);
}

void  
user_link_led_timer_init(int time)
{
    os_timer_disarm(&link_led_timer);
    os_timer_setfn(&link_led_timer, (os_timer_func_t *)user_link_led_timer_cb, NULL);
    os_timer_arm(&link_led_timer, time, 1);
    link_led_level = 0;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(WATCHMAN_LINK_LED_IO_NUM), link_led_level);
}
/*
void  
user_link_led_timer_done(void)
{
    os_timer_disarm(&link_led_timer);

    GPIO_OUTPUT_SET(GPIO_ID_PIN(WATCHMAN_LINK_LED_IO_NUM), 1);
}
*/
/******************************************************************************
 * FunctionName : user_link_led_output
 * Description  : led flash mode
 * Parameters   : mode, on/off/xhz
 * Returns      : none
*******************************************************************************/
void  
user_link_led_output(uint8 mode)
{

    switch (mode) {
        case LED_OFF:
            os_timer_disarm(&link_led_timer);
            GPIO_OUTPUT_SET(GPIO_ID_PIN(WATCHMAN_LINK_LED_IO_NUM), 1);
            break;
    
        case LED_ON:
            os_timer_disarm(&link_led_timer);
            GPIO_OUTPUT_SET(GPIO_ID_PIN(WATCHMAN_LINK_LED_IO_NUM), 0);
            break;
    
        case LED_1HZ:
            user_link_led_timer_init(1000);
            break;
    
        case LED_5HZ:
            user_link_led_timer_init(200);
            break;

        case LED_20HZ:
            user_link_led_timer_init(50);
            break;

        default:
            printf("ERROR:LED MODE WRONG!\n");
            break;
    }
    
}

/******************************************************************************
 * FunctionName : user_get_key_status
 * Description  : a
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
BOOL  
user_get_key_status(void)
{
    return get_key_status(single_key[0]);
}

/******************************************************************************
 * FunctionName : user_watchman_init
 * Description  : init watchman's key function and relay output
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void  
user_watchman_init(void)
{
    printf("user_watchman_init start!\n");

    watchman = malloc(sizeof(Watchman));
    watchman->lightA = 0;
    watchman->lightB = 0;


    user_link_led_init();

    wifi_status_led_install(WATCHMAN_WIFI_LED_IO_NUM, WATCHMAN_WIFI_LED_IO_MUX, WATCHMAN_WIFI_LED_IO_FUNC);

    single_key[0] = key_init_single(WATCHMAN_KEY_0_IO_NUM, WATCHMAN_KEY_0_IO_MUX, WATCHMAN_KEY_0_IO_FUNC,
                                    user_watchman_long_press, user_watchman_short_press);

    single_key[1] = key_init_single(WATCHMAN_KEY_0_IO_NUM1, WATCHMAN_KEY_0_IO_MUX1, WATCHMAN_KEY_0_IO_FUNC1,
                                    NULL, user_watchman_short_press2);

    keys.key_num = WATCHMAN_KEY_NUM;
    keys.single_key = single_key;

    key_init(&keys);


//    spi_flash_erase_sector(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE);

    spi_flash_read((PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
    		(uint32 *)watchman, sizeof(Watchman));
    ESP_DBG("user_watchman_init,after spi_flash_read,lightA = %d, lightB = %d\n",watchman->lightA,watchman->lightB);

    PIN_FUNC_SELECT(WATCHMAN_RELAY_LED_IO_MUX, WATCHMAN_RELAY_LED_IO_FUNC);
    PIN_FUNC_SELECT(WATCHMAN_RELAY_LED_IO_MUX1, WATCHMAN_RELAY_LED_IO_FUNC1);



    WATCHMAN_STATUS_OUTPUT(WATCHMAN_RELAY_LED_IO_NUM, watchman->lightA);
    WATCHMAN_STATUS_OUTPUT(WATCHMAN_RELAY_LED_IO_NUM1,watchman->lightB);


    ESP_DBG("user_watchman_init end,lightA = %d, lightB = %d\n",watchman->lightA,watchman->lightB);

}
#endif

