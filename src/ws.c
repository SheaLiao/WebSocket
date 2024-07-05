/*********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<2928382441@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  ws.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(2024年07月01日)
 *         Author:  Liao Shengli <2928382441@qq.com>
 *      ChangeLog:  1, Release initial version on "2024年07月01日 19时40分52秒"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <ctype.h>

#include <event2/util.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>

#include "ws.h"
#include "logger.h"
#include "b64.h"
#include "sha1.h"
#include "index_html.h"

static inline char *trim(char* str);
static inline void header_set_version(wss_header_t *header, char *v);
static size_t base64_encode_sha1(char *key, size_t key_length, char **accept_key);

/**
 * Performs a websocket handshake with the client session
 *
 * @param   session     [wss_session_t *]   "The client session"
 * @return              [void]
 */
void do_wss_handshake(wss_session_t *session)
{
    int                         bytes;
    char                        buf[2048] = {0};
    enum HttpStatus_Code        code;
    struct evbuffer            *src;
    struct bufferevent         *bev = session->bev;
    wss_header_t               *header = &session->header;

    log_info("Doing websocket handshake\n");

    src = bufferevent_get_input(bev);
    bytes = evbuffer_get_length(src);

    bytes = evbuffer_remove(src, buf, sizeof(buf));
    header->content = buf;
    header->length = bytes;

    log_debug("Parser [%d] bytes data from client %s:\n%s\n", bytes, session->client, buf);

    code = wss_parser_header(header);
    log_info("Client header parsed HTTP status code: [%d]\n", code);

    if( REQ_HTTP == header->type )
    {
        http_response(bev, code);
        return ;
    }

    // Create Upgrade HTTP header based on clients header
    code = wss_upgrade_header(header);
    switch (code) {
        case HttpStatus_UpgradeRequired:
            log_error("Rejecting HTTP request as the service requires use of the Websocket protocol.\n");
            upgrade_response(bev, code, "This service requires use of the Websocket protocol.");
            break;

        case HttpStatus_NotImplemented:
            log_error("Rejecting HTTP request as Websocket protocol is not yet implemented.\n");
            http_response(bev, code);
            break;

        case HttpStatus_SwitchingProtocols:
            handshake_response(bev, header, code);
            break;

        default:
            log_error("Rejecting HTTP request as server was unable to parse http header as websocket request.\n");
            http_response(bev, code);
            break;
    }

    session->handshaked = 1;
    return ;
}

enum HttpStatus_Code wss_parser_header(wss_header_t *header)
{
    char                  *token = NULL;
    char                  *lineptr, *temp, *line, *sep;
    char                  *tokenptr = NULL, *sepptr = NULL;

    log_info("Starting parsing header received from client.\n");

    /* Checking request is websocket or http */
    if( strstr(header->content, "WebSocket") )
        header->type = REQ_WS;
    else
        header->type = REQ_HTTP;

    /* Process the first line of the request message by \r\n  */
    token = strtok_r(header->content, "\r", &tokenptr);
    if (!token || tokenptr[0] != '\n'){
        log_error("Found invalid first line of header\n");
        header->type = REQ_FRAME; /* Maybe frame package  */
        return HttpStatus_BadRequest;
    }
    tokenptr++;

    /* Receiving and checking method of the request */
    if ( !(header->method=strtok_r(token, " ", &lineptr)) ) {
        log_error("Unable to parse header method\n");
        return HttpStatus_BadRequest;
    } else if( strncmp(header->method, "GET", 3) ) {
        log_error("Method that the client is using is unknown.\n");
        return HttpStatus_BadRequest;
    }

    /* Receiving and checking path of the request */
    if ( !(header->path=strtok_r(NULL, " ", &lineptr)) ) {
        log_error("Unable to parse header path\n");
        return HttpStatus_BadRequest;
    } else if ( strncmp("/", header->path, 1) != 0 &&
            strncasecmp("ws://", header->path, 5) != 0 &&
            strncasecmp("http://", header->path, 7) != 0 ) {
        log_error("The request URI is not absolute URI nor relative path\n");
        return HttpStatus_NotFound;
    }

    if( strstr(header->path, "favicon.ico") )
    {
        log_error("favicon service not being available\n");
        return HttpStatus_NotImplemented;
    }

    /* Receiving and checking version of the request */
    if ( !(header->version = strtok_r(NULL, " ", &lineptr)) ) {
        log_error("Unable to parse header version\n");
        return HttpStatus_BadRequest;
    } else if( strncmp(header->version, "HTTP/1.1", 8) ) {
        log_error("HTTP version that the client is using is unknown\n");
        return HttpStatus_BadRequest;
    }

