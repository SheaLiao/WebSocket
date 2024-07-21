/*
 * esp8266.c
 *
 *  Created on: Jul 20, 2024
 *      Author: asus
 */


#include <stdlib.h>
#include "usart.h"
#include "esp8266.h"

#define CONFIG_DEBUG
#ifdef CONFIG_DEBUG
#define dbg_print(format, args...) printf(format,##args)
#else
#define dbg_print(format, args...) do{} while(0)
#endif


int send_atcmd(char *atcmd, char *expect_reply, unsigned int timeout)
{
	int				rv = 1;
	unsigned int	i;
	char			*expect;

	if( !atcmd || strlen(atcmd)<=0 )
	{
		dbg_print("ERROR: Invalid input arguments\r\n");
		return -1;
	}

	dbg_print("\r\nStart send AT command: %s", atcmd);
	clear_atcmd_buf();
	HAL_UART_Transmit(wifi_huart, (uint8_t *)atcmd, strlen(atcmd), 1000);

	expect = expect_reply ? expect_reply : "OK\r\n";

	for(i=0;i<timeout;i++)
	{
		if( strstr(g_wifi_rxbuf, expect) )
		{
			dbg_print("AT command got expect reply '%s'\r\n", expect);
			rv = 0;
			goto CleanUp;
		}

		if( strstr(g_wifi_rxbuf, "ERROR\r\n") || strstr(g_wifi_rxbuf, "FAIL\r\n") )
		{
			rv = 2;
			goto CleanUp;
		}

		HAL_Delay(1);
	}

CleanUp:
	dbg_print("<<<< AT command reply:\r\n%s", g_wifi_rxbuf);
	return rv;
}



int atcmd_send_data(unsigned char *data,int bytes , unsigned int timeout)
{
	int				rv = -1;
	unsigned int	i;

	if( !data || bytes <= 0 )
	{
		dbg_print("ERROR: Invalid input arguments\r\n");
		return -1;
	}

	dbg_print("\r\nStart send AT command send [%d] bytes data\n", bytes);
	clear_atcmd_buf();
	HAL_UART_Transmit(wifi_huart, data, bytes, 1000);

	for(i=0;i<timeout;i++)
	{
		if( strstr(g_wifi_rxbuf, "SEND OK\r\n") )
		{
			rv = 0;
			goto CleanUp;
		}

		if( strstr(g_wifi_rxbuf, "ERROR\r\n") )
		{
			rv = 1;
			goto CleanUp;
		}

		HAL_Delay(1);
	}

CleanUp:
	dbg_print("<<<< AT command reply:\r\n%s", g_wifi_rxbuf);
	return rv;

}


int esp8266_module_init(void)
{
	int		i;

	dbg_print("INFO: Reset ESP8266 module now...\r\n");
	send_atcmd("AT+RST\r\n", EXPECT_OK, 500);

	for(i=0;i<6;i++)
	{
		if( !send_atcmd("AT\r\n", EXPECT_OK, 500) )
		{
			dbg_print("INFO: Send AT to ESP8266 and got reply ok\r\n");
			break;
		}

		HAL_Delay(100);
	}

	if( i>=6 )
	{
		dbg_print("ERROR: Can't AT reply after reset\r\n");
		return -2;
	}

	if( send_atcmd("AT+CWMODE_CUR=1\r\n", EXPECT_OK, 500) )
	{
		dbg_print("ERROR: Set ESP8266 work as Station mode failure\r\n");
		return -3;
	}

	if( send_atcmd("AT+CWDHCP_CUR=1,1\r\n", EXPECT_OK, 500) )
	{
		dbg_print("ERROR: Enable ESP8266 Station mode failure\r\n");
		return -4;
	}

#if 0

	if( send_atcmd("AT+GMR\r\n", EXPECT_OK, 500) )
	{
		dbg_print("ERROR: AT+GMR check ESP8266 reversion failure\r\n");
		return -5;
	}
#endif

	HAL_Delay(500);
	return 0;
}


int esp8266_join_network(char *ssid, char *pwd)
{
	char		atcmd[128] = {0x00};
	int			i;

	if( !ssid || !pwd )
	{
		dbg_print("ERROR: Invalid input arguments\r\n");
		return -1;
	}

	snprintf(atcmd, sizeof(atcmd), "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", ssid, pwd);
	if( send_atcmd(atcmd, "CONNECTED", 10000) )
	{
		dbg_print("ERROR: ESP8266 connect to '%s' failure\r\n", ssid);
		return -2 ;
	}
	dbg_print("INFO: ESP8266 connect to '%s' ok\r\n", ssid);

	for(i=0;i<10;i++)
	{
		if( !send_atcmd("AT+CIPSTA_CUR?\r\n", "255.", 1000) )
		{
			dbg_print("INFO: ESP8266 got ipaddress ok\r\n");
			return 0 ;
		}

		HAL_Delay(300);
	}

	dbg_print("INFO: ESP8266 got ipaddress failure\r\n");
	return -3 ;
}



