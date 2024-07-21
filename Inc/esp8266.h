/*
 * esp8266.h
 *
 *  Created on: Jul 20, 2024
 *      Author: asus
 */

#ifndef INC_ESP8266_H_
#define INC_ESP8266_H_

#include <stdio.h>

#define wifi_huart		&huart2
#define g_wifi_rxbuf	g_uart2_rxbuf
#define g_wifi_rxbytes	g_uart2_bytes


#define clear_atcmd_buf()	do {memset(g_wifi_rxbuf,0,sizeof(g_wifi_rxbuf));\
								g_wifi_rxbytes=0;} while(0)

#define EXPECT_OK	"OK\r\n"
extern int send_atcmd(char *atcmd, char *expect_reply, unsigned int timeout);

extern int esp8266_module_init(void);

extern int esp8266_module_reset(void);

extern int esp8266_join_network(char *ssid, char *pwd);

int esp8266_get_ipaddr(char *ipaddr, char *gateway, int ipaddr_size);

int esp8266_ping_test(char *host);

extern int esp8266_sock_connect(char *servip, int port);

extern int esp8266_sock_disconnect(void);

extern int esp8266_sock_send(unsigned char *data, int bytes);

extern int esp8266_sock_recv(unsigned char *buf, int size);

#endif /* INC_ESP8266_H_ */
