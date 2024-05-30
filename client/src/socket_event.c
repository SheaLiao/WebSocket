/*********************************************************************************
 *      Copyright:  (C) 2024 linuxer<linuxer@email.com>
 *                  All rights reserved.
 *
 *       Filename:  socket_event.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(10/05/24)
 *         Author:  Liao Shengli <2928382441@qq.com>
 *      ChangeLog:  1, Release initial version on "10/05/24 14:34:07"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <event.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <event2/bufferevent_ssl.h>


#include "socket_event.h"
#include "logger.h"
#include "packet.h"

int socket_ev_init(socket_event_t *sock_ev, char *servip, int port)
{
	if( !sock_ev || port <= 0 )
	{
		return -1;
	}

	memset(sock_ev, 0, sizeof(sock_ev));
	strncpy(sock_ev->servip, servip, 64);
	sock_ev->port = port;
	sock_ev->base = NULL;
	sock_ev->sslbev = NULL;
	sock_ev->ssl_ctx = NULL;
	sock_ev->ssl = NULL;

	return 0;
}


int socket_ev_close(socket_event_t *sock_ev)
{
	if( !sock_ev )
	{
		return -1;
	}

	if (sock_ev->sslbev)
	{
		bufferevent_free(sock_ev->sslbev);
		sock_ev->sslbev = NULL;
	}

	if (sock_ev->base)
	{
		event_base_free(sock_ev->base);
		sock_ev->base = NULL;
	}

	if( sock_ev->ssl )
    {
        SSL_shutdown(sock_ev->ssl);
        SSL_free(sock_ev->ssl);
        sock_ev->ssl = NULL;
    }


	if( sock_ev->ssl_ctx )
	{
		SSL_CTX_free(sock_ev->ssl_ctx);
		sock_ev->ssl_ctx = NULL;
	}

	return 0;
}



int socket_ev_connect(socket_event_t *sock_ev)
{
	int					rv = -1;
	struct sockaddr_in	addr;
	int					len = sizeof(addr);
	struct bufferevent 	*sslbev = NULL;

	if( !sock_ev )
	{
		return -1;
	}

	memset(&addr, 0, len);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(sock_ev->port);
	inet_aton(sock_ev->servip, &addr.sin_addr);

	sock_ev->sslbev = bufferevent_openssl_socket_new(sock_ev->base, -1, sock_ev->ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE);
	if( !sock_ev->sslbev )
	{
		log_error("Could not create ssl bufferevent\n");
		event_base_free(sock_ev->base);
		return -4;
	}

	bufferevent_setcb(sock_ev->sslbev, read_cb, NULL, event_callback, sock_ev);  // 添加事件回调
	bufferevent_enable(sock_ev->sslbev, EV_READ | EV_WRITE);

	rv = bufferevent_socket_connect(sock_ev->sslbev, (struct sockaddr *)&addr, len);
	if( rv < 0 )
	{
		// 如果连接失败，设定一个定时事件来尝试重连
		struct timeval reconnect_time = {DELAY_TIME, 0};
		struct event *reconnect_event = evtimer_new(sock_ev->base, reconnect_cb, sock_ev);
		evtimer_add(reconnect_event, &reconnect_time);
	}
	log_info("event reconnect to server successfully\n");

	return 0;
}



void event_callback(struct bufferevent *bev, short events, void *arg)
{
	int				rv = -1;
	struct timeval 	tv;
	socket_event_t 	*sock_ev = (socket_event_t *)arg;

	if( events & BEV_EVENT_CONNECTED )
	{
		log_info("Connect to server successfully\n");
		return ;
	}
	else if( events & BEV_EVENT_EOF )
	{
		log_warn("Connection closed\n");
	}
	else if( events & BEV_EVENT_ERROR )
	{
		log_error("Connection error\n");
	}

	struct timeval reconnect_time = {DELAY_TIME, 0};
	struct event *reconnect_event = evtimer_new(sock_ev->base, reconnect_cb, sock_ev);
	evtimer_add(reconnect_event, &reconnect_time);
}


void read_cb(struct bufferevent *bev, void *arg)
{
	log_info("read successfully\n");
}


void reconnect_cb(evutil_socket_t fd, short events, void *arg)
{
	socket_event_t 		*sock_ev = (socket_event_t *)arg;

	socket_ev_connect(sock_ev);
}



void send_data(evutil_socket_t fd, short events, void *arg)
{
	socket_event_t 			*sock_ev = (socket_event_t *)arg;

	packet_t				pack;
	char					buf[1024];
	int						len = 0;

	sample_data(&pack);
	len = packet_data(&pack, buf, sizeof(buf));
	log_info("%s\n", buf);

	bufferevent_write(sock_ev->sslbev, buf, len);
}


void signal_cb(evutil_socket_t sig, short events, void *arg)
{
	socket_event_t	*sock_ev = (socket_event_t *)arg;
	struct timeval	delay = {DELAY_TIME, 0};

	log_warn("Caught a signal, stop\n");
	event_base_loopexit(sock_ev->base, &delay);
}
