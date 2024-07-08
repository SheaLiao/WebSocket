/*********************************************************************************
 *      Copyright:  (C) 2022 GuoWenxue <guowenxue@gmail.com>
 *                  All rights reserved.
 *
 *       Filename:  wss.h
 *    Description:  This file is a websocket Protocol API.
 *
 *        Version:  1.0.0(10/09/22)
 *         Author:  Guo Wenxue <guowenxue@gmail.com>
 *      ChangeLog:  1, Release initial version on "10/09/22 15:11:36"
 *
 ********************************************************************************/

#ifndef  _WSS_H_
#define  _WSS_H_

#define WSS_SERVER_VERSION "v1.0.0"


#define SHA_DIGEST_LENGTH           20
#define MAGIC_WEBSOCKET_KEY         "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define HTTP_STATUS_LINE            "HTTP/1.1 %d %s\r\n"
#define HTTP_HTML_HEADERS           "Content-Type: text/html; charset=utf-8\r\n"\
                                    "Content-Length: %d\r\n"\
                                    "Connection: Closed\r\n"\
                                    "Server: WSServer/%s\r\n"\
                                    "\r\n"
#define HTTP_WS_VERSION_HEADER      "Sec-WebSocket-Version: %d, %d, %d\r\n"
#define HTTP_UPGRADE_HEADERS        "Content-Type: text/plain\r\n"\
                                    "Content-Length: %d\r\n"\
                                    "Connection: Upgrade\r\n"\
                                    "Upgrade: websocket\r\n"\
                                    "Sec-WebSocket-Version: 13\r\n"\
                                    "Server: WSServer/%s\r\n"\
                                    "\r\n"

#define HTTP_HANDSHAKE_EXTENSIONS   "Sec-Websocket-Extensions: "
#define HTTP_HANDSHAKE_SUBPROTOCOL  "Sec-Websocket-Protocol: "
#define HTTP_HANDSHAKE_ACCEPT       "Sec-Websocket-Accept: "
#define HTTP_HANDSHAKE_HEADERS      "Upgrade: websocket\r\n"\
                                    "Connection: Upgrade\r\n"\
                                    "Server: WSServer/%s\r\n"\
                                    "\r\n"

#define HTTP_BODY                   "<!doctype html>"\
                                    "<html lang=\"en\">"\
                                    "<head>"\
                                    "<meta charset=\"utf-8\">"\
                                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">"\
                                    "<title>%d %s</title>"\
                                    "</head>"\
                                    "<body>"\
                                    "<h1>%s</h1>"\
                                    "</body>"\
                                    "</html>"

#define SEC_WEBSOCKET_KEY_LENGTH    16

typedef enum {
    UNKNOWN  = 0,
    HIXIE75  = 1,
    HIXIE76  = 2,
    HYBI04   = 4,
    HYBI05   = 5,
    HYBI06   = 6,
    HYBI07   = 7,
    HYBI10   = 8,
    RFC6455  = 13
} wss_type_t;

typedef enum {
    REQ_HTTP = 0, /* request for http */
    REQ_WS   = 1, /* request for websocket */
    REQ_FRAME= 2, /* websocket frame */
} head_type_t;

#define SIZE_PAYLOAD   4096
#define SIZE_HEADER    1024
typedef struct {
    char *content;            // 存储请求或响应的内容
    unsigned int length;      // 内容的长度
    head_type_t type;         // 头部类型（请求类型），例如 HTTP 请求、WebSocket 请求或 WebSocket 帧
    char *method;             // HTTP 请求方法，例如 GET 或 POST
    char *version;            // HTTP 版本，例如 HTTP/1.1
    char *path;               // 请求路径，例如 /index.html
    char *host;               // 主机名，例如 www.example.com
    char *payload;            // 请求或响应的有效负载（数据部分）
    int ws_version;           // WebSocket 版本
    wss_type_t ws_type;       // WebSocket 类型，例如 RFC6455
    char *ws_upgrade;         // WebSocket 升级头部字段
    char *ws_connection;      // WebSocket 连接头部字段
    char *ws_origin;          // WebSocket 原点头部字段
    char *ws_key;             // WebSocket 密钥，用于握手验证
    
    /*Hixie-76 协议特有的密钥字段，用于早期的 WebSocket 握手协议*/ 
    char *ws_key1;            // Hixie-76 协议的第一个密钥
    char *ws_key2;            // Hixie-76 协议的第二个密钥
    char *ws_key3;            // Hixie-76 协议的第三个密钥（8 字节）
} wss_header_t;

