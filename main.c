#include "main.h"
//#include "stdbool.h"

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
ADC_HandleTypeDef hadc1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_ADC1_Init(void);

int Lectura_IR(void);
void Seguidor_Linea(void);
void Sensor_Distancia(void);

#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
#define LED_Azul GPIOD, GPIO_PIN_15

//Sensores Infrarrojos (GPIO_Input): PB2, PE7, PE8, PE9, PE10
#define IR_Izquierdo GPIOB, GPIO_PIN_2
#define IR_Central_Izquierdo GPIOE, GPIO_PIN_7
#define IR_Central GPIOE, GPIO_PIN_8
#define IR_Central_Derecho GPIOE, GPIO_PIN_9
#define IR_Derecho GPIOE, GPIO_PIN_10

//Control de la direccion de giro de los motores (GPIO_Output): PE11, PE12, PE13, PE14
#define IN1 GPIOE, GPIO_PIN_11
#define IN2 GPIOE, GPIO_PIN_12
#define IN3 GPIOE, GPIO_PIN_13
#define IN4 GPIOE, GPIO_PIN_14

//PWM de los motores, uso el TIM4 (Timer 4) en modo PWM Generation CH2 (PB7) y PWM Generation CH3 (PB8)
//APB1 Timer Clock esá a 16MHz. Prescaler:16, Counter Mode: Up, Counter Period:99
// 16MHz / 16 = 1MHz ; 1 / 1MHz = 1useg ; 1useg * 99 = 0,1mseg ; 1 / 0,1mseg = 10kHz
//Pulse: 0 (0%) OV - 99 (100%) 3V. Ciclo de actividad en porcentaje
#define Timer_Motor &htim4
#define Motor_Izquierdo TIM_CHANNEL_2
#define Motor_Derecho TIM_CHANNEL_3

//El Trigger es PC12
#define TRIG GPIOC, GPIO_PIN_12
//TIM1 CH1 como Input Capture Direct Mode: PA8
#define Timer_ECHO &htim1
#define ECHO TIM_CHANNEL_1

//Tacometros de los motores, usan el ADC1 Canal 1 (PA1) y Canal 2 (PA2)
#define Tacometro_Izquierdo GPIOA, GPIO_PIN_1
#define Tacometro_Derecho GPIOA, GPIO_PIN_2

uint8_t Contador_Lectura_IR=0;
uint8_t Contador_Sensor_Distancia=0;

//Si el sensor IR detecta la línea negra vale 0, si no la detecta vale 1
uint8_t Vector_IR[5]={1,1,1,1,1};
int IR_Decimal=0;
uint8_t Distancia_cm=0;

//El IC (Input Capture) captura valores de 16bits
uint16_t IC_Flanco_Subida=0;
uint16_t IC_Flanco_Bajada=0;
uint16_t IC_Diferencia=0;
int Primer_Captura=0;

uint8_t Sensor_Velocidad[2]={0,0};

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_ADC1_Init();

  HAL_TIM_Base_Start_IT(&htim3);

  HAL_TIM_PWM_Start(Timer_Motor, Motor_Izquierdo);
  HAL_TIM_PWM_Start(Timer_Motor, Motor_Derecho);

  HAL_TIM_IC_Start_IT(Timer_ECHO, ECHO);

  HAL_ADC_Start_IT(&hadc1);

  HAL_GPIO_WritePin(LED_Azul, 0);

  while (1)
  {
	  IR_Decimal=Lectura_IR();
	  Sensor_Distancia();
	  Seguidor_Linea();
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) //El Timer 3 está configurado para que interrumpa cada 1ms
{
	Contador_Lectura_IR++;
	Contador_Sensor_Distancia++;
}

int Lectura_IR(void)
{
	if(Contador_Lectura_IR==99) //Si pasaron 100ms=0,1s
	{
		int Valor=0;

		Vector_IR[0]=HAL_GPIO_ReadPin(IR_Derecho);
		Vector_IR[1]=HAL_GPIO_ReadPin(IR_Central_Derecho);
		Vector_IR[2]=HAL_GPIO_ReadPin(IR_Central);
		Vector_IR[3]=HAL_GPIO_ReadPin(IR_Central_Izquierdo);
		Vector_IR[4]=HAL_GPIO_ReadPin(IR_Izquierdo);

		Valor=Vector_IR[4]*16+Vector_IR[3]*8+Vector_IR[2]*4+Vector_IR[1]*2+Vector_IR[0]*1;
		return Valor;

		Contador_Lectura_IR=0;

		HAL_GPIO_TogglePin(LED_Azul);
	}
}

