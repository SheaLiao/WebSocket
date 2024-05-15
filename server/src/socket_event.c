/*********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<2928383441@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  socket_event.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(14/05/24)
 *         Author:  Liao Shengli <2928383441@qq.com>
 *      ChangeLog:  1, Release initial version on "14/05/24 18:52:32"
 *                 
 ********************************************************************************/

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#include "socket_event.h"
#include "logger.h"


int socket_ev_init(socket_event_t *sock_ev, int port)
{
	if( !sock_ev || port <= 0 )
	{
		return -1;
	}

	memset(sock_ev, 0, sizeof(sock_ev));
	sock_ev->fd = -1;
	sock_ev->port = port;
	sock_ev->base = NULL;
	sock_ev->listener = NULL;
	sock_ev->sig = NULL;

	return 0;
}


int socket_ev_close(socket_event_t *sock_ev)
{
	if( !sock_ev )
	{
		return -1;
	}

	if( sock_ev->listener )
	{
		evconnlistener_free(sock_ev->listener);
		sock_ev->listener = NULL;
	}

	if( sock_ev->sig )
	{
		event_free(sock_ev->sig);
		sock_ev->sig = NULL;
	}

	if( sock_ev->base )
	{
		event_base_free(sock_ev->base);
		sock_ev->base = NULL;
	}

	return 0;
}



void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *arg)
{
	socket_event_t			*sock_ev = (socket_event_t *)arg;
	struct bufferevent		*bev = NULL;


	bev = bufferevent_socket_new(sock_ev->base, fd, BEV_OPT_CLOSE_ON_FREE);
	if( !bev )
	{
		log_error("bufferevent_socket_new() failure:%s\n", strerror(errno));
		close(fd);
		return ;
	}

	bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}


void read_cb(struct bufferevent *bev, void *arg)
{
	char		buf[1024];
	int			len;

	while( (len = bufferevent_read(bev, buf, sizeof(buf))) > 0 )
	{
		buf[len] = '\0';
		log_info("data:%s\n", buf);
		bufferevent_write(bev, buf, len);
	}
}


void event_cb(struct bufferevent *bev, short events, void *arg)
{
	if( events & BEV_EVENT_EOF )
	{
		log_warn("Connection closed\n");
	}
	else if( events & BEV_EVENT_ERROR )
	{
		log_error("Connection error:%s\n", strerror(errno));
	}
	
	bufferevent_free(bev);
}


void signal_cb(evutil_socket_t sig, short events, void *arg)
{
	socket_event_t	*sock_ev = (socket_event_t *)arg;
	struct timeval	delay = {2, 0};

	log_warn("Caught a signal, stop\n");

	event_base_loopexit(sock_ev->base, &delay);
}
