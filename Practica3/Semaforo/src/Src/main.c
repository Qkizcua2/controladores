/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
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
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
typedef enum {
    ESTADO_VERDE,              // Estado inicial esperando al peatón
    ESTADO_ESPERA_4S,          // Peatón pulsó, mantenemos verde 4s
    ESTADO_AMARILLO,           // Coches amarillo 3s
    ESTADO_ROJO_PEATON_VERDE,  // Coches rojo, Peatones verde 10s
    ESTADO_PARPADEO_LENTO,     // Peatón verde parpadea 5s
    ESTADO_PARPADEO_RAPIDO     // Peatón verde parpadea rápido 3s
} EstadoSemaforo;

EstadoSemaforo estado = ESTADO_VERDE;

volatile uint8_t boton_flag = 0;
volatile uint8_t peticion_pendiente = 0;
volatile uint8_t modo_emergencia = 0;

uint32_t tiempo_verde = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void print_estado(char *txt);
void smart_delay(uint32_t delay_ms);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void print_estado(char *txt)
{
    char msg[100];
    sprintf(msg, "%s - Tiempo: %lu ms\r\n", txt, HAL_GetTick());
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

// Retardo inteligente que se rompe instantáneamente si hay una emergencia
void smart_delay(uint32_t delay_ms)
{
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start < delay_ms) && !modo_emergencia)
    {
        // Espera no bloqueante
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  tiempo_verde = 0; // Forzamos la inicialización en la primera vuelta
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      if (modo_emergencia)
      {
          // Modo Emergencia: Apagar todos los LEDs excepto amarillo
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);

          // Parpadeo del amarillo coches
          HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_6);
          HAL_Delay(500); // Aquí usamos el delay normal porque estamos bloqueados en emergencia
          continue;       // Vuelve al inicio del while(1)
      }

      switch(estado)
      {
          case ESTADO_VERDE:
              if (tiempo_verde == 0)
              {
                  tiempo_verde = HAL_GetTick();
                  print_estado("ESTADO_VERDE");

                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);   // Verde coches ON
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); // Amarillo coches OFF
                  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET); // Rojo coches OFF
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);   // Rojo peatón ON
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET); // Verde peatón OFF
              }

              // Condición 1: Comprobar si han pasado los 5 segundos mínimos
              if (HAL_GetTick() - tiempo_verde >= 5000)
              {
                  // Si hay una petición guardada, o han pulsado justo ahora
                  if (boton_flag || peticion_pendiente)
                  {
                      boton_flag = 0;
                      peticion_pendiente = 0;
                      estado = ESTADO_ESPERA_4S;
                  }
              }
              else
              {
                  // Si pulsan ANTES de los 5 segundos, guardamos la petición
                  if (boton_flag)
                  {
                      peticion_pendiente = 1;
                      boton_flag = 0;
                  }
              }
              break;

          case ESTADO_ESPERA_4S:
              print_estado("ESTADO_ESPERA_4S");
              smart_delay(4000); 
              if (!modo_emergencia) estado = ESTADO_AMARILLO;
              break;

          case ESTADO_AMARILLO:
              print_estado("ESTADO_AMARILLO");
              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET); 
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   
              smart_delay(3000); 
              if (!modo_emergencia) estado = ESTADO_ROJO_PEATON_VERDE;
              break;

          case ESTADO_ROJO_PEATON_VERDE:
              print_estado("ESTADO_ROJO_PEATON_VERDE");
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); 
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);   
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET); 
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);   
              smart_delay(10000); 
              if (!modo_emergencia) estado = ESTADO_PARPADEO_LENTO;
              break;

          case ESTADO_PARPADEO_LENTO:
              print_estado("ESTADO_PARPADEO_LENTO");
              for(int i = 0; i < 10; i++)
              {
                  if (modo_emergencia) break;
                  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4); 
                  smart_delay(500); 
              }
              if (!modo_emergencia) estado = ESTADO_PARPADEO_RAPIDO;
              break;

          case ESTADO_PARPADEO_RAPIDO:
              print_estado("ESTADO_PARPADEO_RAPIDO");
              for(int i = 0; i < 12; i++)
              {
                  if (modo_emergencia) break;
                  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
                  smart_delay(250);
              }
              
              if (!modo_emergencia)
              {
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
                  tiempo_verde = 0; 
                  boton_flag = 0;
                  peticion_pendiente = 0;
                  estado = ESTADO_VERDE;
              }
              break;
      }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

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
  * @brief USART2 Initialization Function
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|GPIO_PIN_7, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin PA7 */
  GPIO_InitStruct.Pin = LD2_Pin|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 (Pulsador) */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // Detecta cuando pulsas hacia GND
  GPIO_InitStruct.Pull = GPIO_PULLUP;          // Elimina el ruido estático (Fantasma)
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 (Switch de Emergencia) */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING; // Detecta cambios en ambas direcciones
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;               // Por si se desconecta el cable, que lea 0
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PB4 PB5 PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  // INTERRUPCIÓN DEL BOTÓN (PB10)
  if (GPIO_Pin == GPIO_PIN_10)
  {
    // Solo se aceptan pulsaciones si NO estamos en emergencia y si estamos en el ESTADO_VERDE
    if (!modo_emergencia && estado == ESTADO_VERDE)
    {
      boton_flag = 1;
    }
  }

  // INTERRUPCIÓN DEL INTERRUPTOR DE EMERGENCIA (PA8)
  if (GPIO_Pin == GPIO_PIN_8)
  {
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_SET)
    {
      modo_emergencia = 1; // Se activó el switch
    }
    else
    {
      // Se desactivó el switch: Reiniciamos la máquina de estados
      modo_emergencia = 0;
      estado = ESTADO_VERDE;
      tiempo_verde = 0;
      peticion_pendiente = 0;
      boton_flag = 0;
    }
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */