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
#include "temp.h"

#define PORT  11111



static void temp_cb(evutil_socket_t fd, short events, void *arg)
{
	 struct bufferevent		*bev = (struct bufferevent *)arg;

	 if (!bev) {
        log_error("Received NULL buffer event in temp_cb\n");
        return;
    }
	 //wss_session_t *session = (wss_session_t *)arg;

	//if (!session)
    //{
    //    log_error("session pointer is NULL in temp_cb\n");
    //    return;
    //}

    //log_info("Sending temperature for client: %s\n", session->client);

    send_temperature(bev);
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

    if( events&(BEV_EVENT_EOF|BEV_EVENT_ERROR) )
    {
        if( session )
            log_warn("remote client %s closed\n", session->client);

   //     bufferevent_free(bev);
    }

    return ;
}


static void accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *arg)
{
	struct event_base               *ebase = arg;
    //struct bufferevent              *bev_accpt;
	struct bufferevent 				*recv_bev, *send_bev;
	struct event            		*temp_event = NULL;
	struct timeval          		tv;
    struct sockaddr_in              *sock = (struct sockaddr_in *)addr;
    wss_session_t                   *session;

	if( !(session = malloc(sizeof(*session))) )
    {
        log_error("malloc for session failure:%s\n", strerror(errno));
        close(fd);
        return ;
    }

	memset(session, 0, sizeof(*session));
    snprintf(session->client, sizeof(session->client), "[%d->%s:%d]", fd, inet_ntoa(sock->sin_addr), ntohs(sock->sin_port));
    log_info("accpet new socket client %s\n", session->client);

#if 0
	bev_accpt = bufferevent_socket_new(ebase, fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);
	if( !bev_accpt )
    {
        log_error("create bufferevent for client for %s failed\n", session->client);
        return;
    }
	session->bev = bev_accpt;

	bufferevent_setcb(bev_accpt, read_cb, NULL, event_cb, session);
	bufferevent_enable(bev_accpt, EV_READ|EV_WRITE);

#endif

	recv_bev = bufferevent_socket_new(ebase, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    if (!recv_bev) {
        log_error("create bufferevent for client %s failed\n", session->client);
        return;
    }

    send_bev = bufferevent_socket_new(ebase, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    if (!send_bev) {
        log_error("create send bufferevent for client %s failed\n", session->client);
        bufferevent_free(recv_bev);
        return;
    }

    session->recv_bev = recv_bev;
    session->send_bev = send_bev;

    bufferevent_setcb(recv_bev, read_cb, NULL, event_cb, session);
    bufferevent_enable(recv_bev, EV_READ | EV_WRITE);

#if 1
	tv.tv_sec = 5;	
	tv.tv_usec = 0;
	temp_event = event_new(ebase, -1, EV_PERSIST, temp_cb, send_bev);
	if (!temp_event)
    {
        log_error("failed to create temp event\n");
        return;
    }
	event_add(temp_event, &tv);
#endif

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

	if( log_open("console", loglevel, logsize, THREAD_LOCK_NONE) < 0 )
	{
		fprintf(stderr, "initial log system failure\n");
		return -1;
	}

	base = event_base_new();
	if( !base )
	{
		log_error("event_base_new() failure\n");
		return -2;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	listener = evconnlistener_new_bind(base, accept_cb, base, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr *)&addr, sizeof(addr));
	if( !listener )
	{
		log_error("Can't create a listener\n");
		event_base_free(base);
		return -3;
	}



	event_base_dispatch(base);
	evconnlistener_free(listener);
	event_base_free(base);

	return 0;
} 

