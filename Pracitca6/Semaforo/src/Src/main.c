/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Semáforo FINAL - Completo (Labs 3, 4, 5 y 6)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "lcd_drv.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    ESTADO_VERDE,
    ESTADO_ESPERA_4S,
    ESTADO_AMARILLO,
    ESTADO_ROJO_PEATON_VERDE,
    ESTADO_PARPADEO_LENTO,
    ESTADO_PARPADEO_RAPIDO
} EstadoSemaforo;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
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
static void MX_TIM1_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
void print_estado(char *txt);
void smart_delay(uint32_t delay_ms);
void Control_Buzzer(void);
void Control_LCD_Visual(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void print_estado(char *txt) {
    char msg[100];
    sprintf(msg, "%s - Tiempo: %lu ms\r\n", txt, HAL_GetTick());
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

// Pantalla
void Update_LCD_Content(char* linea1, uint8_t r, uint8_t g, uint8_t b) {
    clearLCD();           // Limpia la pantalla
    setRGB(r, g, b);      // Cambia el color de fondo
    locateLCD(0, 0);      // Pone el cursor en la columna 0, fila 0
    printLCD(linea1);     // Imprime el texto
}

void Control_LCD_Visual(void) {
    static EstadoSemaforo estado_anterior = -1;
    static uint8_t peticion_anterior = 255;

    // Actualiza si cambia el estado o si entra una petición estando en verde
    if (estado != estado_anterior || (estado == ESTADO_VERDE && peticion_pendiente != peticion_anterior)) {
        estado_anterior = estado;
        peticion_anterior = peticion_pendiente;

        switch(estado) {
            case ESTADO_VERDE:
                if (peticion_pendiente || boton_flag) {
                    Update_LCD_Content("Espere Verde", 0, 0, 255); // Azul
                } else {
                    Update_LCD_Content("Pulse Boton", 255, 0, 0);  // Rojo
                }
                break;
            case ESTADO_ESPERA_4S:
            case ESTADO_AMARILLO:
                Update_LCD_Content("Espere Verde", 0, 0, 255);     // Azul
                break;
            case ESTADO_ROJO_PEATON_VERDE:
            case ESTADO_PARPADEO_LENTO:
            case ESTADO_PARPADEO_RAPIDO:
                Update_LCD_Content("Pase", 0, 255, 0);             // Verde
                break;
        }
    }
    
    // LCD en Modo Emergencia
    if (modo_emergencia) {
         static uint8_t emerg_init = 0;
         if (!emerg_init) {
             Update_LCD_Content("EMERGENCIA", 255, 255, 0);        // Amarillo
             emerg_init = 1;
         }
    } else {
         // Se mantiene vacío intencionalmente para evitar warnings
    }
}

// Zumbador
void Buzzer_Play(uint32_t freq) {
    if (freq == 0) return;
    uint32_t period = (1000000 / freq) - 1;
    __HAL_TIM_SET_AUTORELOAD(&htim1, period);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, period / 2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
}

void Buzzer_Stop(void) {
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
}

void Control_Buzzer(void) {
    static uint32_t ultimo_cambio = 0;
    static uint8_t pitido_activo = 0;
    static EstadoSemaforo estado_anterior = ESTADO_VERDE;

    if (modo_emergencia) { 
        Buzzer_Stop(); 
        pitido_activo = 0;
        return; 
    }

    if (estado != estado_anterior) {
        estado_anterior = estado;
        Buzzer_Stop();
        pitido_activo = 0;
        ultimo_cambio = HAL_GetTick();
    }

    if (estado == ESTADO_ROJO_PEATON_VERDE) {
        if (HAL_GetTick() - ultimo_cambio >= 500) {
            ultimo_cambio = HAL_GetTick();
            pitido_activo = !pitido_activo;
            if (pitido_activo) Buzzer_Play(500); else Buzzer_Stop();
        }
    }
    else if (estado == ESTADO_PARPADEO_LENTO) {
        if (HAL_GetTick() - ultimo_cambio >= 200) {
            ultimo_cambio = HAL_GetTick();
            pitido_activo = !pitido_activo;
            if (pitido_activo) Buzzer_Play(2000); else Buzzer_Stop();
        }
    }
    else if (estado == ESTADO_PARPADEO_RAPIDO) {
        if (!pitido_activo) {
            Buzzer_Play(3000);
            pitido_activo = 1;
        }
    }
    else {
        if (pitido_activo) { Buzzer_Stop(); pitido_activo = 0; }
    }
}


void smart_delay(uint32_t delay_ms) {
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start < delay_ms) && !modo_emergencia) {
        Control_Buzzer(); 
        Control_LCD_Visual(); 
    }
}

