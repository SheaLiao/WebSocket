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



int atcmd_send_data(unsigned char *data, int bytes , unsigned int timeout)
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



int esp8266_connect_wifi(void)
{
    char ipaddr[16];
    char gateway[16];
    int status = 0;

    // 初始化ESP8266模块
    if (esp8266_module_init() != 0)
    {
    	dbg_print("ESP8266 init failure\r\n");
        return -1;
    }

    // 连接到指定的WiFi网络
    if (esp8266_join_network(ROUTER, PASSWORD) != 0)
    {
    	dbg_print("join WiFi failure\r\n");
        return -1;
    }

    // 获取IP地址和网关
    if (esp8266_get_ipaddr(ipaddr, gateway, sizeof(ipaddr)) != 0)
    {
    	dbg_print("get IP failure\r\n");
        return -1;
    }
    printf("connect successfully，IP: %s, gateway: %s\r\n", ipaddr, gateway);

    // Ping测试
    if (esp8266_ping_test("192.168.10.104") != 0)
    {
    	dbg_print("Ping failure\r\n");
        return -1;
    }

    // 设置成功标志
    status |= FLAG_WIFI_CONNECTED;
    dbg_print("WiFi connect successfully\r\n");

    return status;
}



int esp8266_setup_server(int port)
{
    char atcmd[128] = {0x00};

    if (send_atcmd("AT+CIPMUX=1\r\n", "OK", 1500))
    {
    	dbg_print("ERROR: Set ESP8266 to multi-connection mode failed\r\n");
        return -1;
    }

	snprintf(atcmd, sizeof(atcmd), "AT+CIPSERVER=1,%d\r\n", port);
	if (send_atcmd(atcmd, "OK", 1500))
	{
		dbg_print("ERROR: Start ESP8266 server failed\r\n");
		return -2;
	}

	dbg_print("INFO: ESP8266 server started on port %d\r\n", port);
	return 0;
}


int esp8266_sock_send(unsigned char *data, int id, int bytes)
{
	char		atcmd[128] = {0x00};

	if( !data || bytes<=0 )
	{
		dbg_print("ERROR: Invalid input arguments\r\n");
		return -1;
	}

	snprintf(atcmd, sizeof(atcmd), "AT+CIPSEND=%d,%d\r\n", id, bytes);
	if( send_atcmd(atcmd, ">", 3000) )
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


int check_client_connection(int *link_id)
{
    int client_connected = 0;

    printf("buf: %s\r\n", g_uart2_rxbuf);
    if (g_uart2_bytes > 0)
    {
        // 使用 strstr 检查是否有新连接的指示消息
        char *connect_str = strstr(g_uart2_rxbuf, ",CONNECT");
        if (connect_str != NULL)
        {
            // 获取连接ID
            char *id_str = strtok(g_uart2_rxbuf, ",");
            if (id_str != NULL)
            {
                *link_id = atoi(id_str);
                printf("Client connected on link %d\r\n", *link_id);
                client_connected = 1; // 设置标志位，表示有新连接
            }
        }
        else
        {
            printf("No new connection found\r\n");
        }
    }
    else
    {
        printf("No data in buffer\r\n");
    }

    // 如果检测到新连接，则清空缓冲区
    if (client_connected)
    {
        g_uart2_bytes = 0; // 重置缓冲区计数器
        memset(g_uart2_rxbuf, 0, sizeof(g_uart2_rxbuf)); // 清空缓冲区内容
    }

    return client_connected; // 返回连接状态
}






