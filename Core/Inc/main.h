/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *  Created on: 11/2025
  *      Editor: Dana Gonzalez
  *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#include <stdint.h>

#define row_mask 0x1F

#define PRESCALE 0

#define BITS_PER_PIXEL 24
#define BITS_PER_CHANNEL 8

#define WIDTH 64
#define HEIGHT 64
#define SCAN_RATE 32 // this is a 1/32 display

#define BRIGHTNESS 10 // this can be from 1 to 10

#define PIXEL(f, x, y) f[y * 64 + x]

#define TEXT_WIDTH 5
#define TEXT_HEIGHT 7

typedef enum {
   STATE_START,
   STATE_PLAYING,
   STATE_GAME_OVER
} GameState;

void _error_handler(void);

extern volatile uint8_t bit;
extern volatile uint8_t row;
extern volatile uint8_t busyFlag;
extern volatile uint32_t frame_count;
extern volatile uint32_t value_adc;
extern volatile uint8_t game_started;
extern volatile GameState game_state;
extern uint32_t score;


/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BOTON_Pin GPIO_PIN_13
#define BOTON_GPIO_Port GPIOA
#define BOTON_EXTI_IRQn EXTI15_10_IRQn

#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA

#define Start_Pin GPIO_PIN_13
#define Start_GPIO_Port GPIOC
#define Start_EXTI_IRQn EXTI15_10_IRQn


/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
