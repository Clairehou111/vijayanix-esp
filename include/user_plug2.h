#ifndef __USER_ESPSWITCH_H__
#define __USER_ESPSWITCH_H__

#include "driver/key.h"
#include "c_types.h"

/* NOTICE---this is for 512KB spi flash.
 * you can change to other sector if you use other size spi flash. */
#define PRIV_PARAM_START_SEC        0x7C

#define PRIV_PARAM_SAVE     0

#define PLUG_KEY_NUM            2

#define PLUG_KEY_0_IO_MUX     PERIPHS_IO_MUX_MTCK_U
#define PLUG_KEY_0_IO_NUM     13
#define PLUG_KEY_0_IO_FUNC    FUNC_GPIO13

#define PLUG_WIFI_LED_IO_MUX     PERIPHS_IO_MUX_GPIO0_U
#define PLUG_WIFI_LED_IO_NUM     0
#define PLUG_WIFI_LED_IO_FUNC    FUNC_GPIO0

#define PLUG_LINK_LED_IO_MUX     PERIPHS_IO_MUX_MTDI_U
#define PLUG_LINK_LED_IO_NUM     12
#define PLUG_LINK_LED_IO_FUNC    FUNC_GPIO12

#define PLUG_RELAY_LED_IO_MUX     PERIPHS_IO_MUX_MTDO_U
#define PLUG_RELAY_LED_IO_NUM     15
#define PLUG_RELAY_LED_IO_FUNC    FUNC_GPIO15


#define PLUG_KEY_0_IO_MUX1     PERIPHS_IO_MUX_GPIO4_U
#define PLUG_KEY_0_IO_NUM1     4
#define PLUG_KEY_0_IO_FUNC1    FUNC_GPIO4


#define PLUG_RELAY_LED_IO_MUX1     PERIPHS_IO_MUX_GPIO5_U
#define PLUG_RELAY_LED_IO_NUM1     5
#define PLUG_RELAY_LED_IO_FUNC1    FUNC_GPIO5


#define PLUG_NUM					2


#define PLUG_STATUS_OUTPUT(pin, on)     GPIO_OUTPUT_SET(pin, on)

enum {
    LED_OFF = 0,
    LED_ON  = 1,
    LED_1HZ,
    LED_5HZ,
    LED_10HZ,
    LED_20HZ,
};




struct plug_saved_param {
    uint16_t status;
    uint8 pad[3];
};



void user_plug_init2(void);
uint16 user_plug_get_status2(void);
void user_plug_set_status2(uint16 status);
BOOL user_get_key_status(void);
void user_link_led_output(uint8);


#endif

