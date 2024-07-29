/*
 * frame.c
 *
 *  Created on: 2024年7月27日
 *      Author: asus
 */


#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "frame.h"
#include "esp8266.h"
#include "usart.h"
#include "tim.h"


#define CONFIG_DEBUG
#ifdef CONFIG_DEBUG
#define dbg_print(format, args...) printf(format,##args)
#else
#define dbg_print(format, args...) do{} while(0)
#endif


static inline uint64_t htonl64(uint64_t value);
static inline uint16_t htons16(uint16_t value);
static void unmask(wss_frame_t *frame);


void do_parser_frames(int sockfd, wss_session_t *session)
{
    uint8_t                     rxbuf[UART2_BUFFER_SIZE];
    uint8_t                     frame_buf[UART2_BUFFER_SIZE];
    size_t                      offset = 0;
    int                         bytes;
    wss_frame_t                *frame;
    wss_close_t                 closing = 0;

    dbg_print("Doing websocket parser frames\n");

    //esp8266_sock_recv(rxbuf, bytes);
    uart2_ring_buffer_read(rxbuf);
    // 从客户端接收数据
    extract_after_ipd(rxbuf, frame_buf);


    dbg_print("Parser [%d] bytes data from client %s\n", bytes, session->client);

    // 解析 WebSocket 帧
    frame = wss_parse_frame(frame_buf, bytes, &offset);
    if (!frame)
    {
    	dbg_print("Unable to parse frame\n");
        free(session);
        return;
    }

    dbg_print("Validating frame\n");

    // 部分 WebSocket 帧检测
    if (offset > bytes)
    {
    	dbg_print("Detected that data was missing in order to complete frame\n");
        closing = CLOSE_UNEXPECTED;
    }

    if (!wss_validate_frame(frame, &closing))
    {
    	dbg_print("Invalid frame detected: %s\n", wss_closing_string(closing));
    }

    // 检测到关闭帧
    if (frame->opcode == CLOSE_FRAME)
    {
    	dbg_print("Stopping frame validation as closing frame was parsed\n");
        closing = CLOSE_PROTOCOL;
    }

    dbg_print("Valid websocket frame was parsed.\n");

    // 回复帧处理
    if (closing)
    {
        bytes = wss_create_close_frame((char *)frame_buf, sizeof(frame_buf), closing);
    }
    else
    {
        if (frame->payload > 0)
        {
            bytes = wss_create_frame(TEXT_FRAME, frame->payload, frame->payloadLength, (char *)frame_buf, sizeof(frame_buf));
        }
        else
        {
            bytes = wss_create_text_frame((char *)frame_buf, sizeof(frame_buf), FRAME_MSG_ACK);
        }
    }

    wss_free_frame(frame);

    dbg_print("Sending frames to client\n");

    esp8266_sock_send(frame_buf, sockfd, bytes);

    if (closing)
    {
    	delay_us(10000);
        dbg_print("Closing connection, since closing frame has been sent\n");
        free(session);
    }

    return;
}




