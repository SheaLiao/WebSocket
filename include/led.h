/********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<li@gmail.com>
 *                  All rights reserved.
 *
 *       Filename:  led.h
 *    Description:  This file 
 *
 *        Version:  1.0.0(10/07/24)
 *         Author:  Liao Shengli <li@gmail.com>
 *      ChangeLog:  1, Release initial version on "10/07/24 19:21:50"
 *                 
 ********************************************************************************/

#ifndef _LED_H_
#define _LED_H_


#include <gpiod.h>


enum
{
    LED_R,
    LED_G,
    LED_B,
    LED_MAX,
};

/* RGB Led status */
enum
{
    OFF=0,
    ON,
};

/* RGB led struct */
typedef struct led_gpio_s
{
    int                 idx;  /* led index       */
    int                 gpio; /* led GPIO port   */
    const char         *desc; /* led description */
    struct gpiod_line  *line; /* led gpiod line  */
} led_gpio_t;


extern void init_gpio(struct gpiod_chip *chip);
extern void close_gpio(struct gpiod_chip *chip);
extern void parse_led_data(const char *payload, size_t length);
extern void turn_led(int which, int status);

#endif
