/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (Semáforo + Interrupciones + Ultrasonidos)
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
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
typedef enum {
    ESTADO_VERDE,              // Estado inicial esperando al peatón
    ESTADO_ESPERA_4S,          // Peatón pulsó/detectado, mantenemos verde 4s
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
static void MX_TIM2_Init(void);
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

// Retardo inteligente: se rompe si hay emergencia
void smart_delay(uint32_t delay_ms)
{
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start < delay_ms) && !modo_emergencia)
    {
        // Espera no bloqueante
    }
}

// =================================================================
// FUNCIONES DEL SENSOR ULTRASÓNICO (PARTES 1 Y 2)
// =================================================================

// Retardo exacto en microsegundos usando TIM2
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);  
    while (__HAL_TIM_GET_COUNTER(&htim2) < us); 
}

// Cambia el pin PA9 a Modo Salida (Trigger)
void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

// Cambia el pin PA9 a Modo Entrada (Echo)
void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

// Dispara, mide y calcula la distancia
uint32_t Leer_Distancia_Ultrasonido(void)
{
    uint32_t tiempo_eco = 0;
    uint32_t timeout = 0;

    Set_Pin_Output(GPIOA, GPIO_PIN_9);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    delay_us(2); 
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
    delay_us(10); // Pulso de 10us
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

    Set_Pin_Input(GPIOA, GPIO_PIN_9);

    timeout = HAL_GetTick();
    while (!(GPIOA->IDR & GPIO_PIN_9)) 
    {
        if (HAL_GetTick() - timeout > 10) return 0; 
    }

    __HAL_TIM_SET_COUNTER(&htim2, 0);

    timeout = HAL_GetTick();
    while ((GPIOA->IDR & GPIO_PIN_9)) 
    {
        if (HAL_GetTick() - timeout > 50) break; 
    }

    tiempo_eco = __HAL_TIM_GET_COUNTER(&htim2);
    return tiempo_eco / 58; // Convertimos a cm
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
  MX_TIM2_Init();

  /* USER CODE BEGIN 2 */
  tiempo_verde = 0; 
  HAL_TIM_Base_Start(&htim2); // IMPRESCINDIBLE: Encendemos el cronómetro
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      if (modo_emergencia)
      {
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);

          HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_6);
          HAL_Delay(500); 
          continue;       
      }

      switch(estado)
      {
          case ESTADO_VERDE:
              if (tiempo_verde == 0)
              {
                  tiempo_verde = HAL_GetTick();
                  print_estado("ESTADO_VERDE");

                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);   // Coches Verde
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); 
                  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET); 
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);   // Peatón Rojo
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET); 
              }

              // =================================================================
              // INTEGRACIÓN DEL SENSOR (PARTE 3)
              // =================================================================
              static uint32_t ultimo_escaneo = 0;
              // Escaneamos cada 100ms para no saturar el chip
              if (HAL_GetTick() - ultimo_escaneo >= 100) 
              {
                  ultimo_escaneo = HAL_GetTick();
                  uint32_t distancia = Leer_Distancia_Ultrasonido();
                  
                  // Si detectamos algo a 20 cm o menos, simulamos que han pulsado el botón
                  if (distancia > 0 && distancia <= 20) 
                  {
                      boton_flag = 1; 
                  }
              }

              // Lógica de los 5 segundos mínimos
              if (HAL_GetTick() - tiempo_verde >= 5000)
              {
                  if (boton_flag || peticion_pendiente)
                  {
                      boton_flag = 0;
                      peticion_pendiente = 0;
                      estado = ESTADO_ESPERA_4S;
                  }
              }
              else
              {
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
  * @brief TIM2 Initialization Function
  */
static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83; // Divide 84MHz para que cada tick sea 1us
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
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

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, LD2_Pin|GPIO_PIN_7|GPIO_PIN_9, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  // Configuramos el pin 9 inicialmente como salida
  GPIO_InitStruct.Pin = LD2_Pin|GPIO_PIN_7|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Botón físico en PB10 con resistencia PULLUP
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Leds
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  // Interruptor de Emergencia en PA8 con resistencia PULLDOWN
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Leds
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Activar interrupciones
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  // BOTÓN PB10
  if (GPIO_Pin == GPIO_PIN_10)
  {
    if (!modo_emergencia && estado == ESTADO_VERDE)
    {
      boton_flag = 1;
    }
  }

  // SWITCH DE EMERGENCIA PA8
  if (GPIO_Pin == GPIO_PIN_8)
  {
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_SET)
    {
      modo_emergencia = 1; 
    }
    else
    {
      modo_emergencia = 0;
      estado = ESTADO_VERDE;
      tiempo_verde = 0;
      peticion_pendiente = 0;
      boton_flag = 0;
    }
  }
}
/* USER CODE END 4 */

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