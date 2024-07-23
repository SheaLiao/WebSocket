/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "esp8266.h"
#include "sht20.h"
#include "core_json.h"
#include "wss.h"
#include "index_html.h"

#include <string.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

#define	PORT			12345
#define JSON_BUF_SIZE 	128
int 					client_link_id = -1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static int report_tempRH_json(void);

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

	int			wifi_status = 0;
	int			server_status = -1;
	uint32_t 	last_send_time = 0;
	uint32_t 	send_interval = 5000; // 发�?�间隔，单位为毫�?


  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM6_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */



  while (1)
  {
	  printf("Current wifi_status: %d, server_status: %d\r\n", wifi_status, server_status);
	  if (!(wifi_status & FLAG_WIFI_CONNECTED))
	  {
		  wifi_status = esp8266_connect_wifi();
		  if (wifi_status & FLAG_WIFI_CONNECTED)
		  {
			  printf("ESP8266 connect WiFi successfully\r\n");

			  server_status = esp8266_setup_server(PORT);
			  if (server_status == 0)
			  {
				  printf("ESP8266 server setup on port %d successfully\r\n", PORT);
				  wifi_status |= FLAG_SOCK_CONNECTED;
			  }
			  else
			  {
				  printf("ESP8266 server setup on port %d failed\r\n", PORT);
			  }
		  }
		  else
		  {
			  printf("ESP8266 connect WiFi failure\r\n");
		  }
	  }


	  // �?查是否有新的客户端连�?
	  if (wifi_status & FLAG_SOCK_CONNECTED && check_client_connection(&client_link_id))
	  {
		  wifi_status |= FLAG_CLIENT_CONNECTED;
		  printf("connect successfully on link %d\r\n", client_link_id);
	  }
	  else
	  {
		  printf("No client connected\r\n");
	  }

	  // 在检测到客户端连接后定时发�?�数�?
	  if (wifi_status & FLAG_CLIENT_CONNECTED)
	  {
		  uint32_t current_time = HAL_GetTick();
		  if (current_time - last_send_time >= send_interval)
		  {
			  // 发�?�温湿度数据
			  if (report_tempRH_json() == 0)
			  {
				  printf("Temperature and humidity data sent successfully\r\n");
			  }
			  else
			  {
				  printf("Failed to send temperature and humidity data\r\n");
			  }
			  last_send_time = current_time; // 更新上次发�?�时�?
		  }
	  }

	  HAL_Delay(300);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }


  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */


int send_html_page(int client_link_id)
{
    int html_length = strlen(g_index_html);
    char send_cmd[128];

    // 生成 AT+CIPSEND 命令
    snprintf(send_cmd, sizeof(send_cmd), "AT+CIPSEND=%d,%d\r\n", client_link_id, html_length);

    // 发�?? AT+CIPSEND 命令
    if (send_atcmd(send_cmd, ">", 500) == 0)
    {
        // 发�?? HTML 数据
        if (send_atcmd(g_index_html, "SEND OK", 500) == 0)
        {
            return 0; // 发�?�成�?
        }
        else
        {
            dbg_print("ERROR: HTML data send failure\r\n");
            return -2; // 数据发�?�失�?
        }
    }
    else
    {
        dbg_print("ERROR: AT+CIPSEND command failure\r\n");
        return -1; // AT+CIPSEND 命令失败
    }
}



int report_tempRH_json(void)
{
    char buf[JSON_BUF_SIZE];
    float temperature, humidity;
    int rv;

    // 获取温度和湿度数�?
    SHT20_SampleData(0xF3, &temperature);
    SHT20_SampleData(0xF5, &humidity);

    // 生成 JSON 消息
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "{\"Temperature\":\"%.2f\",\"Humidity\":\"%.2f\"}", temperature, humidity);

    // 发�?? JSON 消息
    char send_cmd[128];
    snprintf(send_cmd, sizeof(send_cmd), "AT+CIPSEND=%d,%d\r\n", client_link_id, strlen(buf));

    // 发�?? AT+CIPSEND 命令
    if (send_atcmd(send_cmd, ">", 500) == 0)
    {
        // 发�?? JSON 数据
        if (send_atcmd(buf, "SEND OK", 500) == 0)
        {
            rv = 0; // 发�?�成�?
        }
        else
        {
            rv = -1; // 数据发�?�失�?
        }
    }
    else
    {
        rv = -1; // AT+CIPSEND 命令失败
    }

    return rv;
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
