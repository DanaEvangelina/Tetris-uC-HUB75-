/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  *  Created on: 11/2025
  *      Author: Dana Gonzalez
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "stm32f446xx.h"
#include "systick.h"
#include "led.h"
#include "led_programs.h"
#include "inicializacion.h"
// #include "video.h"
#include "video_gojo.h"

// Agrego fotos estáticas
#include "image2.h"
#include "image1.h"
#include "colors.h"
#include "text.h"
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
ADC_HandleTypeDef hadc2;

TIM_HandleTypeDef htim6;

/* USER CODE BEGIN PV */

float valor_max_adc= 4100, valor_min_adc =0;
volatile uint32_t value_adc;

uint32_t x; // mantenido por compatibilidad (en píxeles cuando se necesite)
volatile uint8_t bit;
volatile uint8_t row;
volatile uint32_t frame_count;
uint32_t frames_per_program=4000;
uint8_t bandera_debugger = 1;
uint8_t bandera_adc = 0 ;

float x_min = 0;       // mínima posición horizontal (no usado en la nueva lógica)
float x_max = WIDTH-1; // máxima posición horizontal (no usado en la nueva lógica)
volatile uint8_t rotacion = 0;

#define BLOCK_SIZE  3
#define BOARD_WIDTH  (WIDTH / BLOCK_SIZE)
#define BOARD_HEIGHT (HEIGHT / BLOCK_SIZE)

// Parámetros del juego
float fall_speed = 0.5f;   // (ya no usado para caída en bloques, lo dejo por compat)
float y_offset = 0.0f;     // posición vertical del tetromino actual en píxeles (sincronizada con y_block)
char current_tetromino = 'S';
RGB_t current_color;


// Posiciones en bloques
int x_block = 0;     // posición X del tetromino en unidades de bloque
int y_block = 0;     // posición Y del tetromino en unidades de bloque

// Próxima pieza / auxiliares
char next_tetromino = 'I';
RGB_t next_color;
int next_x_block = 0;
int next_y_block = 0;
int new_y = 0;       // auxiliar para nuevo tetromino


// Variables estáticas para la caída (solo se inicializan una vez)
static uint32_t last_drop_time = 0;
static uint32_t drop_interval = 300; // ms entre caídas (ajustable)  ESTO SETEA LA DIFICULTAD!!!!!


// --- Variables globales de puntaje ---
uint32_t score = 0;     // Puntos acumulados
uint8_t points_per_piece = 15;  // Puntos por tetromino asentado

uint32_t gameOverStartTime = 0;
static uint32_t last_speedup_score = 0;

volatile GameState game_state = STATE_START;
volatile uint8_t game_started = 0;

volatile uint8_t busyFlag; // Se limpia con el DMA2_Stream2_IRQHandler
                           // Esto es cuando termina la transferencia de datos
uint8_t *nextBuffer;

uint32_t current_buffer, start_time;
programs_t programa_actual=video_1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC2_Init(void);
static void MX_TIM6_Init(void);
void add_points(uint32_t *score, uint16_t points) {
    *score += points;}
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */




