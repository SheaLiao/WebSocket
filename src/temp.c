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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <cjson/cJSON.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>


#include "wss.h"
#include "frame.h"
#include "temp.h"
#include "logger.h"


static int get_temperature(float *temp)
{
	int				fd = -1;
	int				found = 0;
	char			buf[128];
	char			*ptr = NULL;
	char			w1_path[64] = "/sys/bus/w1/devices/";
	char			chip[32];
	DIR				*dirp = NULL;
	struct dirent	*direntp = NULL;

	if( NULL == ( dirp = opendir(w1_path) ) )
	{
		log_error("Open dir error: %s\n", strerror(errno));
		return -1;
	}

	while( NULL != ( direntp = readdir(dirp) ) )
	{
		if( strstr(direntp->d_name, "28-") )
		{
			strncpy(chip, direntp->d_name, sizeof(chip));
			found = 1;
		}
	}
	closedir(dirp);

	if( !found )
	{
		log_error("Can't find path\n");
		return -2;
	}

	strncat(w1_path, chip, sizeof(w1_path) - strlen(w1_path));
	strncat(w1_path, "/w1_slave", sizeof(w1_path) - strlen(w1_path));

	fd = open(w1_path, O_RDONLY);
	if( fd < 0 )
	{
		log_error("Open file failure: %s\n", strerror(errno));
		return -3;
	}

	memset(buf, 0, sizeof(buf));

	if( read(fd, buf, sizeof(buf)) < 0 )
	{
		log_error("Read data faliure.\n");
		return -4;
		close(fd);
	}

	ptr = strstr(buf, "t=");
	if( !ptr )
	{
		log_error("Can't find t=\n");
		return -5;
		close(fd);
	}

	ptr += 2;
	*temp = atof(ptr)/1000;

	return 0;
}

void send_temperature(struct bufferevent *bev) 
{
    float 	temperature;
	int		rv = 0;
	char	*json_str;
    char 	frame_buf[FRAME_SIZE];
	int		frame_size = 0;

	if( !bev )
	{
		log_error("temp bev null\n");
	}
	log_info("temp bev not null\n");

	rv = get_temperature(&temperature);
	if( rv < 0)
	{
		log_error("temperature get failure\n");
	}
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
