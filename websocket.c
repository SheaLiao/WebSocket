/*********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<2928382441@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  websocket.c
 *    Description:  This file 
 *      reference:   https://github.com/mortzdk/websocket
 *
 *        Version:  1.0.0(2024年07月01日)
 *         Author:  Liao Shengli <2928382441@qq.com>
 *      ChangeLog:  1, Release initial version on "2024年07月01日 21时34分34秒"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <event2/util.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_struct.h>

#include "logger.h"
#include "wss.h"
#include "frame.h"
#include "sht20.h"
#include "led.h"


#define PORT 20000



static void sample_cb(evutil_socket_t fd, short events, void *arg)
{
#if 0
	struct bufferevent		*bev = (struct bufferevent *)arg;

	if (!bev)
	{
		log_error("Received NULL buffer event in sample_cb\n");
		return;
	}
#endif
	wss_session_t *session = (wss_session_t *)arg;

	if (!session || !session->send_bev) 
	{
		log_error("Invalid session or send_bev in sample_cb\n");
		return;
	}

	send_sample_data(session->send_bev);
}



static void heartbeat_cb(evutil_socket_t fd, short events, void *arg)
{
	wss_session_t *session = (wss_session_t *)arg;

	if (!session || !session->send_bev) 
	{
		log_error("Invalid session or send_bev in heartbeat_cb\n");
		return;
	}

	send_ping_frame(session->send_bev);
}


static void read_cb (struct bufferevent *bev, void *ctx)
{
	wss_session_t              *session = bev->cbarg;

	if( !session->handshaked )
	{
		do_wss_handshake(session);
		return ;
	}

	do_parser_frames(session);
	return ;
}



static void event_cb (struct bufferevent *bev, short events, void *ctx)
{
	wss_session_t              *session = bev->cbarg;

	if (events & BEV_EVENT_EOF) 
	{
		log_info("Client disconnected\n");
	} 
	else if (events & BEV_EVENT_ERROR) 
	{
		log_error("Got an error on the connection: %s\n", strerror(errno));
	}

	if( events & (BEV_EVENT_EOF|BEV_EVENT_ERROR) )
	{
		if( session )
		{
			log_warn("remote client %s closed\n", session->client);

			bufferevent_free(session->recv_bev);
			bufferevent_free(session->send_bev);
		}

	}

	return ;
}


static void accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *arg)
{
	wss_session_t                   *session = arg;
	struct bufferevent 				*recv_bev, *send_bev;
	struct event            		*sample_event = NULL;
	struct timeval       			tv={10, 0};

	struct event 					*heartbeat_event;
	struct timeval 					heartbeat_tv = {25, 0}; 

	struct sockaddr_in              *sock = (struct sockaddr_in *)addr;
	int								send_fd = -1;

	snprintf(session->client, sizeof(session->client), "[%d->%s:%d]", fd, inet_ntoa(sock->sin_addr), ntohs(sock->sin_port));
	log_info("accpet new socket client %s\n", session->client);


	recv_bev = bufferevent_socket_new(session->base, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
	if (!recv_bev) {
		log_error("create bufferevent for client %s failed\n", session->client);
		free(session);
		close(fd);
		return;
	}

	send_fd = dup(fd); 
	send_bev = bufferevent_socket_new(session->base, send_fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
	if (!send_bev) {
		log_error("create send bufferevent for client %s failed\n", session->client);
		bufferevent_free(recv_bev);
		free(session);
		close(send_fd);
		return;
	}

	session->recv_bev = recv_bev;
	session->send_bev = send_bev;

	bufferevent_setcb(recv_bev, read_cb, NULL, event_cb, session);
	bufferevent_enable(recv_bev, EV_READ | EV_WRITE);

	sample_event = event_new(session->base, -1, EV_PERSIST, sample_cb, session);
	if (!sample_event)
	{
		log_error("failed to create temp event\n");
		return;
	}
	event_add(sample_event, &tv);


	heartbeat_event = event_new(session->base, -1, EV_PERSIST, heartbeat_cb, session);
	if (!heartbeat_event)
	{
		log_error("failed to create heartbeat event\n");
		return;
	}
	event_add(heartbeat_event, &heartbeat_tv);


	return;
}



int main (int argc, char **argv)
{
	struct sockaddr_in      addr;
	int						len = sizeof(addr);

	struct event_base		*base = NULL;
	struct evconnlistener	*listener = NULL;

	char                    *logfile = "websocket.log";
	int                     loglevel = LOG_LEVEL_DEBUG;
	int                     logsize = 10;

	wss_session_t           *session;

	if( log_open("console", loglevel, logsize, THREAD_LOCK_NONE) < 0 )
	{
		fprintf(stderr, "initial log system failure\n");
		return -1;
	}

	init_gpio();

	if( !(session = malloc(sizeof(*session))) )
	{
		log_error("malloc for session failure:%s\n", strerror(errno));
		return -2;
	}

	memset(session, 0, sizeof(*session));

	base = event_base_new();
	if( !base )
	{
		log_error("event_base_new() failure\n");
		return -2;
	}

	session->base = base;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	listener = evconnlistener_new_bind(session->base, accept_cb, session, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr *)&addr, sizeof(addr));
	if( !listener )
	{
		log_error("Can't create a listener\n");
		event_base_free(session->base);
		return -3;
	}



	event_base_dispatch(session->base);
	evconnlistener_free(listener);
	event_base_free(session->base);

	return 0;
} 

