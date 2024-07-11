#ifndef wss_frame_h
#define wss_frame_h

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "wss.h"

#define FRAME_SIZE       3072
#define FRAMES_MAX       10

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

typedef enum {
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

/**
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-------+-+-------------+-------------------------------+
 *   |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 *   |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 *   |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 *   | |1|2|3|       |K|             |                               |
 *   +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 *   |     Extended payload length continued, if payload len == 127  |
 *   + - - - - - - - - - - - - - - - +-------------------------------+
 *   |                               |Masking-key, if MASK set to 1  |
 *   +-------------------------------+-------------------------------+
 *   | Masking-key (continued)       |          Payload Data         |
 *   +-------------------------------- - - - - - - - - - - - - - - - +
 *   :                     Payload Data continued ...                :
 *   + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 *   |                     Payload Data continued ...                |
 *   +---------------------------------------------------------------+
 */


typedef enum {
    CONTINUATION_FRAME = 0x0,
    TEXT_FRAME         = 0x1,
    BINARY_FRAME       = 0x2,
    CLOSE_FRAME        = 0x8,
    PING_FRAME         = 0x9,
    PONG_FRAME         = 0xA,
} wss_opcode_t;

typedef struct {
    unsigned short         opcode:4;
    unsigned short           rsv3:1;
    unsigned short           rsv2:1;
    unsigned short           rsv1:1;
    unsigned short            fin:1;

    unsigned short    payload_len:7;
    unsigned short           mask:1;
} wss_frame_head_t;

typedef struct {
    bool fin;
    bool rsv1;
    bool rsv2;
    bool rsv3;
    uint8_t opcode;
    bool mask;
    uint64_t payloadLength;
    char maskingKey[4];
    char *payload;
    uint64_t extDataLength;
    uint64_t appDataLength;
} wss_frame_t;


extern void do_parser_frames(wss_session_t *session);

/**
 * Parses a payload of data into a websocket frame. Returns the frame and
 * corrects the offset pointer in order for multiple frames to be processed
 * from the same payload.
 *
 * @param   payload         [char *]           "The payload to be processed"
 * @param   payload_length  [size_t]           "The length of the payload"
 * @param   offset          [size_t *]         "A pointer to an offset"
 * @return                  [wss_frame_t *]    "A websocket frame"
 */
wss_frame_t *wss_parse_frame(char *payload, size_t payload_length, size_t *offset);


/**
 * check the validation of the websocket frame.
 *
 * @param   frame       [wss_frame_t *]    "A websocket frame"
 * @param   reason      [wss_close_t *]    "Invalid reason"
 * @return              [int   ]           0: Invalid   1: Valid"
 */
wss_close_t wss_validate_frame(wss_frame_t *frame, wss_close_t *reason);

/**
 * Releases memory used by a frame.
 *
 * @param   ping     [wss_frame_t *]    "The frame that should be freed"
 * @return           [void]
 */
void wss_free_frame(wss_frame_t *frame);


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
int wss_create_frame(int opcode, char *payload, size_t payload_len, char *buf, size_t size);


char *wss_closing_string(wss_close_t reason);

/* Create a websocket text frame message. */
#define FRAME_MSG_ACK            "ACK"
#define FRAME_MSG_NAK            "NAK"
#define wss_create_text_frame(buf, size, msg)     wss_create_frame(TEXT_FRAME, msg, strlen(msg), buf, size)

/* Create a websocket close frame message. */
#define wss_create_close_frame(buf, size, reason) \
        wss_create_frame(CLOSE_FRAME, wss_closing_string(reason), strlen(wss_closing_string(reason)), buf, size)

#endif
