/********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<2928382441@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  ws.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(2024年06月03日)
 *         Author:  Liao Shengli <2928382441@qq.com>
 *      ChangeLog:  1, Release initial version on "2024年06月03日 11时55分30秒"
 *                 
 ********************************************************************************/

#ifndef _WS_H_
#define _WS_H_

#define BUFFER_LENGTH	 	4096
#define GUID  				"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WEBSOCK_KEY_LENGTH	19 //length of "Sec-WebSocket-Key: "


enum {
  WS_HANDSHARK = 0,
  WS_TRANMISSION = 1,
  WS_END = 2,
};

typedef struct ws_ophdr_s 
{
	unsigned char opcode:4,
				  rsv3:1,
				  rsv2:1,
				  rsv1:1,
				  fin:1;
	unsigned char pl_len:7,
				  mask:1;
} ws_ophdr_t;


typedef struct ws_head_126_s 
{
  unsigned short 	pl_len;
  char 				mask_key[4];
} ws_head_126_t;

typedef struct ws_head_127_s 
{
  unsigned long long 	pl_len;
  char 					mask_key[4];
} ws_head_127_t;

typedef struct ws_ctx_s
{
	char					buf[BUFFER_LENGTH];
	struct event_base       *base;
	struct evconnlistener   *listener;
	struct bufferevent 		*bev;
	int						state;
} ws_ctx_t;



extern int ws_ctx_init(ws_ctx_t *ws_ctx);
extern void ws_ctx_close(ws_ctx_t *ws_ctx);
extern int base64_encode(char *in_str, int in_len, char *out_str);
extern int readline(char *allbuf, int idx, char *linebuf);
extern int handshark(struct bufferevent *bev, char *buf, int len);
extern void umask(char *payload, int length, char *mask_key);
extern int transmission(ws_ctx_t *ws_ctx, char *buf, int len);
extern void ws_request(struct bufferevent *bev, char *buf, int len, void *arg);


#endif
