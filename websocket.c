/*********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<2928382441@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  websocket.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(2024年06月03日)
 *         Author:  Liao Shengli <2928382441@qq.com>
 *      ChangeLog:  1, Release initial version on "2024年06月03日 20时52分02秒"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <event2/listener.h>

#include "logger.h"
#include "ws.h"

#define PORT	8888


void read_cb(struct bufferevent *bev, void *arg)
{
	ws_ctx_t	*ws_ctx = (ws_ctx_t *)arg;
	char		buf[BUFFER_LENGTH] = {0};
	int			len = 0;

	len = bufferevent_read(ws_ctx->bev, buf, BUFFER_LENGTH);
	if( len > 0 )
	{
		ws_request(ws_ctx->bev, buf, len, ws_ctx);
	}
}


void event_cb(struct bufferevent *bev, short events, void *arg)
{
	if (events & BEV_EVENT_EOF) 
	{
        log_error("Connection closed.\n");
    } 
	else if (events & BEV_EVENT_ERROR) 
	{
        log_error("Connect failure: %s\n", strerror(errno));
    }
    //bufferevent_free(bev);
}


void accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *arg)
{
    ws_ctx_t				*ws_ctx = (ws_ctx_t *)arg;

    ws_ctx->bev = bufferevent_socket_new(ws_ctx->base, fd, BEV_OPT_CLOSE_ON_FREE);
    if( !ws_ctx->bev )
    {
        log_error("bufferevent_socket_new() failure:%s\n", strerror(errno));
        close(fd);
        return ;
    }
	

	ws_ctx->state = 0;
	bufferevent_setcb(ws_ctx->bev, read_cb, NULL, event_cb, ws_ctx);
	bufferevent_enable(ws_ctx->bev, EV_READ | EV_WRITE);
}


int main (int argc, char **argv)
{
	ws_ctx_t				*ws_ctx = (ws_ctx_t *)malloc(sizeof(ws_ctx_t));
	struct evconnlistener   *listener = NULL;
	struct sockaddr_in		addr;
	int                 	len = sizeof(addr);

	char                    *logfile = "websocket.log";
	int                     loglevel = LOG_LEVEL_DEBUG;
	int                     logsize = 10;

	if( log_open(logfile, loglevel, logsize, THREAD_LOCK_NONE) < 0 )
	{
		fprintf(stderr, "initial log system failure\n");
		return -1;
	}

	if( ws_ctx_init(ws_ctx) < 0 )
	{
		log_error("ws_ctx initial failure\n");
		return -2;
	}

	ws_ctx->base = event_base_new();
    if( !ws_ctx->base )
    {
        log_error("event_base_new() failure\n");
        return -3;
    }

	memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

	listener = evconnlistener_new_bind(ws_ctx->base, accept_cb, ws_ctx, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr *)&addr, sizeof(addr));
    if( !listener )
    {
        log_error("Can't create a listener\n");
        return -4;
    }

	event_base_dispatch(ws_ctx->base);

	evconnlistener_free(listener);
	ws_ctx_close(ws_ctx);
	free(ws_ctx);

	return 0;
} 