// Ultrasonido
void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);  
    while (__HAL_TIM_GET_COUNTER(&htim2) < us); 
}

void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

uint32_t Leer_Distancia_Ultrasonido(void) {
    uint32_t tiempo_eco = 0;
    uint32_t timeout = 0;

    Set_Pin_Output(GPIOA, GPIO_PIN_9);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    delay_us(2); 
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
    delay_us(10); 
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

    Set_Pin_Input(GPIOA, GPIO_PIN_9);

    timeout = HAL_GetTick();
    while (!(GPIOA->IDR & GPIO_PIN_9)) { if (HAL_GetTick() - timeout > 10) return 0; }

    __HAL_TIM_SET_COUNTER(&htim2, 0);

    timeout = HAL_GetTick();
    while ((GPIOA->IDR & GPIO_PIN_9)) { if (HAL_GetTick() - timeout > 50) break; }

    tiempo_eco = __HAL_TIM_GET_COUNTER(&htim2);
    return tiempo_eco / 58; 
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_TIM2_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  
  // 1. Parche prescaler TIM1 a 83 para el buzzer
  __HAL_TIM_SET_PRESCALER(&htim1, 83); 
  
  // 2. Parche pin PA8 (Emergencia) para que funcione en RISING_FALLING y con PULLDOWN
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  //Inicializar Pantalla LCD
  initLCD();
  Control_LCD_Visual(); // Fuerza el primer pintado

  tiempo_verde = 0; 
  HAL_TIM_Base_Start(&htim2); // Arranca TIM2 para ultrasonidos
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
          Control_LCD_Visual(); // Pinta "EMERGENCIA"
          HAL_Delay(500); 
          continue;       
      }

      Control_Buzzer(); 
      Control_LCD_Visual(); 

      switch(estado)
      {
          case ESTADO_VERDE:
              if (tiempo_verde == 0)
              {
                  tiempo_verde = HAL_GetTick();
                  print_estado("ESTADO_VERDE");

                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);   
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); 
                  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET); 
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);   
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET); 
              }

              static uint32_t ultimo_escaneo = 0;
              if (HAL_GetTick() - ultimo_escaneo >= 100) 
              {
                  ultimo_escaneo = HAL_GetTick();
                  uint32_t distancia = Leer_Distancia_Ultrasonido();
                  if (distancia > 0 && distancia <= 20) { boton_flag = 1; }
              }

              if (HAL_GetTick() - tiempo_verde >= 5000)
              {
                  if (boton_flag || peticion_pendiente)
                  {
                      boton_flag = 0; peticion_pendiente = 0;
                      estado = ESTADO_ESPERA_4S;
                  }
              }
              else if (boton_flag) { 
                  peticion_pendiente = 1; 
                  boton_flag = 0; 
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
                  tiempo_verde = 0; boton_flag = 0; peticion_pendiente = 0;
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
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
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
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
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|GPIO_PIN_7|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin PA7 PA9 */
  GPIO_InitStruct.Pin = LD2_Pin|GPIO_PIN_7|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : Echo_Pin */
  GPIO_InitStruct.Pin = Echo_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Echo_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  // --- 1. INTERRUPCIÓN DEL BOTÓN (PB10) ---
  if (GPIO_Pin == GPIO_PIN_10) {
    if (!modo_emergencia && estado == ESTADO_VERDE) {
        boton_flag = 1;
    }
  }
  
  // --- 2. INTERRUPCIÓN DE EMERGENCIA (PA8) ---
  if (GPIO_Pin == GPIO_PIN_8) {
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_SET) {
        modo_emergencia = 1; 
    } else {
        // EL ESCUDO: Solo reiniciamos el semáforo si de verdad venimos de una emergencia real.
        // Esto ignora los ruidos eléctricos fantasmas provocados al pulsar el botón.
        if (modo_emergencia == 1) {
            modo_emergencia = 0; 
            estado = ESTADO_VERDE;
            tiempo_verde = 0; 
            peticion_pendiente = 0; 
            boton_flag = 0;
            
            // Avisamos a la LCD de que hemos vuelto a la normalidad
            extern void Control_LCD_Visual(void);
        }
    }
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
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  * where the assert_param error has occurred.
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