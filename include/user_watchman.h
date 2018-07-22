#ifndef __USER_ESPSWITCH_H__
#define __USER_ESPSWITCH_H__

#include "driver/key.h"

/* NOTICE---this is for 512KB spi flash.
 * you can change to other sector if you use other size spi flash. */
#define PRIV_PARAM_START_SEC        0x7C

#define PRIV_PARAM_SAVE     0

#define WATCHMAN_KEY_NUM            2

#define WATCHMAN_KEY_0_IO_MUX     PERIPHS_IO_MUX_MTCK_U
#define WATCHMAN_KEY_0_IO_NUM     13
#define WATCHMAN_KEY_0_IO_FUNC    FUNC_GPIO13

#define WATCHMAN_WIFI_LED_IO_MUX     PERIPHS_IO_MUX_GPIO0_U
#define WATCHMAN_WIFI_LED_IO_NUM     0
#define WATCHMAN_WIFI_LED_IO_FUNC    FUNC_GPIO0

#define WATCHMAN_LINK_LED_IO_MUX     PERIPHS_IO_MUX_MTDI_U
#define WATCHMAN_LINK_LED_IO_NUM     12
#define WATCHMAN_LINK_LED_IO_FUNC    FUNC_GPIO12

#define WATCHMAN_RELAY_LED_IO_MUX     PERIPHS_IO_MUX_MTDO_U
#define WATCHMAN_RELAY_LED_IO_NUM     15
#define WATCHMAN_RELAY_LED_IO_FUNC    FUNC_GPIO15


#define WATCHMAN_KEY_0_IO_MUX1     PERIPHS_IO_MUX_GPIO4_U
#define WATCHMAN_KEY_0_IO_NUM1     4
#define WATCHMAN_KEY_0_IO_FUNC1    FUNC_GPIO4


#define WATCHMAN_RELAY_LED_IO_MUX1     PERIPHS_IO_MUX_GPIO5_U
#define WATCHMAN_RELAY_LED_IO_NUM1     5
#define WATCHMAN_RELAY_LED_IO_FUNC1    FUNC_GPIO5


#define LIGHTA_NAME						"lightA"
#define LIGHTB_NAME					    "lightB"



#define WATCHMAN_STATUS_OUTPUT(pin, on)     GPIO_OUTPUT_SET(pin, on)

enum {
    LED_OFF = 0,
    LED_ON  = 1,
    LED_1HZ,
    LED_5HZ,
    LED_10HZ,
    LED_20HZ,
};




typedef struct watchman_saved_param {
	bool lightA;
	bool lightB;
} Watchman;

#define INVALIDVALUE 0xFF

void user_watchman_init(void);
Watchman* user_watchman_get_status(void);
void user_watchman_set_lightA(bool lightA);
void user_watchman_set_lightB(bool lightB);
void user_watchman_set_status(bool lightA ,bool lightB);
BOOL user_get_key_status(void);
void user_link_led_output(uint8);


#endif

