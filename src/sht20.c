/*********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<li@gmail.com>
 *                  All rights reserved.
 *
 *       Filename:  sht20.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(11/07/24)
 *         Author:  Liao Shengli <li@gmail.com>
 *      ChangeLog:  1, Release initial version on "11/07/24 20:50:11"
 *                 
 ********************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <cjson/cJSON.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>

#include "wss.h"
#include "frame.h"
#include "sht20.h"
#include "logger.h"


static inline void msleep(unsigned long ms)
{
        struct timespec sleep;
        unsigned long ulTmp;

        sleep.tv_sec = ms / 1000;
        if (sleep.tv_sec == 0)
        {
                ulTmp = ms * 10000;
                sleep.tv_nsec = ulTmp * 100;
        }
        else
        {
                sleep.tv_nsec = 0;
        }
        nanosleep(&sleep, 0);
}


int sht20_softreset(int fd)
{
        int             rv = -1;
        uint8_t buf[2] = {0};

        if( fd < 0 )
        {
                log_error("input the invalid argument\n");
                return -1;
        }

        buf[0] = SOFTRESET;
        rv = write(fd, buf, 1);
        if( rv < 0 )
        {
                log_error("write softreset cmd failure\n");
                return -2;
        }

        msleep(50);

        return 0;
}



int sht20_init(void)
{
        int             fd = -1;
        int             rv = -1;

        fd = open(I2C_FILE, O_RDWR);
        if( fd < 0 )
        {
                log_error("i2c open failure:%s\n", strerror(errno));
                return -1;
        }

        ioctl(fd, I2C_TENBIT, 0);
        ioctl(fd, I2C_SLAVE, SHT2X_ADDR);

        rv = sht20_softreset(fd);
        if( rv < 0 )
        {
                log_error("sht20_softreset() failure\n");
                close(fd);
                return -2;
        }

        return fd;
}




int sht20_send_cmd(int fd, char *cmd)
{
        int             rv = -1;
        uint8_t buf[2] = {0};

        if( fd < 0 )
        {
                log_error("input the invalid argument\n");
                return -1;
        }

        if( strcmp(cmd, "temp")==0 )
        {
                buf[0] = TEMP_CMD_NO_HOLD;
                rv = write(fd, buf, 1);
                if( rv < 0 )
                {
                        log_error("send temp cmd failure\n");
                        return -2;
                }
                msleep(85);
        }
        else if( strcmp(cmd, "rh") == 0 )
        {
                buf[0] = RH_CMD_NO_HOLD;
                rv = write(fd, buf, 1);
                if( rv < 0 )
                {
                        log_error("send humidity cmd failure\n");
                        return -3;
                }
                msleep(29);
        }

        return 0;
}


int get_temp_rh(int fd, sample_ctx_t *sample)
{
        int                     rv = -1;
        uint8_t         buf[4] = {0};

        if( fd < 0 || !sample)
        {
                log_error("input the invalid argument\n");
                return -1;
        }

        rv = sht20_send_cmd(fd, "temp");
        if( rv < 0 )
        {
                log_error("send temp cmd failure\n");
                return -2;
        }
        else
        {
                rv = read(fd, buf, 3);
                if( rv < 0 )
                {
                        log_error("get temperature failure\n");
                        return -3;
                }
                sample->temp = 175.72 * (((((int) buf[0]) << 8) + (buf[1] & 0xFC)) / 65536.0) - 46.85;
        }

        rv = sht20_send_cmd(fd, "rh");
        if( rv < 0 )
        {
                log_error("send rh cmd failure\n");
                return -2;
        }
        else
        {
                rv = read(fd, buf, 3);
                if( rv < 0 )
                {
                        log_error("get humidity failure\n");
                        return -3;
                }
                sample->rh = 125 * (((((int) buf[0]) << 8) + (buf[1] & 0xFC)) / 65536.0) - 6;
        }

        return 0;
}

int sht20_sample_data(sample_ctx_t *sample)
{
        int             fd = -1;
        int             rv = -1;

        if( !sample )
        {
                log_error("invalid input argument\n");
                return -1;
        }

        fd = sht20_init();
        if( fd < 0 )
        {
                log_error("sht20_init() failure\n");
                close(fd);
                return -2;
        }

        rv = get_temp_rh(fd, sample);
        if( rv < 0 )
        {
                log_error("get_temp_rh() failure\n");
                close(fd);
                return -4;
        }

        log_info("temp: %f, humidity: %f\n", sample->temp, sample->rh);

        close(fd);
        return 0;
}


void send_sample_data(struct bufferevent *bev)
{
    sample_ctx_t sample;
    char *json_str;
    char frame_buf[FRAME_SIZE];
    int frame_size;

    if (!bev)
    {
        log_error("Sample bufferevent is NULL\n");
        return;
    }

    sht20_sample_data(&sample);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "timestamp", (double)time(NULL));

    cJSON *temp_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(temp_obj, "type", "temperature");
    cJSON_AddNumberToObject(temp_obj, "value", sample.temp);
    
    cJSON *humid_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(humid_obj, "type", "humidity");
    cJSON_AddNumberToObject(humid_obj, "value", sample.rh);

    cJSON_AddItemToObject(root, "temperature", temp_obj);
    cJSON_AddItemToObject(root, "humidity", humid_obj);

    json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

	log_info("str_length:%d, json_str: %s\n", strlen(json_str), json_str);

    if (json_str)
    {
        frame_size = wss_create_text_frame(frame_buf, FRAME_SIZE, json_str);
        log_info("Frame size: %d, frame:%s\n", frame_size, frame_buf);
        if (frame_size > 0)
        {
            bufferevent_write(bev, frame_buf, frame_size);
        }

        free(json_str);
    }

    return;
}






