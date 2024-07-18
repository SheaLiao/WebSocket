/*********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<li@gmail.com>
 *                  All rights reserved.
 *
 *       Filename:  websocket.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(15/07/24)
 *         Author:  Liao Shengli <li@gmail.com>
 *      ChangeLog:  1, Release initial version on "15/07/24 20:03:58"
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
#include <event2/thread.h>
#include <pthread.h>


#include "logger.h"
#include "wss.h"
#include "frame.h"
#include "sht20.h"
#include "led.h"

#define PORT 			20000
#define SEND_INTERVAL 	5

pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;

static void accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int slen, void *arg);
static void read_cb(struct bufferevent *bev, void *ctx);
static void event_cb(struct bufferevent *bev, short events, void *ctx);
static void send_cb(evutil_socket_t fd, short event, void *arg);



int main (int argc, char **argv)
{
	struct sockaddr_in      addr;
	int						len = sizeof(addr);

	struct event_base       *base = NULL;
	struct evconnlistener   *listener = NULL;

	char                    *logfile = "websocket.log";
	int                     loglevel = LOG_LEVEL_INFO;
	int                     logsize = 10;

	struct gpiod_chip 		*chip;

	if( log_open("console", loglevel, logsize, THREAD_LOCK_NONE) < 0 )
	{
		fprintf(stderr, "initial log system failure\n");
		return -1;
	}

	init_gpio(chip);

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


	close_gpio(chip);
	evconnlistener_free(listener);
	event_base_free(base);

	return 0;
}



static void accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *arg)
{
	struct event_base		*base = arg;
	struct bufferevent		*bev;
	struct event 			*timer_event;
	struct timeval			tv = {SEND_INTERVAL, 0};
	struct sockaddr_in 		*sock = (struct sockaddr_in *) addr;
	wss_session_t 			*session;


	if (!(session = malloc(sizeof(*session))))
	{
		log_error("malloc for session failure: %s\n", strerror(errno));
		close(fd);
		return;
	}

	memset(session, 0, sizeof(*session));
	snprintf(session->client, sizeof(session->client), "[%d->%s:%d]", fd, inet_ntoa(sock->sin_addr),ntohs(sock->sin_port));
	log_info("accept new socket client %s\n", session->client);

	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);
	if( !bev )
	{
		log_error("create bufferevent for client for %s failed\n", session->client);
		free(session);
		close(fd);
		return ;
	}
	log_info("create bufferevent successfully\n");

	session->bev = bev;

	bufferevent_setcb(bev, read_cb, NULL, event_cb, session);
	bufferevent_enable(bev, EV_READ|EV_WRITE);


#if 1
	timer_event = evtimer_new(base, send_cb, session);
	if( !timer_event )
	{
		log_error("timer_event create failure\n");
		bufferevent_free(bev);
		free(session);
		return ;
	}
	log_info("timer_event set successfully\n");
	session->timer_event = timer_event;
	evtimer_add(timer_event, &tv);
#endif

	return ;
}



static void read_cb(struct bufferevent *bev, void *ctx)
{
	wss_session_t *session = ctx;

	if (!session->handshaked)
	{
		do_wss_handshake(session);
		return;
	}

	do_parser_frames(session);

	return;
}



static void send_cb(evutil_socket_t fd, short event, void *arg)
{
    wss_session_t *session = arg;
    struct timeval current_time, interval;
    struct timeval tv = {SEND_INTERVAL, 0};

    if (!session || !session->bev)
    {
        log_error("session or bev is NULL\n");
        return;
    }

    if (session->handshaked)
    {
        gettimeofday(&current_time, NULL);

        timersub(&current_time, &session->last_sent_time, &interval);

        if (interval.tv_sec >= SEND_INTERVAL)
        {
            pthread_mutex_lock(&send_mutex);

            if (evbuffer_get_length(bufferevent_get_output(session->bev)) == 0)
            {
                send_sample_data(session);
                gettimeofday(&session->last_sent_time, NULL);
            }

            pthread_mutex_unlock(&send_mutex);
        }

        evtimer_add(session->timer_event, &tv);
    }
}



static void event_cb (struct bufferevent *bev, short events, void *ctx)
{
	wss_session_t              *session = ctx;

	if( events&(BEV_EVENT_EOF|BEV_EVENT_ERROR) )
	{
		if( session )
			log_warn("remote client %s closed\n", session->client);

		if (session->timer_event)
		{
			evtimer_del(session->timer_event);
			event_free(session->timer_event);
		}

		bufferevent_free(session->bev);
		free(session);
	}

	return ;
}
