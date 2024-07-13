/********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<li@gmail.com>
 *                  All rights reserved.
 *
 *       Filename:  sht20.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(11/07/24)
 *         Author:  Liao Shengli <li@gmail.com>
 *      ChangeLog:  1, Release initial version on "11/07/24 20:49:21"
 *                 
 ********************************************************************************/

#ifndef _SHT20_H_
#define _SHT20_H_

#define I2C_FILE                "/dev/i2c-1"
#define SHT2X_ADDR              0x40
#define SOFTRESET               0xFE
#define TEMP_CMD_NO_HOLD        0xF3
#define RH_CMD_NO_HOLD          0xF5

typedef struct sample_ctx_s
{
        float   temp;
        float   rh;
} sample_ctx_t;


static inline void msleep(unsigned long ms);
extern int sht20_init(void);
extern int sht20_softreset(int fd);
extern int sht20_send_cmd(int fd, char *cmd);
extern int get_temp_rh(int fd, sample_ctx_t *sample);
extern int sht20_sample_data(sample_ctx_t *sample);
extern void send_sample_data(struct bufferevent *bev);

#endif
