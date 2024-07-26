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
#include "ringbuf.h"

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
#define JSON_BUF_SIZE 	1028
int 					client_link_id = -1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static int report_tempRH_json(void);
static int parser_led_json(RingBuffer *rb);

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
	uint32_t 	send_interval = 5000;

	wss_session_t *session;

	if (!(session = malloc(sizeof(*session))))
	{
		printf("ERROR: Memory allocation for session failed\r\n");
		return -1;
	}
	memset(session, 0, sizeof(*session));

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


      if (wifi_status & FLAG_SOCK_CONNECTED && client_link_id < 0)
      {
          int connection_status = check_client_connection(&client_link_id);
          if (connection_status > 0)
          {
              wifi_status |= FLAG_CLIENT_CONNECTED;
              printf("Client connected successfully on link %d\r\n", client_link_id);
          }
          else if (connection_status < 0)
          {
              printf("Client connection on link [%d] closed\r\n", client_link_id);
              client_link_id = -1;
          }
      }

      printf("client_link_id: %d\r\n");

	  if (wifi_status & FLAG_CLIENT_CONNECTED)
	  {
		  if (!session->handshaked)
		{
			do_wss_handshake(&client_link_id, session);
		}
		else
		{
			uint32_t current_time = HAL_GetTick();
			if (current_time - last_send_time >= send_interval)
			{
				if (report_tempRH_json() == 0)
				{
					printf("Temperature and humidity data sent successfully\r\n");
				}
				else
				{
					printf("Failed to send temperature and humidity data\r\n");
				}
				last_send_time = current_time;
			}

			if (g_uart2_ringbuf.count > 0)
			{
				if (parser_led_json(&g_uart2_ringbuf) != 0)
				{
					// 根据需要清空缓冲区
					ring_buffer_init(&g_uart2_ringbuf);
				}
			}
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



int report_tempRH_json(void)
{
    char buf[JSON_BUF_SIZE];
    float temperature, humidity;
    int rv;

    SHT20_SampleData(0xF3, &temperature);
    SHT20_SampleData(0xF5, &humidity);

    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "{\"Temperature\":\"%.2f\",\"Humidity\":\"%.2f\"}", temperature, humidity);

    rv = esp8266_sock_send((unsigned char *)buf, client_link_id, strlen(buf));

    return rv;
}




int parser_led_json(RingBuffer *rb)
{
    JSONStatus_t result;
    char json_string[UART2_BUFFER_SIZE]; // 缓冲区大小要根据实际情况调整
    size_t bytes_read = 0;

    // 从环形缓冲区读取数据
    while (ring_buffer_read(rb, (uint8_t *)&json_string[bytes_read]) == 0 && bytes_read < UART2_BUFFER_SIZE)
    {
        bytes_read++;
    }

    if (bytes_read == 0)
    {
        printf("ERROR: No data read from ring buffer!\r\n");
        return -1;
    }

    json_string[bytes_read] = '\0'; // 确保字符串终止

    char *json_data = strstr(json_string, "{");
    if (json_data == NULL)
    {
        printf("ERROR: No JSON data found in string!\r\n");
        return -1;
    }

    size_t json_len = bytes_read - (json_data - json_string);

    printf("DBUG: Extracted JSON string: %s\r\n", json_data);

    result = JSON_Validate(json_data, json_len);

    if (JSONPartial == result)
    {
        printf("WARN: JSON document is valid so far but incomplete!\r\n");
        return 0;
    }

    if (JSONSuccess != result)
    {
        printf("ERROR: JSON document is not valid JSON!\r\n");
        return -1;
    }

    for (int i = 0; i < LedMax; i++)
    {
        char *value;
        size_t valen;

        result = JSON_Search(json_data, json_len, leds[i].name, strlen(leds[i].name), &value, &valen);
        if (JSONSuccess == result)
        {
            char save = value[valen];
            value[valen] = '\0';

            if (!strncasecmp(value, "on", 2))
            {
                printf("DBUG: turn %s on\r\n", leds[i].name);
                turn_led(i, ON);
            }
            else if (!strncasecmp(value, "off", 3))
            {
                printf("DBUG: turn %s off\r\n", leds[i].name);
                turn_led(i, OFF);
            }

            value[valen] = save;
        }
    }

    return 1;
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
