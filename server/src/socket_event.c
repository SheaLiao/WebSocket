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
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <event2/bufferevent_ssl.h>


#include "socket_event.h"
#include "logger.h"
#include "database.h"


int socket_ev_init(socket_event_t *sock_ev, int port)
{
	if( !sock_ev || port <= 0 )
	{
		return -1;
	}

	memset(sock_ev, 0, sizeof(socket_event_t));
	sock_ev->port = port;
	sock_ev->base = NULL;
	sock_ev->listener = NULL;
	sock_ev->sig = NULL;
	sock_ev->ssl_ctx = NULL;
	sock_ev->ssl = NULL;

	return 0;
}


int evssl_init(socket_event_t *sock_ev)
{
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	sock_ev->ssl_ctx = SSL_CTX_new(TLS_server_method());
    if( !sock_ev->ssl_ctx )
    {
        log_error("SSL_CTX_new() failure\n");
		SSL_CTX_free(sock_ev->ssl_ctx);
		return -1;
    }

	if( !SSL_CTX_use_certificate_chain_file(sock_ev->ssl_ctx, CACERT_FILE) || !SSL_CTX_use_PrivateKey_file(sock_ev->ssl_ctx, PRIVKEY_FILE, SSL_FILETYPE_PEM) )
	{
		log_error("Couldn't read 'pkey' or 'cert' file\n");
		SSL_CTX_free(sock_ev->ssl_ctx);
		return -2;
	}
	
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


void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *arg)
{
	socket_event_t			*sock_ev = (socket_event_t *)arg;
	struct bufferevent		*bev = NULL;
	struct event_base 		*evbase;

	evbase = evconnlistener_get_base(listener);
	
	bev = bufferevent_openssl_socket_new(evbase, fd, sock_ev->ssl, BUFFEREVENT_SSL_ACCEPTING, BEV_OPT_CLOSE_ON_FREE);
	if( !bev )
	{
		log_error("bufferevent_socket_new() failure\n");
		close(fd);
		return ;
	}

	bufferevent_setcb(bev, read_cb, NULL, event_callback, sock_ev);
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
	
		if( insert_database(buf, sizeof(buf)) < 0 )
		{
			log_error("Failed to insert data into database\n");
		}
	}
}


void event_callback(struct bufferevent *bev, short events, void *arg)
{
	socket_event_t		*sock_ev = (socket_event_t *)arg;

	if( events & BEV_EVENT_EOF )
	{
		log_warn("Connection closed\n");
	}
	else if( events & BEV_EVENT_ERROR )
	{
		log_error("Connection error\n");
	}
	else if( events & BEV_EVENT_CONNECTED )
	{
		log_info("Connect successfully\n");
		return ;
	}
	
}


void signal_cb(evutil_socket_t sig, short events, void *arg)
{
	socket_event_t	*sock_ev = (socket_event_t *)arg;
	struct timeval	delay = {DELAY_TIME, 0};

	log_warn("Caught a signal, stop\n");

	event_base_loopexit(sock_ev->base, &delay);
}
