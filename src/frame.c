#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>

#include "frame.h"
#include "logger.h"
#include "predict.h"
#include "led.h"


#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_struct.h>

static inline uint64_t htonl64(uint64_t value);
static inline uint16_t htons16(uint16_t value);
static void unmask(wss_frame_t *frame);

/**
 * Performs a websocket handshake with the client session
 *
 * @param   session     [wss_session_t *]   "The client session"
 * @return              [void]
 */
void do_parser_frames(wss_session_t *session)
{
    char                        rxbuf[4096] = {0};
    int                         len, bytes;
    char                        frame_buf[4096] = {0};
    size_t                      offset = 0;
    struct evbuffer            *src;
    struct bufferevent         *bev = session->recv_bev;
    wss_frame_t                *frame;
    wss_close_t                 closing = 0;

    log_info("Doing websocket parser frames\n");

    /* copy out the receive data from evbuffer to rxbuf */
    src = bufferevent_get_input(bev);
    len = evbuffer_get_length(src);
    bytes = evbuffer_copyout(src, rxbuf, len);

    /* clear the receive evbuffer */
    evbuffer_drain(src, len);

    log_debug("Parser [%d] bytes data from client %s\n", bytes, session->client);
    log_dump(LOG_LEVEL_DEBUG, NULL, rxbuf, bytes);


    /*+----------------------+
     *|   Parsing the frame  |
     *+----------------------+*/

    frame = wss_parse_frame(rxbuf, bytes, &offset);
    if( !frame )
    {
        log_error("Unable to parse frame\n");
        bufferevent_free(bev);
    }

    /*+----------------------+
     *| validation the frame |
     *+----------------------+*/

    log_info("Validating frame\n");

    /* Partial websocket frame detect */
    if ( offset > bytes )
    {
        log_error("Detected that data was missing in order to complete frame\n");
        closing = CLOSE_UNEXPECTED;
    }

	//验证解析的帧，检查帧的完整性和有效性,如果数据不完整或帧无效，设置关闭原因 
    if( !wss_validate_frame(frame, &closing) )
    {
        log_error("Invalid frame detected: %s\n", wss_closing_string(closing));
    }

    /*  Close frame found */
    if ( frame->opcode == CLOSE_FRAME ) {
        log_warn("Stopping frame validation as closing frame was parsed\n");
        closing = CLOSE_PROTOCOL;
    }

    log_warn("Valid websocket frame was parsed.\n");

    /*+----------------------+
     *|  Reply for the frame |
     *+----------------------+*/

	/*构建响应帧*/
	//关闭帧
    if( closing )
    {
        bytes = wss_create_close_frame(frame_buf, sizeof(frame_buf), closing);
        log_dump(LOG_LEVEL_DEBUG, "Closing Frame:\n", frame_buf, bytes);
    }
    else
    {
    	//帧有有效负载, 构建文本帧作为回复
        if( frame->payload > 0 )
        {
            bytes = wss_create_frame(TEXT_FRAME, frame->payload, frame->payloadLength, frame_buf, sizeof(frame_buf));
            log_dump(LOG_LEVEL_DEBUG, "Reply Frame:\n", frame_buf, bytes);
        }
        else//帧没有有效负载,发送一个确认消息帧
        {
            bytes = wss_create_text_frame(frame_buf, sizeof(frame_buf), FRAME_MSG_ACK);
        }
    }

    wss_free_frame(frame);

    log_info("Sending frames to client\n");
    bufferevent_write(bev, frame_buf, bytes);

    if( closing )//如果是关闭帧，等待一段时间后关闭 bufferevent 连接
    {
        usleep(10000);
        log_trace("Closing connection, since closing frame has been sent\n");
        bufferevent_free(bev);
    }

    return ;
}

/**
 * Parses a payload of data into a websocket frame. Returns the frame and
 * corrects the offset pointer in order for multiple frames to be processed
 * from the same payload.
 *
 * @param   payload [char *]           "The payload to be processed"
 * @param   length  [size_t]           "The length of the payload"
 * @param   offset  [size_t *]         "A pointer to an offset"
 * @return          [wss_frame_t *]    "A websocket frame"
 */