    /* We've reached the payload */
    if ( strlen(token) >= 2 && tokenptr[0] == '\r' && tokenptr[1] == '\n' ) {
        tokenptr += 2;

        if ( strlen(tokenptr) > SIZE_PAYLOAD ) {
            log_error("Payload size received is too large\n");
            return HttpStatus_BadRequest;
        }

        header->payload = tokenptr;
        token = NULL;
    } else {
        token = strtok_r(NULL, "\r", &tokenptr);

        if ( tokenptr[0] == '\n' ) {
            tokenptr++;
        } else if ( tokenptr[0] != '\0' ) {
            log_error("Line shall always end with newline character\n");
            return HttpStatus_BadRequest;
        }
    }

    while ( token ) {
        log_trace("token: %s\n", token);

        line = strtok_r(token, ":", &lineptr);

        if( strlen(token)> SIZE_HEADER ) {
            log_error("Header size received is too large.");
            return HttpStatus_BadRequest;
        };

        /* start parser the header lines */
        if ( line ) {
            if( !strcasecmp(line,"Host") ) {
                header->host= (strtok_r(NULL, "", &lineptr)+1);
                log_debug("Host: %s\n", header->host);

            } else if( !strcasecmp(line,"Connection") ) {
                header->ws_connection = (strtok_r(NULL, "", &lineptr)+1);
                log_debug("Connection: %s\n", header->ws_connection);

            } else if( !strcasecmp(line,"Upgrade") ) {
                header->ws_upgrade = (strtok_r(NULL, "", &lineptr)+1);
                log_debug("Upgrade: %s\n", header->ws_upgrade);

            } else if( !strcasecmp(line,"Origin") ) {
                header->ws_origin = (strtok_r(NULL, "", &lineptr)+1);
                log_debug("Origin: %s\n", header->ws_origin);

            } else if( !strcasecmp(line,"Sec-WebSocket-Version") ) {
                sep = trim(strtok_r(strtok_r(NULL, "", &lineptr), ",", &sepptr));
                while (NULL != sep) {
                    header_set_version(header, sep);
                    sep = trim(strtok_r(NULL, ",", &sepptr));
                }
                log_debug("Sec-WebSocket-Version: %d\n", header->ws_version);

            } else if( !strcasecmp(line,"Sec-WebSocket-Key") ) {
                header->ws_key = (strtok_r(NULL, "", &lineptr)+1);
                log_debug("Sec-WebSocket-Key: %s\n", header->ws_key);

            } else if( !strcasecmp(line,"Sec-WebSocket-Key1") ) {
                header->ws_key1 = (strtok_r(NULL, "", &lineptr)+1);
                header->ws_type = HIXIE76;
                log_debug("Sec-WebSocket-Key1: %s\n", header->ws_key1);

            } else if( !strcasecmp(line,"Sec-WebSocket-Key2") ) {
                header->ws_key2 = (strtok_r(NULL, "", &lineptr)+1);
                header->ws_type = HIXIE76;
                log_debug("Sec-WebSocket-Key2: %s\n", header->ws_key2);
            }

            /* RFU for other parser */
        };

        // We've reached the payload
        if ( strlen(tokenptr)>=2 && tokenptr[0]=='\r' && tokenptr[1]=='\n' ) {
            tokenptr += 2;

            if ( strlen(tokenptr) > SIZE_PAYLOAD ) {
                log_error("Payload size received is too large\n");
                return HttpStatus_BadRequest;
            }

            header->payload = tokenptr;
            temp = NULL;
        } else {
            temp = strtok_r(NULL, "\r", &tokenptr);

            if ( tokenptr[0]=='\n' ) {
                tokenptr++;
            } else if ( tokenptr[0] != '\0' ) {
                log_error("Line shall always end with newline character\n");
                return HttpStatus_BadRequest;
            }
        }

        if ( temp==NULL && header->ws_type==HIXIE76 ) {
            header->ws_type = HIXIE76;
            header->ws_key3 = header->payload;
        }

        token = temp;
    }

    if ( header->ws_type == UNKNOWN
            && header->ws_version == 0
            && header->ws_key1 == NULL
            && header->ws_key2 == NULL
            && header->ws_key3 == NULL
            && header->ws_key == NULL
            && header->ws_upgrade != NULL
            && strcasecmp(header->ws_upgrade, "websocket") == 0
            && header->ws_connection != NULL
            && strcasecmp(header->ws_connection, "Upgrade") == 0
            && header->host != NULL
            && header->ws_origin != NULL ) {
        header->ws_type = HIXIE75;
    }

