/*
 * httpdserver.h
 *
 *  Created on: 2018Äê6ÔÂ13ÈÕ
 *      Author: hxhoua
 */

#ifndef INCLUDE_HTTPDSERVER_H_
#define INCLUDE_HTTPDSERVER_H_

#if HTTPD_SERVER
#include "espfsformat.h"
#include "espfs.h"
#include "captdns.h"
#include "httpd.h"
#include "cgiwifi.h"
#include "httpdespfs.h"
#include "user_cgi.h"
#include "webpages-espfs.h"

#ifdef ESPFS_HEATSHRINK
#include "heatshrink_config_custom.h"
#include "heatshrink_decoder.h"
#endif


HttpdBuiltInUrl builtInUrls[]={
	{"*", cgiRedirectApClientToHostname, "esp.nonet"},
	{"/", cgiRedirect, "/index.html"},
	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
	{"/wifi/wifi.tpl", cgiEspFsTemplate, tplWlan},
	{"/wifi/connect.cgi", cgiWiFiConnect, NULL},
	{"/wifi/connstatus.cgi", cgiWiFiConnStatus, NULL},
	{"/wifi/setmode.cgi", cgiWiFiSetMode, NULL},

	{"/config", cgiEspApi, NULL},
	{"/client", cgiEspApi, NULL},
	{"/upgrade", cgiEspApi, NULL},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL} //end marker
};

#endif
#endif /* INCLUDE_HTTPDSERVER_H_ */
