/*********************************************************************************
 *      Copyright:  (C) 2024 Liao Shengli<li@gmail.com>
 *                  All rights reserved.
 *
 *       Filename:  led.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(10/07/24)
 *         Author:  Liao Shengli <li@gmail.com>
 *      ChangeLog:  1, Release initial version on "10/07/24 19:29:19"
 *                 
 ********************************************************************************/

#include <gpiod.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

#include "led.h"
#include "logger.h"

led_gpio_t  leds[LED_MAX] =
{
	{ LED_R, 19, "red",   NULL},
	{ LED_G, 13, "green", NULL},
	{ LED_B, 26, "blue",  NULL},
};


void init_gpio(struct gpiod_chip *chip)
{
	const char *chipname = "gpiochip0";
	int i;

	chip = gpiod_chip_open_by_name(chipname);
	if (!chip) 
	{
		printf("gpiod open '%s' failed: %s\n", chipname, strerror(errno));
		return;
	}

	for (i = 0; i < LED_MAX; i++) 
	{
		leds[i].line = gpiod_chip_get_line(chip, leds[i].gpio);
		gpiod_line_request_output(leds[i].line, "rgbled", 0);
	}
}


void close_gpio(struct gpiod_chip *chip)
{	
	int i;

	for(i=0; i<LED_MAX; i++)
	{
		gpiod_line_release(leds[i].line);
	}

	gpiod_chip_close(chip);
}


void turn_led(int which, int status) {
	if (which < 0 || which >= LED_MAX)
		return;

	log_info("turn %5s led GPIO#%d %s\n", leds[which].desc, leds[which].gpio, status ? "ON" : "OFF");

	gpiod_line_set_value(leds[which].line, status);

}


static int parse_led_id(const char *ledId)
{
	if (strcmp(ledId, "R") == 0) 
	{
		return LED_R;
	} 
	else if (strcmp(ledId, "G") == 0) 
	{
		return LED_G;
	} 
	else if (strcmp(ledId, "B") == 0) 
	{
		return LED_B;
	} 
	else 
	{
		return LED_MAX;  // 无效的 LED ID
	}
}


static int parse_led_status(const char *status) {
	if (strcmp(status, "on") == 0) 
	{
		return ON;
	} 
	else if (strcmp(status, "off") == 0) 
	{
		return OFF;
	} 
	else 
	{
		return -1;  // 无效的状态
	}
}



void parse_led_data(const char *payload, size_t length)
{
	cJSON *root = cJSON_ParseWithLength(payload, length);

	if (root == NULL) 
	{
		log_error("JSON parsing error\n");
		return;
	}

	cJSON *led_id_json = cJSON_GetObjectItem(root, "ledId");
	cJSON *status_json = cJSON_GetObjectItem(root, "status");

	if (!cJSON_IsString(led_id_json) || !cJSON_IsString(status_json)) {
		log_error("Invalid JSON structure\n");
		cJSON_Delete(root);
		return;
	}

	int led_id = parse_led_id(led_id_json->valuestring);
	int status = parse_led_status(status_json->valuestring);

	if (led_id >= 0 && led_id < LED_MAX && status >= 0)
	{
		turn_led(led_id, status);
	}
	else
	{
		if (led_id < 0 || led_id >= LED_MAX)
		{
			log_error("Invalid LED ID: %s\n", led_id_json->valuestring);
		}
		if (status < 0)
		{
			log_error("Invalid LED status: %s\n", status_json->valuestring);
		}
	}

	cJSON_Delete(root);
}
