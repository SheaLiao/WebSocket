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
#include <event.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <arpa/inet.h>

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
	sock_ev->fd = -1;
	strncpy(sock_ev->servip, servip, 64);
	sock_ev->port = port;
	sock_ev->base = NULL;
	sock_ev->bev = NULL;

	return 0;
}


int socket_ev_close(socket_event_t *sock_ev)
{
	if( !sock_ev )
	{
		return -1;
	}
	
    if (sock_ev->bev)
    {
        bufferevent_free(sock_ev->bev);
        sock_ev->bev = NULL;
    }

	if (sock_ev->base)
    {
        event_base_free(sock_ev->base);
        sock_ev->base = NULL;
    }

    if (sock_ev->fd != -1)
    {
        close(sock_ev->fd);
        sock_ev->fd = -1;
    }

	return 0;
}



int socket_ev_connect(socket_event_t *sock_ev)
{
	int					rv = -1;
	int					fd = -1;
	struct sockaddr_in	addr;
	int					len = sizeof(addr);
	struct event_base 	*base = NULL;
	struct bufferevent 	*bev = NULL;

	if( !sock_ev )
	{
		return -1;
	}

	//socket_ev_close(sock_ev);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if( fd < 0 )
	{
		log_error("Creat socket failure:%s\n", strerror(errno));
		return -2;
	}
	log_info("Socket create fd[%d] successfully\n", fd);

	memset(&addr, 0, len);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(sock_ev->port);
	inet_aton(sock_ev->servip, &addr.sin_addr);


	base = event_base_new();
	if ( !base )
	{
		log_error("Could not initialize libevent!\n");
		close(fd);
		return -3;
	}

	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if( !bev )
	{
		log_error("Could not create bufferevent\n");
		event_base_free(base);
		close(fd);
		return -4;
	}

	rv = bufferevent_socket_connect(bev, (struct sockaddr *)&addr, len);
	if( rv < 0 )
	{
		if (errno == EINPROGRESS)
		{
			printf("connecting...\n");
		}
		else
		{
			log_error("event connect failure\n");
			bufferevent_free(bev);
			event_base_free(base);
			close(fd);
			return -5;
		}
	}
	else
	{
		log_info("event connect to server successfully\n");
		sock_ev->fd = fd;
		sock_ev->base = base;
		sock_ev->bev = bev;
	}

	return 0;
}



void event_callback(struct bufferevent *bev, short events, void *arg)
{
	struct timeval 	tv;
	socket_event_t 	*sock_ev = (socket_event_t *)arg;
	struct event 	*reconnect_ev;

	if( events & BEV_EVENT_CONNECTED )
	{
		log_info("connect to server successfully\n");
	}
	else if ( events & (BEV_EVENT_ERROR | BEV_EVENT_EOF) )
	{
		log_error("connect failure\n");
		tv.tv_sec = 5; 
		tv.tv_usec = 0;
		reconnect_ev = event_new(sock_ev->base, -1, EV_PERSIST, reconnect_cb, sock_ev->bev);
		if (reconnect_ev) 
		{
            event_add(reconnect_ev, &tv);
        } 
		else 
		{
            log_error("Could not create reconnect event\n");
            // 仅释放已分配的资源
            if (sock_ev->bev)
                bufferevent_free(sock_ev->bev);

            if (sock_ev->base)
                event_base_free(sock_ev->base);
            
			close(sock_ev->fd);
        }
	}
	return ;
}


void read_cb(struct bufferevent *bev, void *arg)
{
	log_info("read successfully\n");
}

#if 0
void write_cb(struct bufferevent *bev, void *arg)
{
	char 					buf[128] = {"Temperature:25.00"};
	int						len = sizeof(buf);
	socket_event_t 			*sock_ev = (socket_event_t *)arg;

	bufferevent_write(bev, buf, len);
	
	log_info("send successfully\n");
}
#endif

void reconnect_cb(evutil_socket_t fd, short events, void *arg)
{
	socket_event_t 		*sock_ev = (socket_event_t *)arg;
	int					rv = -1;

	log_info("Reconnecting...\n");

	socket_ev_close(sock_ev);


	rv = socket_ev_connect(sock_ev);
	if( rv < 0 )
	{
		log_error("Reconnection failed\n");
        return;
	}

	bufferevent_setcb(sock_ev->bev, read_cb, NULL, event_callback, sock_ev);
    bufferevent_enable(sock_ev->bev, EV_READ | EV_WRITE);
}

#if 1
void send_data(evutil_socket_t fd, short events, void *arg)
{
	socket_event_t 			*sock_ev = (socket_event_t *)arg;

	packet_t				pack;
	char					buf[1024];
	int						len = 0;

	sample_data(&pack);
	len = packet_data(&pack, buf, sizeof(buf));

	bufferevent_write(sock_ev->bev, buf, len);
}
#endif


void signal_cb(evutil_socket_t sig, short events, void *arg)
{
	socket_event_t	*sock_ev = (socket_event_t *)arg;
	struct timeval	delay = {2, 0};

	log_warn("Caught a signal, stop\n");
	event_base_loopexit(sock_ev->base, &delay);
}
