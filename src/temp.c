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
#include <stdbool.h>
#include <cjson/cJSON.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>


#include "wss.h"
#include "frame.h"
#include "temp.h"
#include "logger.h"

static float get_temperature() {
	float temp = 0.0;

	temp = rand() % 5 + 25;

    return temp;  
}



void send_temperature(struct bufferevent *bev) 
{
    float 	temperature;
	char	*json_str;
    char 	frame_buf[FRAME_SIZE];
	int		frame_size = 0;

	if( !bev )
	{
		log_error("temp bev null\n");
	}
	log_info("temp bev not null\n");

	temperature = get_temperature();
	log_info("temperature:%f\n", temperature);

    // 构建 JSON 数据
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "temperature");
    cJSON_AddNumberToObject(root, "value", temperature);
    cJSON_AddNumberToObject(root, "timestamp", (double)time(NULL));

    json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if ( json_str ) 
	{
        frame_size = wss_create_text_frame(frame_buf, sizeof(frame_buf), json_str);
		log_info("temp frame size:%s\n", frame_buf);
        if (frame_size > 0) 
		{
            // 通过 WebSocket 连接发送帧
            bufferevent_write(bev, frame_buf, frame_size);
        }

        free(json_str);
    }
}