wss_frame_t *wss_parse_frame(char *payload, size_t length, size_t *offset)
{
    wss_frame_t *frame;

    if ( !payload ) {
    	dbg_print("Payload cannot be NULL\n");
        return NULL;
    }

    if ( !(frame = malloc(sizeof(wss_frame_t))) ) {
    	dbg_print("Unable to allocate frame\n");
        return NULL;
    }
    memset(frame, 0, sizeof(*frame));

    dbg_print("Parsing frame starting from offset %lu\n", *offset);

    frame->fin    = 0x80 & payload[*offset];
    frame->rsv1   = 0x40 & payload[*offset];
    frame->rsv2   = 0x20 & payload[*offset];
    frame->rsv3   = 0x10 & payload[*offset];
    frame->opcode = 0x0F & payload[*offset];
    dbg_print("frame->fin          : %d\n", frame->fin ? 1 : 0);
    dbg_print("frame->opcode       : %d\n", frame->opcode);

    *offset += 1;

    if ( *offset < length ) {
        frame->mask          = 0x80 & payload[*offset];
        frame->payloadLength = 0x7F & payload[*offset];
    }
    dbg_print("frame->mask         : %d\n", frame->mask ? 1 : 0);

    *offset += 1;
    switch (frame->payloadLength) {
        case 126:
            if ( *offset+sizeof(uint16_t) <= length ) {
                memcpy(&frame->payloadLength, payload+*offset, sizeof(uint16_t));
                frame->payloadLength = htons16(frame->payloadLength);
            }
            *offset += sizeof(uint16_t);
            break;
        case 127:
            if ( *offset+sizeof(uint64_t) <= length ) {
                memcpy(&frame->payloadLength, payload+*offset, sizeof(uint64_t));
                frame->payloadLength = htonl64(frame->payloadLength);
            }
            *offset += sizeof(uint64_t);
            break;
    }
    dbg_print("frame->payloadLength: %lu\n", frame->payloadLength);

    if ( frame->mask ) {
        if ( *offset+sizeof(uint32_t) <= length ) {
            memcpy(frame->maskingKey, payload+*offset, sizeof(uint32_t));
        }
        *offset += sizeof(uint32_t);
    }
    dbg_print("frame->maskingKey   : %08X\n", *(unsigned int *)frame->maskingKey);

    frame->appDataLength = frame->payloadLength-frame->extDataLength;
    dbg_print("frame->appDataLength: %lu\n", frame->appDataLength);
    if ( frame->appDataLength > 0 ) {
        if ( *offset+frame->appDataLength <= length ) {
            if ( NULL == (frame->payload = malloc(frame->appDataLength)) ) {
            	dbg_print("Unable to allocate frame application data\n");
                return NULL;
            }

            dbg_print("copy out application data to frame\n");
            memcpy(frame->payload, payload+*offset, frame->appDataLength);
        }
        *offset += frame->appDataLength;
    }

    /* receive data is integrated */
    if ( frame->mask && *offset <= length ) {
        if( frame->appDataLength > 0)
        {
            unmask(frame);
        }
    }


    return frame;
}


wss_close_t wss_validate_frame(wss_frame_t *frame, wss_close_t *reason)
{
    int             error = 0;

    if( !frame )
    {
    	dbg_print("Invalid input arguments\n");
        error = CLOSE_UNEXPECTED;
    }

    /* If opcode is unknown */
    if ( frame->opcode>=0x3 && frame->opcode!=CLOSE_FRAME && frame->opcode != PONG_FRAME )
    {
    	dbg_print("Type Error: Unknown/Unsupport opcode[0x%02x]\n", frame->opcode);
        error = CLOSE_TYPE;
    }

    // Server expects all received data to be masked
    else if ( !frame->mask )
    {
    	dbg_print("Protocol Error: Client message should always be masked\n");
        error = CLOSE_ABNORMAL;

    }


    // Control frames cannot be fragmented,鎺у埗甯т笉鑳藉垎鐗?
    else if ( !frame->fin && frame->opcode >= 0x8 && frame->opcode <= 0xA )
    {
    	dbg_print("Protocol Error: Control frames cannot be fragmented\n");
        error = CLOSE_ABNORMAL;
    }

    // Check that frame is not too large
    else if ( frame->payloadLength > FRAME_SIZE )
    {
    	dbg_print("Protocol Error: Control frames cannot be fragmented\n");
        error = CLOSE_BIG;
    };

    if( error && reason )
        *reason = error;

    return error ? 0 : 1;
}

/**
 * Releases memory used by a frame.
 *
 * @param   ping     [wss_frame_t *]    "The frame that should be freed"
 * @return           [void]
 */
void wss_free_frame(wss_frame_t *frame)
{

    if ( frame )
    {
        if( frame->payload)
        {
            free(frame->payload);
        }
    }
    free(frame);
}



