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
    int rv = 1;
    unsigned int i;
    char *expect;
    uint8_t rx_byte;
    uint32_t start_time;
    char temp_buf[256]; // 临时缓冲区，用于存储从环形缓冲区读取的数据

    if (!atcmd || strlen(atcmd) <= 0)
    {
        dbg_print("ERROR: Invalid input arguments\r\n");
        return -1;
    }

    dbg_print("\r\nStart send AT command: %s", atcmd);
    clear_atcmd_buf();  // 清除环形缓冲区
    HAL_UART_Transmit(wifi_huart, (uint8_t *)atcmd, strlen(atcmd), 1000);

    expect = expect_reply ? expect_reply : "OK\r\n";

    start_time = HAL_GetTick();  // 获取当前时间

    while (1)
    {
        // 从环形缓冲区读取数据
        while (ring_buffer_read(&g_uart2_ringbuf, &rx_byte) == 0)
        {
            // 将读取的数据存入临时缓冲区
            strncat(temp_buf, (char *)&rx_byte, 1);
        }

        // 判断是否包含期望的回复
        if (strstr(temp_buf, expect))
        {
            dbg_print("AT command got expect reply '%s'\r\n", expect);
            rv = 0;
            goto CleanUp;
        }

        // 判断是否出现错误
        if (strstr(temp_buf, "ERROR\r\n") || strstr(temp_buf, "FAIL\r\n"))
        {
            rv = 2;
            goto CleanUp;
        }

        // 超时检测
        if ((HAL_GetTick() - start_time) >= timeout)
        {
            break;
        }

        HAL_Delay(1);
    }

CleanUp:
    dbg_print("<<<< AT command reply:\r\n%s", temp_buf);
    return rv;
}




int atcmd_send_data(unsigned char *data, int bytes, unsigned int timeout)
{
    int rv = -1;
    unsigned int i;
    uint8_t rx_byte;
    uint32_t start_time;
    char temp_buf[256]; // 临时缓冲区，用于存储从环形缓冲区读取的数据

    if (!data || bytes <= 0)
    {
        dbg_print("ERROR: Invalid input arguments\r\n");
        return -1;
    }

    dbg_print("\r\nStart send AT command send [%d] bytes data\n", bytes);
    clear_atcmd_buf();  // 清除环形缓冲区
    HAL_UART_Transmit(wifi_huart, data, bytes, 1000);

    start_time = HAL_GetTick();  // 获取当前时间

    while (1)
    {
        // 从环形缓冲区读取数据
        while (ring_buffer_read(&g_uart2_ringbuf, &rx_byte) == 0)
        {
            // 将读取的数据存入临时缓冲区
            strncat(temp_buf, (char *)&rx_byte, 1);
        }

        // 判断是否收到成功的回复
        if (strstr(temp_buf, "SEND OK\r\n"))
        {
            rv = 0;
            goto CleanUp;
        }

        // 判断是否出现错误
        if (strstr(temp_buf, "ERROR\r\n"))
        {
            rv = 1;
            goto CleanUp;
        }

        // 超时检测
        if ((HAL_GetTick() - start_time) >= timeout)
        {
            break;
        }

        HAL_Delay(1);
    }

CleanUp:
    dbg_print("<<<< AT command reply:\r\n%s", temp_buf);
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
    if (!ipaddr || !gateway || ipaddr_size < 7)
    {
        dbg_print("ERROR: Invalid input arguments\r\n");
        return -1;
    }

    if (send_atcmd("AT+CIPSTA_CUR?\r\n", "255.", 1000))
    {
        dbg_print("INFO: ESP8266 AT+CIPSTA_CUR? command failure\r\n");
        return -2;
    }

    // 从环形缓冲区读取数据
    char buffer[UART2_BUFFER_SIZE] = {0};
    int index = 0;
    uint8_t byte;

    while (uart2_ring_buffer_read(&byte) == 0)
    {
        buffer[index++] = byte;
        if (index >= sizeof(buffer) - 1)
        {
            break;
        }
    }
    buffer[index] = '\0';  // 确保缓冲区是以 null 结尾

    if (util_parser_ipaddr(buffer, "ip:", ipaddr, ipaddr_size))
    {
        dbg_print("ERROR: ESP8266 AT+CIPSTA_CUR? parser IP failure\r\n");
        return -3;
    }

    if (util_parser_ipaddr(buffer, "gateway:", gateway, ipaddr_size))
    {
        dbg_print("ERROR: ESP8266 AT+CIPSTA_CUR? parser gateway failure\r\n");
        return -4;
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


int esp8266_sock_recv(unsigned char *buf, int size)
{
    char *data = NULL;
    char *ptr = NULL;

    int len;
    int rv;
    int bytes;
    uint8_t byte;
    int index = 0;

    if (!buf || size <= 0)
    {
        dbg_print("ERROR: Invalid input arguments\r\n");
        return -1;
    }

    // 从环形缓冲区读取数据
    char buffer[UART2_BUFFER_SIZE] = {0};
    while (uart2_ring_buffer_read(&byte) == 0)
    {
        buffer[index++] = byte;
        if (index >= sizeof(buffer) - 1)
        {
            break;
        }
    }
    buffer[index] = '\0';  // 确保缓冲区是以 null 结尾

    if (!(ptr = strstr(buffer, "+IPD,")) || !(data = strchr(buffer, ':')))
    {
        return 0;
    }

    data++;
    bytes = atoi(ptr + strlen("+IPD,"));

    len = index - (data - buffer);

    if (len < bytes)
    {
        dbg_print("+IPD data not receive over, receive again later ...\r\n");
        return 0;
    }

    memset(buf, 0, size);
    rv = bytes > size ? size : bytes;
    memcpy(buf, data, rv);

    // 清空环形缓冲区
    ring_buffer_init(&g_uart2_ringbuf);

    return rv;
}



int check_client_connection(int *link_id)
{
    int client_connected = 0;
    char buffer[UART2_BUFFER_SIZE] = {0};
    int index = 0;
    uint8_t byte;

    // 从环形缓冲区读取数据
    while (uart2_ring_buffer_read(&byte) == 0)
    {
        buffer[index++] = byte;
        if (index >= sizeof(buffer) - 1)
        {
            break;
        }
    }
    buffer[index] = '\0';  // 确保缓冲区是以 null 结尾

    // 使用 strstr 检查是否有新连接的指示消息
    char *connect_str = strstr(buffer, ",CONNECT");
    if (connect_str != NULL)
    {
        // 获取连接ID
        char *id_str = strtok(buffer, ",");
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

    // 如果检测到新连接，则清空环形缓冲区
    if (client_connected)
    {
        ring_buffer_init(&g_uart2_ringbuf); // 重新初始化环形缓冲区
    }

    return client_connected; // 返回连接状态
}






