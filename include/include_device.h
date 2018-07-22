/*
 * device_header.h
 *
 *  Created on: 2018Äê7ÔÂ15ÈÕ
 *      Author: hxhoua
 */

#ifndef DEVICE_HEADER_H_
#define DEVICE_HEADER_H_

#include "user_config.h"

#if PLUG_DEVICE
#include "user_plug.h"
#endif

#if PLUG_DEVICE2
#include "user_plug2.h"
#endif

#if WATCHMAN_DEVICE
#include "user_watchman.h"
#endif

#endif /* DEVICE_HEADER_H_ */
