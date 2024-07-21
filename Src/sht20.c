/*
 * sht20.c
 *
 *  Created on: Jul 21, 2023
 *      Author: asus
 */


#include <stdio.h>
#include "stm32l4xx_hal.h"
#include "sht20.h"
#include "tim.h"


#include "gpio_i2c_sht20.h"

//#define CONFIG_SHT20_DEBUG 

#ifdef CONFIG_SHT20_DEBUG
#define sht20_print(format,args...)printf(format,##args)
#else
#define sht20_print(format,args...)do{}while(0)
#endif

int SHT20_SampleData(uint8_t cmd,float *data)
{
	uint8_t buf[2];
	float sht20_data = 0.0;
	int rv;

	rv = I2C_Master_Transmit(0x80,&cmd,1); 
	
	if(0 != rv)
	{
		return -1;
	}
	if(cmd == 0xF3)
	{
		HAL_Delay(85);
	}
	else if(cmd == 0xF5)
	{
		HAL_Delay(29);
	}

	rv = I2C_Master_Receive(0x81,buf,2);

	if(0 != rv)
	{
		return -1;
	}
	sht20_data = buf[0];
	sht20_data=ldexp(sht20_data,8);
	sht20_data += buf[1]&0xFC;
	if(cmd == 0xF3)
	{
		*data = (-46.85+175.72*sht20_data/65536);
	}
	else if(cmd == 0xF5)
	{
		*data = (-6+125*sht20_data/65536);
	}
	return *data;
}








