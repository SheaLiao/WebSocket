/********************************************************************************
 *      Copyright:  (C) 2024 linuxer<linuxer@email.com>
 *                  All rights reserved.
 *
 *       Filename:  socket_event.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(10/05/24)
 *         Author:  Liao Shengli <2928383441@qq.com>
 *      ChangeLog:  1, Release initial version on "10/05/24 14:05:39"
 *                 
 ********************************************************************************/

#ifndef _SOCKET_H_
#define _SOCKET_H_

typedef struct socket_event_s
{
	int					fd;
	char				servip[64];
	int					port;
	struct event_base	*base;
	struct bufferevent	*bev;
}socket_event_t;


extern int socket_ev_init(socket_event_t *sock_ev, char *servip, int port);
extern int socket_ev_close(socket_event_t *sock_ev);
extern int socket_ev_connect(socket_event_t *sock_ev);
extern void event_callback(struct bufferevent *bev, short events, void *arg);
extern void read_cb(struct bufferevent *bev, void *arg);
//extern void write_cb(struct bufferevent *bev, void *arg);
extern void reconnect_cb(evutil_socket_t fd, short events, void *arg);
extern void send_data(evutil_socket_t fd, short events, void *arg);

#endif
