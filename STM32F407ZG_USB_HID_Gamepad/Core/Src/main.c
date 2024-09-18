/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "usb_device.h"
#include "gpio.h"
#include "usbd_hid.h"

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

extern USBD_HandleTypeDef hUsbDeviceFS;

/* KEY端口定义 */
#define KEY_X           HAL_GPIO_ReadPin(KEY_X_GPIO_Port, KEY_X_Pin)     
#define KEY_Y           HAL_GPIO_ReadPin(KEY_Y_GPIO_Port, KEY_Y_Pin)     
#define KEY_A           HAL_GPIO_ReadPin(KEY_A_GPIO_Port, KEY_A_Pin)   
#define KEY_B           HAL_GPIO_ReadPin(KEY_B_GPIO_Port, KEY_B_Pin)
#define KEY_RIGHT       HAL_GPIO_ReadPin(KEY_RIGHT_GPIO_Port, KEY_RIGHT_Pin)

#define KEYX_PRES       1              /* KEY_X 按下 */
#define KEYY_PRES       2              /* KEY_Y 按下 */
#define KEYA_PRES       3              /* KEY_A 按下 */
#define KEYB_PRES       4              /* KEY_B 按下 */
#define KEYRIGHT_PRES   5              /* KEY_RIGHT 按下*/

/*
* sendbuf[0]~sendbuf[1]:按键
* sendbuf[2]:左侧摇杆x坐标
* sendbuf[3]:左侧摇杆y坐标
* sendbuf[4]:右侧摇杆x坐标
* sendbuf[5]:右侧摇杆y坐标
*/
unsigned char sendbuf[6]={0x00,0x00,0x80,0x80,0x80,0x80};

uint8_t key_scan(void)
{
    uint8_t keyval = 0;

    if (KEY_X == 0)  keyval = KEYX_PRES;
    if (KEY_Y == 1)  keyval = KEYY_PRES;
    if (KEY_A == 0)  keyval = KEYA_PRES;
    if (KEY_B == 0)  keyval = KEYB_PRES;
    if (KEY_RIGHT == 0) keyval = KEYRIGHT_PRES;

    return keyval;              /* 返回键值 */
}
void key_handle(void)
{
    uint8_t key;
    
    sendbuf[0] = 0x00;
    sendbuf[1] = 0x00;
    
    key = key_scan();
    switch (key)
    {
        case KEYY_PRES:          //up       
        {
            sendbuf[0] = 0x08;
            break;
        }
        case KEYA_PRES:         //down    
        {
            sendbuf[0] = 0x01;
            break;
        }
        case KEYX_PRES:         //left         
        {
            sendbuf[0] = 0x04;
            break;
        }
        case KEYB_PRES:         //right            
        {
            sendbuf[0] = 0x02;
            break;
        }
        case KEYRIGHT_PRES:     //右操作摇杆键
        {
            sendbuf[1] = 0x08;
            break;
        }
        default: break;
    }
}

void adc_channel_set(ADC_HandleTypeDef *adc_handle, uint32_t ch, uint32_t rank, uint32_t stime)
{
    /* 配置对应ADC通道 */
    ADC_ChannelConfTypeDef adc_channel;
    adc_channel.Channel = ch;               /* 设置ADCX对通道ch */
    adc_channel.Rank = rank;                /* 设置采样序列 */
    adc_channel.SamplingTime = stime;       /* 设置采样时间 */
    HAL_ADC_ConfigChannel( adc_handle, &adc_channel);   
}

uint32_t adc_get_result(uint32_t ch)
{
    adc_channel_set(&hadc1, ch, 1, ADC_SAMPLETIME_480CYCLES);   /* 设置通道，序列和采样时间 */
    HAL_ADC_Start(&hadc1);                                       /* 开启ADC */
    HAL_ADC_PollForConversion(&hadc1, 10);                       /* 轮询转换 */

    return (uint16_t)HAL_ADC_GetValue(&hadc1);                   /* 返回最近一次ADC1规则组的转换结果 */
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_USB_DEVICE_Init();
    while (1)
    {
        key_handle();
        sendbuf[4] = (uint8_t)adc_get_result(ADC_CHANNEL_10);
        sendbuf[5] = (uint8_t)adc_get_result(ADC_CHANNEL_11);
        USBD_HID_SendReport(&hUsbDeviceFS,(uint8_t*)&sendbuf,sizeof(sendbuf));
    }
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