static int util_parser_ipaddr(char *buf, char *key, char *ipaddr, int size)
{
	char		*start;
	char		*end;
	int			len;

	if( !buf || !key || !ipaddr )
	{
		return -1;
	}

	start = strstr(buf, key);
	if( !start )
	{
		return  -2;
	}

	start+=strlen(key) + 1;
	end =strchr(start,'"');
	if( !end )
	{
		return -2;
	}

	len = end - start;
	len = len > size ? size : len;

	memset(ipaddr, 0, size);
	strncpy(ipaddr, start, len);

	return 0;
}


int esp8266_get_ipaddr(char *ipaddr, char *gateway, int ipaddr_size)
{
	if( !ipaddr || !ipaddr || ipaddr_size<7 )
	{
		dbg_print("ERROR: Invalid input arguments\r\n");
		return -1;
	}

	if( send_atcmd("AT+CIPSTA_CUR?\r\n", "255.", 1000) )
	{
		dbg_print("INFO: ESP8266 AT+CIPSTA_CUR? command failure\r\n");
		return -2 ;
	}

	if( util_parser_ipaddr(g_wifi_rxbuf, "ip:", ipaddr, ipaddr_size) )
	{
		dbg_print("ERROR: ESP8266 AT+CIPSTA_CUR? parser IP failure\r\n");
		return -3 ;
	}

	if( util_parser_ipaddr(g_wifi_rxbuf, "gateway:", gateway, ipaddr_size) )
	{
		dbg_print("ERROR: ESP8266 AT+CIPSTA_CUR? parser gateway failure\r\n");
		return -4 ;
	}

	dbg_print("INFO: ESP8266 got IP address[%s] gateway[%s] ok\r\n", ipaddr, gateway);
	return 0;
}


int esp8266_ping_test(char *host)
{
	char		atcmd[128] = {0x00};

	if( !host )
	{
		dbg_print("ERROR: Invalid input arguments\r\n");
		return -1;
	}

	snprintf(atcmd, sizeof(atcmd), "AT+PING=\"%s\"\r\n", host);
	if( send_atcmd(atcmd, EXPECT_OK, 3000) )
	{
		dbg_print("ERROR: ESP8266 ping test [%s] failure\r\n", host);
		return -2 ;
	}

	dbg_print("INFO: ESP8266 ping test [%s] ok\r\n", host);
	return 0;
}



int esp8266_sock_connect(char *servip, int port)
{
	char		atcmd[128] = {0x00};

	if( !servip || port<=0 )
	{
		dbg_print("ERROR: Invalid input arguments\r\n");
		return -1;
	}

	send_atcmd("AT+CIPMUX=0\r\n", EXPECT_OK, 1500);

	snprintf(atcmd, sizeof(atcmd), "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", servip, port);
	if( send_atcmd(atcmd, "CONNECT\r\n", 1000) )
	{
		dbg_print("ERROR: ESP8266 connect to [%s:%d] failure\r\n", servip, port);
		return -2 ;
	}

	dbg_print("INFO: ESP8266 connect to [%s:%d] ok\r\n", servip, port);
	return 0;
}



int esp8266_sock_disconnect(void)
{
	send_atcmd("AT+CIPCLOSE\r\n", EXPECT_OK, 1500);
	return 0;
}



int esp8266_sock_send(unsigned char *data, int bytes)
{
	char		atcmd[128] = {0x00};

	if( !data || bytes<=0 )
	{
		dbg_print("ERROR: Invalid input arguments\r\n");
		return -1;
	}

	snprintf(atcmd, sizeof(atcmd), "AT+CIPSEND=%d\r\n", bytes);
	if( send_atcmd(atcmd, ">", 500) )
	{
		dbg_print("ERROR: AT+CIPSEND command failure\r\n");
		return -2 ;
	}

	if( atcmd_send_data((unsigned char *)data, bytes, 1000) )
	{
		dbg_print("ERROR: AT+CIPSEND send data failure\r\n");
		return -3 ;
	}

	return bytes;
}


int esp8266_sock_recv(unsigned char *buf, int size)
{
	char		*data = NULL;
	char		*ptr = NULL;

	int			len;
	int			rv;
	int			bytes;

	if( !buf || size<=0 )
	{
		dbg_print("ERROR: Invalid input arguments\r\n");
		return -1;
	}

	if( g_wifi_rxbuf <= 0 )
	{
		return 0;
	}

	if( !(ptr=strstr(g_wifi_rxbuf, "+IPD,")) || !(data=strchr(g_wifi_rxbuf, ':')) )
	{
		return 0;
	}

	data++;
	bytes = atoi(ptr+strlen("+IPD,"));

	len = g_wifi_rxbytes - (data-g_uart2_rxbuf);

	if( len <bytes )
	{
		dbg_print("+IPD data not receive over, receive again later ...\r\n");
		return 0;
	}

	memset(buf, 0, size);
	rv = bytes>size ? size : bytes;
	memcpy(buf, data, rv);

	clear_atcmd_buf();

	return rv;
}



