wss_frame_t *wss_parse_frame(char *payload, size_t length, size_t *offset)
{
    wss_frame_t *frame;

    if ( !payload ) {
        log_error("Payload cannot be NULL\n");
        return NULL;
    }

    if ( !(frame = malloc(sizeof(wss_frame_t))) ) {
        log_error("Unable to allocate frame\n");
        return NULL;
    }
    memset(frame, 0, sizeof(*frame));

    log_trace("Parsing frame starting from offset %lu\n", *offset);

    frame->fin    = 0x80 & payload[*offset];
    frame->rsv1   = 0x40 & payload[*offset];
    frame->rsv2   = 0x20 & payload[*offset];
    frame->rsv3   = 0x10 & payload[*offset];
    frame->opcode = 0x0F & payload[*offset];
    log_trace("frame->fin          : %d\n", frame->fin ? 1 : 0);
    log_trace("frame->opcode       : %d\n", frame->opcode);

    *offset += 1;

    if ( *offset < length ) {
        frame->mask          = 0x80 & payload[*offset];
        frame->payloadLength = 0x7F & payload[*offset];
    }
    log_trace("frame->mask         : %d\n", frame->mask ? 1 : 0);

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
    log_trace("frame->payloadLength: %lu\n", frame->payloadLength);

    if ( frame->mask ) {
        if ( *offset+sizeof(uint32_t) <= length ) {
            memcpy(frame->maskingKey, payload+*offset, sizeof(uint32_t));
        }
        *offset += sizeof(uint32_t);
    }
    log_trace("frame->maskingKey   : %08X\n", *(unsigned int *)frame->maskingKey);

    frame->appDataLength = frame->payloadLength-frame->extDataLength;
    log_trace("frame->appDataLength: %lu\n", frame->appDataLength);
    if ( frame->appDataLength > 0 ) {
        if ( *offset+frame->appDataLength <= length ) {
            if ( NULL == (frame->payload = malloc(frame->appDataLength)) ) {
                log_error("Unable to allocate frame application data\n");
                return NULL;
            }

            log_trace("copy out application data to frame\n");
            memcpy(frame->payload, payload+*offset, frame->appDataLength);
        }
        *offset += frame->appDataLength;
    }

    /* receive data is integrated */
    if ( frame->mask && *offset <= length ) {
        if( frame->appDataLength > 0)
        {
            unmask(frame);
            log_dump(LOG_LEVEL_DEBUG, "frame->payload      :\n", frame->payload, frame->appDataLength);
        }
    }

	parse_led_data(frame->payload, frame->appDataLength);

    return frame;
}

/**
 * check the validation of the websocket frame.
 *
 * @param   frame       [wss_frame_t *]    "A websocket frame"
 * @param   reason      [wss_close_t *]    "Invalid reason"
 * @return              [int   ]           0: Invalid   1: Valid"
 */
wss_close_t wss_validate_frame(wss_frame_t *frame, wss_close_t *reason)
{
    int             error = 0;

    if( !frame )
    {
        log_error("Invalid input arguments\n");
        error = CLOSE_UNEXPECTED;
    }

    /* If opcode is unknown */
    if ( frame->opcode>=0x3 && frame->opcode!=CLOSE_FRAME && frame->opcode != PONG_FRAME )
    {
        log_error("Type Error: Unknown/Unsupport opcode[0x%02x]\n", frame->opcode);
        error = CLOSE_TYPE;
    }

    // Server expects all received data to be masked
    else if ( !frame->mask )
    {
        log_error("Protocol Error: Client message should always be masked\n");
        error = CLOSE_ABNORMAL;

    }


    // Control frames cannot be fragmented,控制帧不能分片 
    else if ( !frame->fin && frame->opcode >= 0x8 && frame->opcode <= 0xA )
    {
        log_error("Protocol Error: Control frames cannot be fragmented\n");
        error = CLOSE_ABNORMAL;
    }

    // Check that frame is not too large
    else if ( frame->payloadLength > FRAME_SIZE )
    {
        log_error("Protocol Error: Control frames cannot be fragmented\n");
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



/**
 * Creates an websocket frame.
 *
 * @param   opcode      [int   ]           frame opcode
 * @param   payload     [char *]           frame payload message
 * @param   payload_len [char *]           frame payload message length
 * @param   buf         [char *]           frame output buffer
 * @param   size        [int   ]           frame output buffer size
 * @return              [int   ]           frame output bytes"
 */
int wss_create_frame(int opcode, char *payload, size_t payload_len, char *buf, size_t size)
{
    wss_frame_head_t   *frame = (wss_frame_head_t *)buf;
    int                 offset = 0;

    if( !buf )
    {
        log_error("Invalid input arguments\n");
        return 0;
    }

    if( size < payload_len + sizeof(wss_frame_head_t) )
    {
        log_error("frame output buffer too small [%d]<%d\n", size, payload_len+10);
    }

    log_trace("Creating opcode[%d] frame\n", opcode);

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

/**
 * Converts the unsigned 64 bit integer from host byte order to network byte
 * order.
 *
 * @param   value  [uint64_t]   "A 64 bit unsigned integer"
 * @return         [uint64_t]   "The 64 bit unsigned integer in network byte order"
 */
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

void send_ping_frame(struct bufferevent *bev)
{
    char frame_buf[10]; 
    int frame_size;

    frame_size = wss_create_frame(PING_FRAME, NULL, 0, frame_buf, sizeof(frame_buf));
    if (frame_size > 0)
	{
        if (bufferevent_write(bev, frame_buf, frame_size) < 0)
		{
            log_error("Error writing ping frame to bufferevent: %s\n", strerror(errno));
        }
    } 
	else
	{
        log_error("Failed to create ping frame\n");
    }
}