int main(void)
{

  /* USER CODE BEGIN 1 */
 	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = BOTON_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BOTON_GPIO_Port, &GPIO_InitStruct);
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	init();
	MX_TIM6_Init();
	MX_ADC2_Init();
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/


  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

      // Logica DMA (Pantalla)
      start_time = HAL_GetTick();
      while(busyFlag); // Bloquea el CPU mientras el DMA labura
      busyFlag = 1;
      current_buffer = DMA2_Stream2->CR | DMA_SxCR_CT;
      nextBuffer = current_buffer ? buffer2 : buffer1;
      while(busyFlag);
      busyFlag = 1;

      // Colores de los tetrominos
      RGB_t colors[] = {cyan, blue, green, red, magenta, white};
      uint32_t num_colors = sizeof(colors) / sizeof(colors[0]);

      // Máquina de estados
      switch (game_state) {

          //  Estado 1: Pantalla de inicio
          case STATE_START: {

              Screen_Start(frame, font);
              LED_fillBuffer(frame, nextBuffer);
              score = 0;
              last_speedup_score = 0;

              if (game_started == 1) {

                  game_state = STATE_PLAYING;

                  // limpiar tablero lógico (en bloques)
                   board_clear();

                  // Inicializar primera pieza en bloques
                  rotacion = 0;
                  const char tetrominos[] = "IOTLS";
                  current_tetromino = tetrominos[rand() % 5];
                  current_color = colors[rand() % num_colors];

                  // colocar centrado en bloques
                  x_block = (BOARD_WIDTH / 2) - (get_tetromino_width(current_tetromino, rotacion) / 2);
                  y_block = 0;

                  // sincronizar variables en píxeles (compat)
                  x = x_block * BLOCK_SIZE;
                  y_offset = y_block * BLOCK_SIZE;

                  game_started = 0;
                  continue;
              }

              break;
          }

          //  Estado 2: Juego en curso
          case STATE_PLAYING: {
              // 1️⃣ Leer ADC
              HAL_ADC_Start(&hadc2);
              HAL_ADC_PollForConversion(&hadc2, 10);
              value_adc = HAL_ADC_GetValue(&hadc2);

              //  Mapear ADC a porcentaje 0.0 - 1.0
              float percentage = mapf((float)value_adc, valor_min_adc, valor_max_adc, 0.0f, 1.0f);

              // Movimiento horizontal (en bloques)
              int max_x_blocks = BOARD_WIDTH - get_tetromino_width(current_tetromino, rotacion);
              int desired_x_block = (int)(percentage * max_x_blocks + 0.5f);

              if (!check_collision_horizontal_block(current_tetromino, rotacion, desired_x_block, y_block)) {
                  x_block = desired_x_block;
              }

              // 2️⃣ Caída vertical controlada por tiempo ("gravedad")
              uint32_t now = HAL_GetTick();
              if (now - last_drop_time >= drop_interval) {
                  if (!check_collision_block(current_tetromino, rotacion, x_block, y_block + 1)) {
                      y_block += 1;
                  } else {
                      // Aterrizó: merge al tablero lógico
                      merge_tetromino_to_board(current_tetromino, current_color, x_block, y_block, rotacion);

                      // Borrar líneas completas y sumar puntos
                      board_clear_full_lines_and_score();


                      // Dificultad progresiva
                      if (score - last_speedup_score >= 100) {
                          last_speedup_score = score;
                          if (drop_interval > 100)
                              drop_interval -= 25;
                      }

                      // Preparar siguiente tetromino (next)
                      const char tetrominos[] = "IOTLS";
                      next_tetromino = tetrominos[rand() % 5];
                      next_color = colors[rand() % num_colors];
                      next_x_block = (int)(percentage * max_x_blocks + 0.5f);
                      next_y_block = 0;
                      new_y = 0;

                      // Game Over si la nueva pieza colisiona en su spawn
                      if (check_game_over_board_y0(current_tetromino, rotacion, x_block, y_block)) {
                          gameOverStartTime = HAL_GetTick();
                          game_state = STATE_GAME_OVER;
                          break;
                      }

                      // Hacer current <- next
                      current_tetromino = next_tetromino;
                      current_color = next_color;
                      x_block = next_x_block;
                      y_block = next_y_block;
                      rotacion = 0;

                      // Sincronizar variables en píxeles (compatibilidad)
                      x = x_block * BLOCK_SIZE;
                      y_offset = y_block * BLOCK_SIZE;
                  }

                  // Actualizar tiempo del último descenso
                  last_drop_time = now;
              }

              // Dibujar escena: limpiar frame, render tablero y pieza actual
              LED_rectangle(frame, black, 0, HEIGHT - 1, 0, WIDTH - 1);
              render_board_to_frame(frame);
              draw_tetromino_on_frame(frame, current_color, x_block, y_block, rotacion, current_tetromino);
              LED_fillBuffer(frame, nextBuffer);
              break;
          }

          //  Estado 3: Game Over
          case STATE_GAME_OVER: {

              LED_rectangle(frame, black, 0, HEIGHT - 1, 0, WIDTH - 1);

              LED_GameOverBlocks(frame);
              LED_Text(frame, font, "score:", 5,40 , white, 1);
              LED_DrawNumber(frame, score, 10, 50, 2);
              LED_fillBuffer(frame, nextBuffer);

              if (HAL_GetTick() - gameOverStartTime >= 10000) {
                  game_state = STATE_START;
              }
              break;
          }
      }

      //  Mantener framerate
      while (HAL_GetTick() - start_time < 30); // ~33Hz
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc2.Init.Resolution = ADC_RESOLUTION_12B;
  hadc2.Init.ScanConvMode = DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  hadc2.Init.DMAContinuousRequests = DISABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;  // Esto corresponde a PA4
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 8399;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 250;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : BOTON_Pin */
  GPIO_InitStruct.Pin = BOTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Start_Pin */
  GPIO_InitStruct.Pin = Start_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Start_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USART_TX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(USART_TX_GPIO_Port, &GPIO_InitStruct);



  // Habilitar NVIC para EXTI3
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */

/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

//Callback del antirrebote
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM6){
        HAL_TIM_Base_Stop_IT(&htim6);

        // Leer botón principal
        if (HAL_GPIO_ReadPin(BOTON_GPIO_Port, BOTON_Pin) == 0){

                if(rotacion < 3) rotacion++;
                else rotacion = 0;
            }

        if (HAL_GPIO_ReadPin(Start_GPIO_Port, Start_Pin) == 0){
                     game_started = 1;
                    }
        }
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin==BOTON_Pin){
		HAL_TIM_Base_Start_IT(&htim6); //antirrebote
	}
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