int wss_create_frame(int opcode, char *payload, size_t payload_len, char *buf, size_t size)
{
    wss_frame_head_t   *frame = (wss_frame_head_t *)buf;
    int                 offset = 0;

    if( !buf )
    {
    	dbg_print("Invalid input arguments\n");
        return 0;
    }

    if( size < payload_len + sizeof(wss_frame_head_t) )
    {
    	dbg_print("frame output buffer too small [%d]<%d\n", size, payload_len+10);
    }

    dbg_print("Creating opcode[%d] frame\n", opcode);

    /*+----------------+
     *|   Frame head   |
      +----------------+*/
    memset(buf, 0, size);
    frame->fin = 1;
    frame->rsv1 = 0;
    frame->rsv2 = 0;
    frame->rsv3 = 0;
    frame->opcode = 0xF & opcode;
    frame->mask = 0;

	offset += sizeof(wss_frame_head_t);

    /*+----------------+
     *| Payload length |
      +----------------+*/
    if( payload_len <= 125 )
    {
        frame->payload_len = payload_len;
    }
    else if ( payload_len <= 65535 )
    {
        uint16_t    plen;

        frame->payload_len = 126;

        plen = htons16(payload_len);
        memcpy(buf+offset, &plen, sizeof(plen));
        offset += sizeof(plen);
    }
    else
    {
        uint64_t    plen;

        frame->payload_len = 127;

        plen = htonl64(payload_len);
        memcpy(buf+offset, &plen, sizeof(plen));
        offset += sizeof(plen);
    }


    /*+----------------+
     *|  Payload data  |
      +----------------+*/
    memcpy(buf+offset, payload, payload_len);
    offset += payload_len;

    return offset;
}



/**
 * Creates a closing frame given a reason for the closure.
 *
 * @param   reason   [wss_close_t]      "The reason for the closure"
 * @return           [char *     ]      "The reason string for the closure"
 */
char *wss_closing_string(wss_close_t reason)
{
    switch (reason) {
        case CLOSE_NORMAL:
            return "Normal close";

        case CLOSE_SHUTDOWN:
            return "Server is shutting down";

        case CLOSE_PROTOCOL:
            return "Protocol close";

        case CLOSE_TYPE:
            return "Unsupported data type";

        case CLOSE_NO_STATUS_CODE:
            return "No status received";

        case CLOSE_ABNORMAL:
            return "Abnormal closure";

        case CLOSE_UTF8:
            return "Invalid frame payload data";

        case CLOSE_POLICY:
            return "Policy Violation";

        case CLOSE_BIG:
            return "Message is too big";

        case CLOSE_EXTENSION:
            return "Mandatory extension";

        case CLOSE_UNEXPECTED:
            return "Internal server error";

        case CLOSE_RESTARTING:
            return "Server is restarting";

        case CLOSE_TRY_AGAIN:
            return "Try again later";

        case CLOSE_INVALID_PROXY_RESPONSE:
            return "The server was acting as a gateway or proxy and received an invalid response from the upstream server.";

        case CLOSE_FAILED_TLS_HANDSHAKE:
            return "Failed TLS Handshake";

        default:
            return "Unknown closing reason";
    }
}



static inline uint64_t htonl64(uint64_t value)
{
    static const int num = 42;

    /**
     * If this check is true, the system is using the little endian
     * convention. Else the system is using the big endian convention, which
     * means that we do not have to represent our integers in another way.
     */
    if (*(char *)&num == 42) {
        const uint32_t high = (uint32_t)(value >> 32);
        const uint32_t low = (uint32_t)(value & 0xFFFFFFFF);

        return (((uint64_t)(htonl(low))) << 32) | htonl(high);
    } else {
        return value;
    }
}

/**
 * Converts the unsigned 16 bit integer from host byte order to network byte
 * order.
 *
 * @param   value  [uint16_t]   "A 16 bit unsigned integer"
 * @return         [uint16_t]   "The 16 bit unsigned integer in network byte order"
 */
static inline uint16_t htons16(uint16_t value)
{
    static const int num = 42;

    /**
     * If this check is true, the system is using the little endian
     * convention. Else the system is using the big endian convention, which
     * means that we do not have to represent our integers in another way.
     */
    if (*(char *)&num == 42) {
        return ntohs(value);
    } else {
        return value;
    }
}

static void unmask(wss_frame_t *frame)
{
    char *applicationData = frame->payload+frame->extDataLength;
    uint64_t i = 0;
    uint64_t j;

    for (j = 0; likely(i < frame->appDataLength); i++, j++){
        applicationData[j] = applicationData[i] ^ frame->maskingKey[j % 4];
    }
}
