/*********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<2928382441@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  ws.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(2024年06月03日)
 *         Author:  Liao Shengli <2928382441@qq.com>
 *      ChangeLog:  1, Release initial version on "2024年06月03日 12时21分32秒"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <event2/listener.h>
\
#include "ws.h"
#include "logger.h"

#define ntohll(x)   ((((uint64_t)ntohl(x&0xFFFFFFFF)) << 32) + ntohl(x >> 32))

int ws_ctx_init(ws_ctx_t *ws_ctx)
{
	if( !ws_ctx )
	{
		return -1;
	}

	memset(ws_ctx, 0, sizeof(ws_ctx));
	ws_ctx->base = NULL;
	ws_ctx->listener = NULL;
	ws_ctx->bev = NULL;
	ws_ctx->state = 0;

	return 0;
}


void ws_ctx_close(ws_ctx_t *ws_ctx)
{
	if( !ws_ctx )
	{
		return ;
	}

	if( ws_ctx->bev )
	{
		bufferevent_free(ws_ctx->bev);
		ws_ctx->bev = NULL;
	}

	if( ws_ctx->base )
	{
		event_base_free(ws_ctx->base);
		ws_ctx->base = NULL;
	}

}

int base64_encode(char *in_str, int in_len, char *out_str) 
{
    BIO 	*b64, *bio;
    BUF_MEM *bptr = NULL;
    size_t 	size = 0;

    if (in_str == NULL || out_str == NULL)
        return -1;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, in_str, in_len);
    BIO_flush(bio);

    BIO_get_mem_ptr(bio, &bptr);
    memcpy(out_str, bptr->data, bptr->length);
    out_str[bptr->length-1] = '\0';
    size = bptr->length;

    BIO_free_all(bio);
    return size;
}


int readline(char *allbuf, int idx, char *linebuf) 
{
    int len = strlen(allbuf);

    for(; idx < len; idx++) 
	{
        if (allbuf[idx] == '\r' && allbuf[idx+1] == '\n') 
		{
            return idx+2;
        } 
		else 
		{
            *(linebuf++) = allbuf[idx];
        }
    }

    return -1;
}


int handshark(struct bufferevent *bev, char *buf, int len)
{
	char	linebuf[1024] = {0};
	int		idx = 0;
	char	sec_data[20] = {0};
	char	sec_accept[128] = {0};

	do{
		memset(linebuf, 0, 1024);
		idx = readline(buf, idx, linebuf);

		if(strstr(linebuf, "Sec-WebSocket-Key"))
		{
			strcat(linebuf, GUID);
			
			SHA1(linebuf + WEBSOCK_KEY_LENGTH, strlen(linebuf + WEBSOCK_KEY_LENGTH), sec_data);
			base64_encode(sec_data, strlen(sec_data), sec_accept);

			memset(buf, 0, BUFFER_LENGTH);
			len = sprintf(buf, "HTTP/1.1 101 Switching Protocols\r\n"
                            	"Upgrade: websocket\r\n"
                        	    "Connection: Upgrade\r\n"
                                "Sec-WebSocket-Accept: %s\r\n\r\n", sec_accept);
			
			printf("Response: %s\n", buf);

			bufferevent_write(bev, buf, len);
			break;
		}
	} while ((buf[idx] != '\r' || buf[idx+1] != '\n') && idx != -1);

	return 0;
}


void umask(char *payload, int length, char *mask_key) 
{
    int i = 0;
    
	for (i = 0; i < length; i++) {
        payload[i] ^= mask_key[i % 4];
    }
}


int transmission(ws_ctx_t *ws_ctx, char *buf, int len)
{
	ws_ophdr_t		*hdr = NULL;
	ws_head_126_t	*hdr126 = NULL;
	ws_head_127_t	*hdr127 = NULL;

	unsigned char 	*payload = NULL;
	int				pl_len = 0;

	printf("buf:%s\n", buf);
	hdr = (ws_ophdr_t *)buf;
	if( hdr->pl_len < 126 )
	{
		pl_len = hdr->pl_len; 
		payload = (unsigned char *)(buf + sizeof(ws_ophdr_t) + 4);
		
		if (hdr->mask)  //mask=1,need to decode
		{
            umask((char *)payload, pl_len, buf + sizeof(ws_ophdr_t));
        }
		
		log_info("payload:%s\n", payload);
	}
	else if ( hdr->pl_len == 126 )
	{
		hdr126 = (ws_head_126_t *)(buf + sizeof(ws_ophdr_t));
		pl_len = ntohs(hdr126->pl_len);
		payload =(unsigned char *) (buf + sizeof(ws_ophdr_t) + sizeof(ws_head_126_t) + 4);
		
		if (hdr->mask)  //mask=1,need to decode
		{
            umask(payload, pl_len, buf + sizeof(ws_ophdr_t) + sizeof(ws_head_126_t));
        }
		
		log_info("payload:%s\n", payload);
	}
	else
	{
		hdr127 = (ws_head_127_t *)(buf + sizeof(ws_ophdr_t));
		pl_len = ntohll(hdr127->pl_len);
		payload =(unsigned char *) (buf + sizeof(ws_ophdr_t) + sizeof(ws_head_127_t) + 4);

		if (hdr->mask)  //mask=1,need to decode
		{
            umask(payload, hdr->pl_len, buf + sizeof(ws_ophdr_t) + sizeof(ws_head_127_t));
        }
		
		log_info("payload:%s\n", payload);
	}

	return 0;
}

void ws_request(struct bufferevent *bev, char *buf, int len, void *arg) 
{
    ws_ctx_t *ws_ctx = (ws_ctx_t *)arg;

    printf("state: %d\n", ws_ctx->state);
    if (ws_ctx->state == WS_HANDSHARK) 
	{
        ws_ctx->state = WS_TRANMISSION;
        handshark(bev, buf, len);
    } 
	else if (ws_ctx->state == WS_TRANMISSION) 
	{
        transmission(ws_ctx, buf, len);
    }

    log_info("websocket request: %d\n", ws_ctx->state);
}