    return HttpStatus_OK;
}

enum HttpStatus_Code wss_upgrade_header(wss_header_t *header)
{
    unsigned char *key = NULL;
    unsigned long key_length;

    log_info("start upgrade websocket header\n");

    log_debug("Validating upgrade header\n");
    if ( !header->ws_upgrade || strcasecmp(header->ws_upgrade, "websocket") || (header->ws_type<=HIXIE76 && strcasecmp(header->ws_upgrade, "WebSocket")) )
    {
        log_error("Invalid upgrade header value\n");
        return HttpStatus_UpgradeRequired;
    }

    log_debug("Validating connection header\n");
    if ( !header->ws_connection || !strstr(header->ws_connection, "Upgrade") ) {
        log_error("Invalid upgrade header value\n");
        return HttpStatus_UpgradeRequired;
    }

    log_debug("Validating websocket version\n");
    switch (header->ws_type) {
        case RFC6455:
        case HYBI10:
        case HYBI07:
            break;
        default:
        log_error("Invalid websocket version\n");
        return HttpStatus_NotImplemented;
    }

    log_debug("Validating websocket key\n");

    if ( !header->ws_key ) {
        log_error("Invalid websocket key\n");
        return HttpStatus_UpgradeRequired;
    }

    key = b64_decode_ex(header->ws_key, strlen(header->ws_key), &key_length);
    if ( !key || key_length != SEC_WEBSOCKET_KEY_LENGTH ) {
        log_error("Invalid websocket key\n");
        free(key);
        return HttpStatus_UpgradeRequired;
    }

    free(key);

    log_info("Accepted handshake, switching protocol\n");
    return HttpStatus_SwitchingProtocols;
}

void http_response(struct bufferevent *bev, enum HttpStatus_Code code)
{
    char                   buf[4096] = {0x00};
    char                  *message = buf;
    const char            *reason  = HttpStatus_reasonPhrase(code);
    int                    oft = 0;
    int                    bodylen = 0;

    /* HTTP version header */
    oft = sprintf(message+oft, HTTP_STATUS_LINE, code, reason);

    if( code != HttpStatus_OK )
    {
        log_info("Serving HTTP error code[%d] page to the client\n", code);

        /* HTTP body length */
        bodylen += strlen(HTTP_BODY) - 6; /* minus: %d %s %s  */
        bodylen += 3; /* HttpStatus_Code should be 3 bytes: 200, 404  */
        bodylen += 2*strlen(reason);

        /* HTTP HTML header */
        oft += sprintf(message+oft, HTTP_HTML_HEADERS, bodylen, WSS_SERVER_VERSION);

        /* HTTP body */
        sprintf(message+oft, HTTP_BODY, code, reason, reason);

        /* send response */
        write(bufferevent_getfd(bev), buf, strlen(buf));
    }
    else
    {
        log_info("Serving default HTTP index page to the client\n");

        /* HTTP body length */
        bodylen += strlen(g_index_html);

        /* HTTP HTML header */
        oft += sprintf(message+oft, HTTP_HTML_HEADERS, bodylen, WSS_SERVER_VERSION);

        /* send response HTML header */
        write(bufferevent_getfd(bev), buf, strlen(buf));

        /* send response Web page */
        write(bufferevent_getfd(bev), g_index_html, strlen(g_index_html));
    }

    bufferevent_free(bev);

    return ;
}

void upgrade_response(struct bufferevent *bev, enum HttpStatus_Code code, char *exp)
{
    char                   buf[4096] = {0x00};
    char                  *message = buf;
    const char            *reason  = HttpStatus_reasonPhrase(code);
    int                    oft = 0;
    int                    bodylen = 0;

    log_info("Serving HTTP error code[%d] page to the client\n", code);

    /* HTTP version header */
    oft = sprintf(message+oft, HTTP_STATUS_LINE, code, reason);

    /* HTTP body is the explain message */
    bodylen += strlen(exp);

    /* HTTP HTML header */
    oft += sprintf(message+oft, HTTP_UPGRADE_HEADERS, bodylen, WSS_SERVER_VERSION);

    /* HTTP body is the explain message */
    sprintf(message+oft, "%s", exp);

    /* send response */
    write(bufferevent_getfd(bev), buf, strlen(buf));

    bufferevent_free(bev);

    return ;
}


