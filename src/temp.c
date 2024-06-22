/*********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<2928382441@qq.com>
 *                  All rights reserved.
 *
 *       Filename:  temp.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(2024年06月22日)
 *         Author:  Liao Shengli <2928382441@qq.com>
 *      ChangeLog:  1, Release initial version on "2024年06月22日 19时49分20秒"
 *                 
 ********************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <cjson/cJSON.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>


#include "ws.h"
#include "temp.h"
#include "logger.h"

float get_temperature() {
	float temp = 0.0;

	temp = rand() % 5 + 25;

    return temp;  
}

void send_temperature(ws_ctx_t *ws_ctx) {
    float temperature = get_temperature();

    // 构建 JSON 数据
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "temperature");
    cJSON_AddNumberToObject(root, "value", temperature);
    cJSON_AddNumberToObject(root, "timestamp", (double)time(NULL));

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str) {
        // 构建 WebSocket 帧
        size_t payload_len = strlen(json_str);
        size_t frame_size = sizeof(ws_ophdr_t) + payload_len;
        char *frame = (char *)malloc(frame_size);
        if (!frame) {
            free(json_str);
            return;
        }

        ws_ophdr_t *hdr = (ws_ophdr_t *)frame;
        hdr->fin = 1;
        hdr->rsv1 = 0;
        hdr->rsv2 = 0;
        hdr->rsv3 = 0;
        hdr->opcode = 0x1; // 文本帧
        hdr->mask = 0;
        hdr->pl_len = payload_len;

        memcpy(frame + sizeof(ws_ophdr_t), json_str, payload_len);

        // 通过 WebSocket 连接发送帧
        bufferevent_write(ws_ctx->bev, frame, frame_size);

        free(json_str);
        free(frame);
    }
}



