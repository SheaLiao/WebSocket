/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */

//变量的声明，extern
extern char 	g_uart1_rxbuf[256];
extern uint8_t 	g_uart1_bytes;

//用来清除接收buffer里的数据
#define clear_uart1_rxbuf() do {memset(g_uart1_rxbuf,0,sizeof(g_uart1_rxbuf));\
								g_uart1_bytes=0;}while(0)



extern char 	g_uart2_rxbuf[256];
extern uint8_t 	g_uart2_bytes;

#define clear_uart2_rxbuf() do {memset(g_uart2_rxbuf,0,sizeof(g_uart2_rxbuf));\
								g_uart2_bytes=0;}while(0)


void uart_forward(void);

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

