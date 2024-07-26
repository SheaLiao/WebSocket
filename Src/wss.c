/*
 * wss.c
 *
 *  Created on: Jul 21, 2024
 *      Author: asus
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "wss.h"
#include "b64.h"
#include "sha1.h"
#include "esp8266.h"
#include "index_html.h"
#include "usart.h"

#define CONFIG_DEBUG
#ifdef CONFIG_DEBUG
#define dbg_print(format, args...) printf(format,##args)
#else
#define dbg_print(format, args...) do{} while(0)
#endif

#define ntohs(x)  (((x) << 8) | ((x) >> 8))
#define htonl(x)  (((x) << 24) | (((x) & 0xFF00) << 8) | (((x) >> 8) & 0xFF00) | ((x) >> 24))


static inline char *trim(char* str);
static inline void header_set_version(wss_header_t *header, char *v);
static size_t base64_encode_sha1(char *key, size_t key_length, char **accept_key);


void do_wss_handshake(int *sockfd, wss_session_t *session)
{
    int                         bytes;
    enum HttpStatus_Code        code;
    wss_header_t               *header = &session->header;
    uint8_t buffer[UART2_BUFFER_SIZE];
	size_t read_len = 0;

	uart2_ring_buffer_read(buffer);
	printf("g_uart2_ringbuf:%s\r\n", g_uart2_ringbuf);
	printf("buffer:%s\r\n", buffer);

//    while (read_len < sizeof(buffer) && ring_buffer_read(&g_uart2_ringbuf, &buffer[read_len]) == 0)
//	{
//		read_len++;
//	}
//
//	if (read_len <= 0)
//	{
//		dbg_print("Ws failed to receive data from ESP8266\n");
//		return;
//	}

	// 将缓冲区数据赋值给 WebSocket 头
	header->content = (char *)buffer;
	header->length = read_len;

	dbg_print("Parser [%d] bytes data from client %s:\n%s\n", read_len, session->client, buffer);

    code = wss_parser_header(header);
    dbg_print("Client header parsed HTTP status code: [%d]\n", code);

    if (REQ_HTTP == header->type)
    {
        http_response(sockfd, code);
        return;
    }

    // Create Upgrade HTTP header based on clients header
    code = wss_upgrade_header(header);
    switch (code)
    {
        case HttpStatus_UpgradeRequired:
        	dbg_print("Rejecting HTTP request as the service requires use of the Websocket protocol.\n");
            upgrade_response(sockfd, code, "This service requires use of the Websocket protocol.");
            break;

        case HttpStatus_NotImplemented:
        	dbg_print("Rejecting HTTP request as Websocket protocol is not yet implemented.\n");
            http_response(sockfd, code);
            break;

        case HttpStatus_SwitchingProtocols:
            handshake_response(sockfd, header, code);
            break;

        default:
        	dbg_print("Rejecting HTTP request as server was unable to parse http header as websocket request.\n");
            http_response(sockfd, code);
            break;
    }

    session->handshaked = 1;
    return;
}




enum HttpStatus_Code wss_parser_header(wss_header_t *header)
{
    char                  *token = NULL;
    char                  *lineptr, *temp, *line, *sep;
    char                  *tokenptr = NULL, *sepptr = NULL;

    dbg_print("Starting parsing header received from client.\n");

    /* Checking request is websocket or http */
    if( strstr(header->content, "WebSocket") )
        header->type = REQ_WS;
    else
        header->type = REQ_HTTP;


    /* Process the first line of the request message by \r\n  */
    token = strtok_r(header->content, "\r", &tokenptr);

	if (!token || tokenptr[0] != '\n'){
		dbg_print("Found invalid first line of header\n");
        header->type = REQ_FRAME; /* Maybe frame package  */
        return HttpStatus_BadRequest;
    }
    tokenptr++;

    /* Receiving and checking method of the request */
    if ( !(header->method=strtok_r(token, " ", &lineptr)) ) {
    	dbg_print("Unable to parse header method\n");
        return HttpStatus_BadRequest;
    } else if( strncmp(header->method, "GET", 3) ) {
    	dbg_print("Method that the client is using is unknown.\n");
        return HttpStatus_BadRequest;
    }

    /* Receiving and checking path of the request */
    if ( !(header->path=strtok_r(NULL, " ", &lineptr)) ) {
    	dbg_print("Unable to parse header path\n");
        return HttpStatus_BadRequest;
    } else if ( strncmp("/", header->path, 1) != 0 &&
            strncasecmp("ws://", header->path, 5) != 0 &&
            strncasecmp("http://", header->path, 7) != 0 ) {
    	dbg_print("The request URI is not absolute URI nor relative path\n");
        return HttpStatus_NotFound;
    }


    if( strstr(header->path, "favicon.ico") )
    {
    	dbg_print("favicon service not being available\n");
        return HttpStatus_NotImplemented;
    }

    /* Receiving and checking version of the request */
    if ( !(header->version = strtok_r(NULL, " ", &lineptr)) ) {
    	dbg_print("Unable to parse header version\n");
        return HttpStatus_BadRequest;
    } else if( strncmp(header->version, "HTTP/1.1", 8) ) {
    	dbg_print("HTTP version that the client is using is unknown\n");
        return HttpStatus_BadRequest;
    }

    /* We've reached the payload */
    if ( strlen(token) >= 2 && tokenptr[0] == '\r' && tokenptr[1] == '\n' ) {
        tokenptr += 2;

        if ( strlen(tokenptr) > SIZE_PAYLOAD ) {
        	dbg_print("Payload size received is too large\n");
            return HttpStatus_BadRequest;
        }


        header->payload = tokenptr;
        token = NULL;
    } else {
        token = strtok_r(NULL, "\r", &tokenptr);

        if ( tokenptr[0] == '\n' ) {
            tokenptr++;
        } else if ( tokenptr[0] != '\0' ) {
        	dbg_print("Line shall always end with newline character\n");
            return HttpStatus_BadRequest;
        }
    }

    while ( token ) {
    	dbg_print("token: %s\n", token);

        line = strtok_r(token, ":", &lineptr);

        if( strlen(token)> SIZE_HEADER ) {
        	dbg_print("Header size received is too large.");
            return HttpStatus_BadRequest;
        };

        /* start parser the header lines */
        if ( line ) {
            if( !strcasecmp(line,"Host") ) {
                header->host= (strtok_r(NULL, "", &lineptr)+1);
                dbg_print("Host: %s\n", header->host);

            } else if( !strcasecmp(line,"Connection") ) {
                header->ws_connection = (strtok_r(NULL, "", &lineptr)+1);
                dbg_print("Connection: %s\n", header->ws_connection);

            } else if( !strcasecmp(line,"Upgrade") ) {
                header->ws_upgrade = (strtok_r(NULL, "", &lineptr)+1);
                dbg_print("Upgrade: %s\n", header->ws_upgrade);

            } else if( !strcasecmp(line,"Origin") ) {
                header->ws_origin = (strtok_r(NULL, "", &lineptr)+1);
                dbg_print("Origin: %s\n", header->ws_origin);

            } else if( !strcasecmp(line,"Sec-WebSocket-Version") ) {
                sep = trim(strtok_r(strtok_r(NULL, "", &lineptr), ",", &sepptr));
                while (NULL != sep) {
                    header_set_version(header, sep);
                    sep = trim(strtok_r(NULL, ",", &sepptr));
                }
                dbg_print("Sec-WebSocket-Version: %d\n", header->ws_version);

            } else if( !strcasecmp(line,"Sec-WebSocket-Key") ) {
                header->ws_key = (strtok_r(NULL, "", &lineptr)+1);
                dbg_print("Sec-WebSocket-Key: %s\n", header->ws_key);

            } else if( !strcasecmp(line,"Sec-WebSocket-Key1") ) {
                header->ws_key1 = (strtok_r(NULL, "", &lineptr)+1);
                header->ws_type = HIXIE76;
                dbg_print("Sec-WebSocket-Key1: %s\n", header->ws_key1);

            } else if( !strcasecmp(line,"Sec-WebSocket-Key2") ) {
                header->ws_key2 = (strtok_r(NULL, "", &lineptr)+1);
                header->ws_type = HIXIE76;
                dbg_print("Sec-WebSocket-Key2: %s\n", header->ws_key2);
            }

            /* RFU for other parser */
        };

        // We've reached the payload
        if ( strlen(tokenptr)>=2 && tokenptr[0]=='\r' && tokenptr[1]=='\n' ) {
            tokenptr += 2;

            if ( strlen(tokenptr) > SIZE_PAYLOAD ) {
            	dbg_print("Payload size received is too large\n");
                return HttpStatus_BadRequest;
            }

            header->payload = tokenptr;
            temp = NULL;
        } else {
            temp = strtok_r(NULL, "\r", &tokenptr);

            if ( tokenptr[0]=='\n' ) {
                tokenptr++;
            } else if ( tokenptr[0] != '\0' ) {
            	dbg_print("Line shall always end with newline character\n");
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

    dbg_print("start upgrade websocket header\n");

    dbg_print("Validating upgrade header\n");
    if ( !header->ws_upgrade || strcasecmp(header->ws_upgrade, "websocket") || (header->ws_type<=HIXIE76 && strcasecmp(header->ws_upgrade, "WebSocket")) )
    {
    	dbg_print("Invalid upgrade header value\n");
        return HttpStatus_UpgradeRequired;
    }

    dbg_print("Validating connection header\n");
    if ( !header->ws_connection || !strstr(header->ws_connection, "Upgrade") ) {
    	dbg_print("Invalid upgrade header value\n");
        return HttpStatus_UpgradeRequired;
    }

    dbg_print("Validating websocket version\n");
    switch (header->ws_type) {
        case RFC6455:
        case HYBI10:
        case HYBI07:
            break;
        default:
        	dbg_print("Invalid websocket version\n");
        	return HttpStatus_NotImplemented;
    }

    dbg_print("Validating websocket key\n");

    if ( !header->ws_key ) {
    	dbg_print("Invalid websocket key\n");
        return HttpStatus_UpgradeRequired;
    }

    key = b64_decode_ex(header->ws_key, strlen(header->ws_key), &key_length);
    if ( !key || key_length != SEC_WEBSOCKET_KEY_LENGTH ) {
    	dbg_print("Invalid websocket key\n");
        free(key);
        return HttpStatus_UpgradeRequired;
    }

    free(key);

    dbg_print("Accepted handshake, switching protocol\n");
    return HttpStatus_SwitchingProtocols;
}


void http_response(int *sockfd, enum HttpStatus_Code code)
{
    char                   buf[4096] = {0x00};
    char                  *message = buf;
    const char            *reason = HttpStatus_reasonPhrase(code);
    int                    oft = 0;
    int                    bodylen = 0;

    /* HTTP version header */
    oft = sprintf(message + oft, HTTP_STATUS_LINE, code, reason);

    if (code != HttpStatus_OK)
    {
    	dbg_print("Serving HTTP error code[%d] page to the client\n", code);

        /* HTTP body length */
        bodylen += strlen(HTTP_BODY) - 6; /* minus: %d %s %s  */
        bodylen += 3; /* HttpStatus_Code should be 3 bytes: 200, 404  */
        bodylen += 2 * strlen(reason);

        /* HTTP HTML header */
        oft += sprintf(message + oft, HTTP_HTML_HEADERS, bodylen, WSS_SERVER_VERSION);

        /* HTTP body */
        sprintf(message + oft, HTTP_BODY, code, reason, reason);

        /* send response */
        esp8266_sock_send((unsigned char *)buf, *sockfd, strlen(buf));
    }
    else
    {
    	dbg_print("Serving default HTTP index page to the client\n");

        /* HTTP body length */
        bodylen += strlen(g_index_html);

        /* HTTP HTML header */
        oft += sprintf(message + oft, HTTP_HTML_HEADERS, bodylen, WSS_SERVER_VERSION);

        /* send response HTML header */
        esp8266_sock_send((unsigned char *)buf, *sockfd, strlen(buf));

        /* send response Web page */
        esp8266_sock_send((unsigned char *)g_index_html, *sockfd, strlen(g_index_html));
    }

    return;
}





void upgrade_response(int *sockfd, enum HttpStatus_Code code, char *exp)
{
    char                   buf[4096] = {0x00};
    char                  *message = buf;
    const char            *reason  = HttpStatus_reasonPhrase(code);
    int                    oft = 0;
    int                    bodylen = 0;

    /* HTTP version header */
    oft = sprintf(message + oft, HTTP_STATUS_LINE, code, reason);

    dbg_print("Serving HTTP error code[%d] page to the client\n", code);

    /* HTTP body length */
    bodylen += strlen(HTTP_BODY) - 6; /* minus: %d %s %s  */
    bodylen += 3; /* HttpStatus_Code should be 3 bytes: 200, 404  */
    bodylen += 2 * strlen(reason);

    /* HTTP HTML header */
    oft += sprintf(message + oft, HTTP_HTML_HEADERS, bodylen, WSS_SERVER_VERSION);

    /* HTTP body */
    sprintf(message + oft, HTTP_BODY, code, reason, reason);

    /* send response */
    esp8266_sock_send((unsigned char *)buf, *sockfd, strlen(buf));

    return;
}



void handshake_response(int *sockfd, wss_header_t *header, enum HttpStatus_Code code)
{
    char buf[4096];
    char *message = buf;
    const char *reason = HttpStatus_reasonPhrase(code);
    size_t oft = 0;
    char key[128];
    char *acceptKey = NULL;
    size_t acceptKeyLength;

    // Generate accept key
    memset(key, 0, sizeof(key));
    snprintf(key, sizeof(key), "%s%s", header->ws_key, MAGIC_WEBSOCKET_KEY);

    acceptKeyLength = base64_encode_sha1(key, strlen(key), &acceptKey);
    if (acceptKeyLength == 0)
    {
        dbg_print("Error generating base64-encoded SHA1 accept key\n");
        return;
    }

    // HTTP version header
    oft = snprintf(message + oft, sizeof(buf) - oft, HTTP_STATUS_LINE, code, reason);
    if (oft >= sizeof(buf))
    {
        dbg_print("Buffer size exceeded while creating HTTP status line\n");
        free(acceptKey);
        return;
    }

    // WebSocket handshake accept
    oft += snprintf(message + oft, sizeof(buf) - oft, HTTP_HANDSHAKE_ACCEPT);
    if (oft >= sizeof(buf) - acceptKeyLength)
    {
        dbg_print("Buffer size exceeded while adding accept key\n");
        free(acceptKey);
        return;
    }
    memcpy(message + oft, acceptKey, acceptKeyLength);
    oft += acceptKeyLength;
    snprintf(message + oft, sizeof(buf) - oft, "\r\n");
    oft += 2;

    // WebSocket handshake headers
    snprintf(message + oft, sizeof(buf) - oft, HTTP_HANDSHAKE_HEADERS, WSS_SERVER_VERSION);

    dbg_print("Handshake Response: \n%s\n", message);

    // Send response
    int rv = esp8266_sock_send((unsigned char *)buf, *sockfd, oft);
	if (rv < 0)
	{
		dbg_print("ERROR: Failed to send HTTP response, error code %d\n", rv);
	}


    free(acceptKey);
    return;
}






static size_t base64_encode_sha1(char *key, size_t key_length, char **accept_key)
{
    int                    i, b;
    size_t                 acceptKeyLength;
    char                   sha1Key[20];

    memset(sha1Key, '\0', 20);
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

    *accept_key = b64_encode((const unsigned char *) sha1Key, 20);
    acceptKeyLength = strlen(*accept_key);

    return acceptKeyLength;
}


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
