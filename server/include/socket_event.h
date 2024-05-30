/********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<2928382441@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  socket_event.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(14/05/24)
 *         Author:  Liao Shengli <2928382441@qq.com>
 *      ChangeLog:  1, Release initial version on "14/05/24 16:32:34"
 *                 
 ********************************************************************************/

#ifndef _SOCKET_EVENT_H_
#define _SOCKET_EVENT_H_


#define DELAY_TIME		5
#define CACERT_FILE 	"./ssl/cacert.pem"
#define PRIVKEY_FILE 	"./ssl/privkey.pem"


typedef struct socket_event_s
{
	int						port;
	struct event_base		*base;
	struct evconnlistener	*listener;
    struct event 			*sig;
	SSL_CTX 				*ssl_ctx;
   	SSL 					*ssl;
}socket_event_t;

extern int socket_ev_init(socket_event_t *sock_ev, int port);
extern int evssl_init(socket_event_t *sock_ev);
extern int socket_ev_close(socket_event_t *sock_ev);
extern void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *arg);
extern void read_cb(struct bufferevent *bev, void *arg);
extern void event_callback(struct bufferevent *bev, short events, void *arg);
extern void reconnect_cb(evutil_socket_t fd, short events, void *arg);
extern void signal_cb(evutil_socket_t sig, short events, void *arg);

#endif