/**
 * Function that generates a handshake response, used to authorize a websocket
 * session.
 *
 * @param   header  [wss_header_t*]         "The http header obtained from the session"
 * @param   code    [enum HttpStatus_Code]  "The http status code to return"
 * @return          [wss_message_t *]       "A message structure that can be passed through ringbuffer"
 */
void handshake_response(struct bufferevent *bev, wss_header_t *header, enum HttpStatus_Code code)
{
    char                   buf[4096] = {0x00};
    char                  *message = buf;
    const char            *reason = HttpStatus_reasonPhrase(code);
    size_t                 oft = 0;
    char                   key[128];
    char                  *acceptKey;
    size_t                 acceptKeyLength;

    /* Generate accept key */
    memset(key, 0, sizeof(key));
    snprintf(key, sizeof(key), "%s%s", header->ws_key, MAGIC_WEBSOCKET_KEY);

    acceptKeyLength = base64_encode_sha1(key, strlen(key), &acceptKey);
    if (acceptKeyLength == 0) {
        free(acceptKey);
        return ;
    }

    /* HTTP version header */
    oft += sprintf(message+oft, HTTP_STATUS_LINE, code, reason);

    /* Websocket handshake accept */
    oft += sprintf(message+oft, HTTP_HANDSHAKE_ACCEPT);
    memcpy(message+oft, acceptKey, acceptKeyLength);
    free(acceptKey);
    oft += acceptKeyLength;
    sprintf(message+oft, "\r\n");
    oft += 2;

    /* Websocket handshake header */
    oft += sprintf(message+oft, HTTP_HANDSHAKE_HEADERS, WSS_SERVER_VERSION);

    log_debug("Handshake Response: \n%s\n", message);

    /* send response */
    write(bufferevent_getfd(bev), buf, strlen(buf));

    return ;
}


/**
 * Function creates a sha1 hash of the key and base64 encodes it.
 *
 * @param   key         [char *]                "The key to be hashed"
 * @param   key_length  [size_t]                "The length of the key"
 * @param   accept_key  [char **]               "A pointer to where the hash should be stored"
 * @return              [size_t]                "Length of accept_key"
 */
static size_t base64_encode_sha1(char *key, size_t key_length, char **accept_key)
{
    int                    i, b;
    size_t                 acceptKeyLength;
    char                   sha1Key[SHA_DIGEST_LENGTH];

    memset(sha1Key, '\0', SHA_DIGEST_LENGTH);
    SHA1Context sha;

    SHA1Reset(&sha);
    SHA1Input(&sha, (const unsigned char*) key, key_length);
    if ( SHA1Result(&sha) ) {
        for (i = 0; i<5; i++) {
            b = htonl(sha.Message_Digest[i]);
            memcpy(sha1Key+(4*i), (unsigned char *) &b, 4);
        }
    } else {
        return 0;
    }

    *accept_key = b64_encode((const unsigned char *) sha1Key, SHA_DIGEST_LENGTH);
    acceptKeyLength = strlen(*accept_key);

    return acceptKeyLength;
}

/**
 * Trims a string for leading and trailing whitespace.
 *
 * @param   str     [char *]    "The string to trim"
 * @return          [char *]    "The input string without leading and trailing spaces"
 */
static inline char *trim(char* str)
{
    if ( !str ) {
        return NULL;
    }

    if ( str[0] == '\0' ) {
        return str;
    }
    int start, end = strlen(str);
    for (start = 0; isspace(str[start]); ++start) {}
    if (str[start]) {
        while (end > 0 && isspace(str[end-1]))
            --end;
        memmove(str, &str[start], end - start);
    }
    str[end - start] = '\0';

    return str;
}

static inline void header_set_version(wss_header_t *header, char *v)
{

    int version = strtol(strtok_r(NULL, "", &v), (char **) NULL, 10);

    if ( header->ws_version < version ) {
        if ( version == 4 ) {
            header->ws_type = HYBI04;
            header->ws_version = version;
        } else if( version == 5 ) {
            header->ws_type = HYBI05;
            header->ws_version = version;
        } else if( version == 6 ) {
            header->ws_type = HYBI06;
            header->ws_version = version;
        } else if( version == 7 ) {
            header->ws_type = HYBI07;
            header->ws_version = version;
        } else if( version == 8 ) {
            header->ws_type = HYBI10;
            header->ws_version = version;
        } else if( version == 13 ) {
            header->ws_type = RFC6455;
            header->ws_version = version;
        }
    }
}
