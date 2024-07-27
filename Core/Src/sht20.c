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


/*通过该宏控制是使用HAL库里的I2C接口还是CPIO模拟串口的接口*/
#define CONFIG_GPIO_I2C

#ifdef CONFIG_GPIO_I2C
#include "gpio_i2c_sht20.h"
#else
#include "i2c.h"
#endif

//#define CONFIG_SHT20_DEBUG //用于调试，打开时由printf，发布产品时注释掉，printf语句会被替换成do{}while(0)

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

#ifdef CONFIG_GPIO_I2C
	rv = I2C_Master_Transmit(0x80,&cmd,1); //GPIO模拟I2C采样
#else
	rv = HAL_I2C_Master_Transmit(&hi2c1,SHT20_ADDR_WR,&cmd,1,0xFFFF);//基于HAL库实现I2C采样
#endif

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

#ifdef CONFIG_GPIO_I2C
	rv = I2C_Master_Receive(0x81,buf,2);
#else
	rv = HAL_I2C_Master_Receive(&hi2c1,SHT20_ADDR_RD,buf,2,0xFFFF);
#endif

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








