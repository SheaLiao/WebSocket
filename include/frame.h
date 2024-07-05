/********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<2928382441@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  frame.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(2024年06月28日)
 *         Author:  Liao Shengli <2928382441@qq.com>
 *      ChangeLog:  1, Release initial version on "2024年06月28日 16时35分32秒"
 *                 
 ********************************************************************************/

#ifndef _FRAME_H_
#define _FRAME_H_

#include "ws.h"

#define FRAME_SIZE       3072
#define FRAMES_MAX       10

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

typedef enum 
{
    CLOSE_NORMAL                 = 1000, /* The connection */
    CLOSE_SHUTDOWN               = 1001, /* Server is shutting down */
    CLOSE_PROTOCOL               = 1002, /* Some error in the protocol has happened */
    CLOSE_TYPE                   = 1003, /* The type (text, binary) was not supported */
    CLOSE_NO_STATUS_CODE         = 1005, /* No status code available */
    CLOSE_ABNORMAL               = 1006, /* Abnormal close */
    CLOSE_UTF8                   = 1007, /* The message wasn't in UTF8 */
    CLOSE_POLICY                 = 1008, /* The policy of the server has been broken */
    CLOSE_BIG                    = 1009, /* The messages received is too big */
    CLOSE_EXTENSION              = 1010, /* Mandatory extension missing */
    CLOSE_UNEXPECTED             = 1011, /* Unexpected happened */
    CLOSE_RESTARTING             = 1012, /* Service Restart */
    CLOSE_TRY_AGAIN              = 1013, /* Try Again Later */
    CLOSE_INVALID_PROXY_RESPONSE = 1014, /* Server acted as a gateway or proxy and received an invalid response from the upstream server. */
    CLOSE_FAILED_TLS_HANDSHAKE   = 1015  /* Unexpected TLS handshake failed */
} wss_close_t;


typedef enum 
{
    CONTINUATION_FRAME = 0x0,
    TEXT_FRAME         = 0x1,
    BINARY_FRAME       = 0x2,
    CLOSE_FRAME        = 0x8,
    PING_FRAME         = 0x9,
    PONG_FRAME         = 0xA,
} wss_opcode_t;


typedef struct wss_frame_head_s
{
	unsigned char	opcode:4;
	unsigned char	rsv3:1;
	unsigned char	rsv2:1;
	unsigned char	rsv1:1;
	unsigned char	fin:1;
	unsigned char	payloadlength:7;
	unsigned char	mask:1;
} wss_frame_head_t;


typedef struct wss_frame_s
{
	bool fin;
    bool rsv1;
    bool rsv2;
    bool rsv3;
    uint8_t opcode;
    bool mask;
    uint64_t payloadlength;
    char maskingkey[4];
    char *payload;
    uint64_t extensionDataLength;
    uint64_t applicationDataLength;
} wss_frame_t;


extern wss_frame_t *wss_parse_frame(char *payload, size_t length, size_t *offset);
extern void do_parser_frames(wss_session_t *session);
extern wss_close_t wss_validate_frame(wss_frame_t *frame, wss_close_t *reason);
extern int wss_create_frame(int opcode, char *payload, size_t payloadlength, char *buf, size_t size);
extern char *wss_closing_string(wss_close_t reason);
extern void wss_free_frame(wss_frame_t *frame);


/* Create a websocket text frame message. */
#define FRAME_MSG_ACK            "ACK"
#define FRAME_MSG_NAK            "NAK"
#define wss_create_text_frame(buf, size, msg)     wss_create_frame(TEXT_FRAME, msg, strlen(msg), buf, size)

/* Create a websocket close frame message. */
#define wss_create_close_frame(buf, size, reason) \
        wss_create_frame(CLOSE_FRAME, wss_closing_string(reason), strlen(wss_closing_string(reason)), buf, size)


#endif
