/*********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<2928382441@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  main.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(15/05/24)
 *         Author:  Liao Shengli <2928382441@qq.com>
 *      ChangeLog:  1, Release initial version on "15/05/24 16:51:05"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <event2/listener.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <event2/bufferevent_ssl.h>


#include "database.h"
#include "logger.h"
#include "socket_event.h"


void print_usage(char *progname)
{
	printf("Usage: %s [OPTION]...\n",progname);

	printf("-p(--port): sepcify server port.\n");
	printf("-d(--debug): running in debug mode.\n");
	printf("-b(--daemon): set program running on background.\n");
	printf("-h(--help): print this help information.\n");

	printf("\nExample: %s -p 8888\n", progname);
	return ;
}



int main (int argc, char **argv)
{
	int					daemon_run = 0;
	int					rv = -1;

	char				*logfile = "server.log";
	int					loglevel = LOG_LEVEL_DEBUG;
	int					logsize = 10;

	socket_event_t		*sock_ev = (socket_event_t *)malloc(sizeof(socket_event_t));
	int					port = 0;
	struct sockaddr_in	addr;

	const char         	*optstring = "bp:dh";
	int         		opt = -1;
	struct option		opts[] = {
		{"daemon", no_argument, NULL, 'b'},
		{"prot", required_argument, NULL, 'p'},
		{"debug", no_argument, NULL, 'd'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};


	while ( (opt = getopt_long(argc, argv, optstring, opts, NULL)) != -1 )
	{
		switch (opt)
		{
			case 'b':
				daemon_run = 1;
				break;

			case 'p':
				port = atoi(optarg);
				break;

			case 'd':
				daemon_run = 0;
				logfile = "console";
				loglevel = LOG_LEVEL_DEBUG;
				break;

			case 'h':
				print_usage(argv[0]);
				return 0;

			default:
				break;
		}
	}


	if( !port )
	{
		print_usage(argv[0]);
		return -1;
	}

	if( log_open(logfile, loglevel, logsize, THREAD_LOCK_NONE ) < 0 )
	{
		fprintf(stderr, "initial log system failure\n");
	}

	if( daemon_run )
	{
		daemon(1, 0);
	}

	rv = open_database("server.db");
	if( rv < 0 )
	{
		log_error("open database failure\n");
		return -2;
	}

	rv = socket_ev_init(sock_ev, port);
	if( rv < 0 )
	{
		log_error("init socket failure\n");
		free(sock_ev);
		return -3;
	}
	log_info("init socket successfully\n");

	rv = evssl_init(sock_ev);
	if( rv < 0 )
	{
		log_error("evssl_init() failure\n");
		free(sock_ev);
		return -4;
	}
	log_info("init ssl successfully\n");

	sock_ev->ssl = SSL_new(sock_ev->ssl_ctx);

	sock_ev->base = event_base_new();
	if( !sock_ev->base )
	{
		log_error("event_base_new() failure\n");
		free(sock_ev);
		return -4;
	}
	log_info("event_base_new() successfully\n");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	sock_ev->listener = evconnlistener_new_bind(sock_ev->base, listener_cb, sock_ev, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr *)&addr, sizeof(addr));
	if( !sock_ev->listener )
	{
		log_error("Can't create a listener\n");
		socket_ev_close(sock_ev);
		free(sock_ev);
		return -5;
	}
	log_info("Create a listener on port[%d]\n", port);

	sock_ev->sig = evsignal_new(sock_ev->base, SIGINT, signal_cb, sock_ev);
	if( !sock_ev->sig || event_add(sock_ev->sig, NULL) < 0 )
	{
		log_error("set signal failure\n");
		socket_ev_close(sock_ev);
		free(sock_ev);
		return -6;
	}

	event_base_dispatch(sock_ev->base);
	
	socket_ev_close(sock_ev);
	free(sock_ev);
	close_database();
	log_close();


	return 0;
} 

