/*
 * socket_bs.h
 *
 *  Created on: 2018��6��10��
 *      Author: hxhoua
 */

#ifndef INCLUDE_SOCKET_BS_H_
#define INCLUDE_SOCKET_BS_H_


int getpeermac( int sockfd, char *buf );
int getpeermac_by_ip(char *ipaddr, char* buf);

#endif /* INCLUDE_SOCKET_BS_H_ */
