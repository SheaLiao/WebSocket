/*
 * esp8266.h
 *
 *  Created on: Jul 20, 2024
 *      Author: asus
 */

#ifndef INC_ESP8266_H_
#define INC_ESP8266_H_

#include <stdio.h>


#define ROUTER		"LingYun_XiaoMi"
#define PASSWORD	"lingyun_emb"

#define FLAG_WIFI_CONNECTED		(1<<0)
#define FLAG_SOCK_CONNECTED		(1<<1)
#define FLAG_CLIENT_CONNECTED 	0x04



#define wifi_huart		&huart2


#define clear_atcmd_buf() do { \
    ring_buffer_init(&g_uart2_ringbuf); \
} while (0)


#define EXPECT_OK	"OK\r\n"

void send_raw(const char *data, int len);

extern int send_atcmd(char *atcmd, char *expect_reply, unsigned int timeout);

extern int esp8266_module_init(void);

int esp8266_module_reset(void);

int esp8266_join_network(char *ssid, char *pwd);

int esp8266_get_ipaddr(char *ipaddr, char *gateway, int ipaddr_size);

int esp8266_ping_test(char *host);

extern int esp8266_connect_wifi(void);

extern int esp8266_setup_server(int port);

extern int esp8266_sock_send(unsigned char *data, int id, int bytes);

extern int esp8266_sock_recv(unsigned char *buf, int size);

extern int check_client_connection(int *link_id);


#endif /* INC_ESP8266_H_ */