void Seguidor_Linea(void)
{
	//Configuro la dirección de giro de los motores para que el vehículo siempre vaya hacia adelante
	HAL_GPIO_WritePin(IN1, 1);
	HAL_GPIO_WritePin(IN2, 0);
	HAL_GPIO_WritePin(IN3, 0);
	HAL_GPIO_WritePin(IN4, 1);

	if(Distancia_cm>20)
	{
		switch(IR_Decimal) ////Los 0 (ceros) indican la lectura de la línea negra
		{

			case 15: //01111
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Izquierdo, 65);
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Derecho, 99);
			break;

			case 7: //00111
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Izquierdo, 70);
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Derecho, 99);
			break;

			case 23: //10111
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Izquierdo, 75);
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Derecho, 90);
			break;

			case 19: //10011
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Izquierdo, 80);
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Derecho, 90);
				break;


			case 27: //11011
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Izquierdo, 90);
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Derecho, 90);
			break;


			case 25: //11001
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Izquierdo, 90);
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Derecho, 80);

			break;

			case 29: //11101
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Izquierdo, 90);
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Derecho, 75);
			break;

			case 28: //11100
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Izquierdo, 90);
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Derecho, 70);
			break;

			case 30: //11110
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Izquierdo, 90);
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Derecho, 65);
			break;

			default:
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Izquierdo, 0);
				__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Derecho, 0);
		}
	}
	else
	{
		__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Izquierdo, 0);
		__HAL_TIM_SET_COMPARE(Timer_Motor, Motor_Derecho, 0);
	}
}

void Sensor_Distancia(void) //Distancia(m) = {(Tiempo del pulso ECO) * (Velocidad del sonido=343.2m/s)}/2
{
	if(Contador_Sensor_Distancia=99)
	{
		HAL_GPIO_WritePin(TRIG, 1);
		HAL_Delay(0.0001); //Tengo que esperar 10us. La funcion HAL_Delay está en mseg
		HAL_GPIO_WritePin(TRIG, 0);

		__HAL_TIM_ENABLE_IT(Timer_ECHO, TIM_IT_CC1);

		Contador_Sensor_Distancia=0;
	}
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) //Llamada del Timer 1 en modo Input Capture, cada vez que se detecte un flanco de subida
{
	if(htim->Channel==HAL_TIM_ACTIVE_CHANNEL_1) //Si la interrupcion se debe al Canal 1 del Timer 1
	{
		if(Primer_Captura==0)
		{
			IC_Flanco_Subida=HAL_TIM_ReadCapturedValue(htim, ECHO);
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, ECHO, TIM_INPUTCHANNELPOLARITY_FALLING); //Seteo el Input Capture para detectar un flanco de bajada
			Primer_Captura=1;
		}
		else if(Primer_Captura==1)
		{
			IC_Flanco_Bajada=HAL_TIM_ReadCapturedValue(htim, ECHO);
			__HAL_TIM_SET_COUNTER(htim, 0);

			if(IC_Flanco_Bajada>IC_Flanco_Subida)
			{
				IC_Diferencia=IC_Flanco_Bajada-IC_Flanco_Subida;
			}

			else if(IC_Flanco_Subida>IC_Flanco_Bajada)
			{
				IC_Diferencia=(0xffff-IC_Flanco_Subida)+IC_Flanco_Bajada;
			}

			Distancia_cm=0.017*IC_Diferencia;

			__HAL_TIM_SET_CAPTUREPOLARITY(htim, ECHO, TIM_INPUTCHANNELPOLARITY_RISING); //Seteo el Input Capture para detectar un flanco de subida
			__HAL_TIM_DISABLE_IT(Timer_ECHO, TIM_IT_CC1);
			Primer_Captura=0;
		}
	}
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) //Cada vez que uso "HAL_ADC_Start_IT" entro a esta función
{
	//Lectura del ADC1 Canal 1 (IN1) PA1
	Sensor_Velocidad[0]=HAL_ADC_GetValue(&hadc1);
	//Lectura del ADC1 Canal 2 (IN2) PA2
	Sensor_Velocidad[1]=HAL_ADC_GetValue(&hadc1);
}










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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_8B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 16-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65536-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 16000-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 16-1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 100-1;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PE7 PE8 PE9 PE10 */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE11 PE12 PE13 PE14 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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