#define HOSTN_LEN   64
typedef struct wss_session_s
{
    char                 client[HOSTN_LEN];
    struct event_base	*base;
	struct bufferevent *recv_bev;  // 接收bufferevent
    struct bufferevent *send_bev;  // 发送bufferevent
	//struct bufferevent  *bev;
    wss_header_t         header;
    unsigned char        handshaked;
} wss_session_t;


/*! Enum for the HTTP status codes.
*/
enum HttpStatus_Code
{
    /*####### 1xx - Informational #######*/
    /* Indicates an interim response for communicating connection status
     * or request progress prior to completing the requested action and
     * sending a final response.
     */
    HttpStatus_SwitchingProtocols = 101, /*!< Indicates that the server understands and is willing to comply with the client's request, via the Upgrade header field. */

    /*####### 2xx - Successful #######*/
    /* Indicates that the client's request was successfully received,
     * understood, and accepted.
     */
    HttpStatus_OK                          = 200, /*!< Indicates that the request has succeeded. */

    /*####### 3xx - Redirection #######*/
    /* Indicates that further action needs to be taken by the user agent
     * in order to fulfill the request.
     */
    /* RFU */

    /*####### 4xx - Client Error #######*/
    /* Indicates that the client seems to have erred.
    */
    HttpStatus_BadRequest                  = 400, /*!< Indicates that the server cannot or will not process the request because the received syntax is invalid, nonsensical, or exceeds some limitation on the server. */
    HttpStatus_NotFound                    = 404, /*!< Indicates that the origin server did not find a current representation for the target resource or is not willing to disclose that one exists. */
    HttpStatus_UpgradeRequired             = 426, /*!< Indicates that the server refuses to perform the request using the current protocol but might be willing to do so after the client upgrades to a different protocol. */

    /*####### 5xx - Server Error #######*/
    /* Indicates that the server is aware that it has erred
     * or is incapable of performing the requested method.
     */
    HttpStatus_InternalServerError           = 500, /*!< Indicates that the server encountered an unexpected condition that prevented it from fulfilling the request. */
    HttpStatus_NotImplemented                = 501, /*!< Indicates that the server does not support the functionality required to fulfill the request. */

    HttpStatus_xxx_max = 1023
};

/*! Returns the standard HTTP reason phrase for a HTTP status code.
 * \param code An HTTP status code.
 * \return The standard HTTP reason phrase for the given \p code or \c NULL if no standard
 * phrase for the given \p code is known.
 */
static inline const char* HttpStatus_reasonPhrase(int code)
{
    switch (code)
    {

        /*####### 1xx - Informational #######*/
        case 101: return "Switching Protocols";

        /*####### 2xx - Successful #######*/
        case 200: return "OK";

        /*####### 3xx - Redirection #######*/
        /* RFU */

        /*####### 4xx - Client Error #######*/
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 426: return "Upgrade To Websocket Required";

        /*####### 5xx - Server Error #######*/
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";

        default: return 0;
    }
}

extern enum HttpStatus_Code wss_parser_header(wss_header_t *header);
extern enum HttpStatus_Code wss_upgrade_header(wss_header_t *header);
extern void http_response(struct bufferevent *bev, enum HttpStatus_Code code);
extern void upgrade_response(struct bufferevent *bev, enum HttpStatus_Code code, char *exp);
extern void handshake_response(struct bufferevent *bev, wss_header_t *header, enum HttpStatus_Code code);
extern void do_wss_handshake(wss_session_t *session);

#endif   /* ----- #ifndef _WSS_H_  ----- */
