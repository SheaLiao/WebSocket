/*
 * sht20.h
 *
 *  Created on: Jul 21, 2023
 *      Author: asus
 */

#ifndef INC_SHT20_H_
#define INC_SHT20_H_

#include "stm32l4xx_hal.h"

#define SHT20_ADDR 0x40

#define SHT20_ADDR_WR (SHT20_ADDR<<1)
#define SHT20_ADDR_RD ((SHT20_ADDR<<1) | 0x01)

#define SOFT_RESET_CMD 0xFE

#define TEMP_CMD 0xE3
#define RH_CMD 0xE5



extern int SHT20_SampleData(uint8_t cmd,float *data);

#endif /* INC_SHT20_H_ */
