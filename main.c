#include "main.h"
#include "math.h"
#include <stdio.h>
#include <string.h>
#include "Display_16x2_I2C.h"
#include "RFID_RC522_SPI.h"

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart6;
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);

void Sensores_Distancia(void);
void Lectura_RFID(void);
void Control_Barrera_Entrada(void);
void Control_Barrera_Salida(void);
void UART_Tx_Rx(void);
void Mensaje_Display(void);

#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
#define LED_Azul GPIOD, GPIO_PIN_15 //El LED azul del STM32F411 corresponde a PD15

//3 Sensores Ultrasonicos
#define TRIG_Entrada GPIOA, GPIO_PIN_8 //Ccorrresponde a PA8, Puerto A y Pin 8
#define TRIG_Salida_1 GPIOA, GPIO_PIN_9 //Corresponde a PA9, Puerto A y Pin 9
#define TRIG_Salida_2 GPIOA, GPIO_PIN_10 //Corresponde a PA10
#define ECHO_Timer &htim3 //Timer 3
#define ECHO_Entrada TIM_CHANNEL_1 //El Timer 3 Canal 1 corresponde a PA6
#define ECHO_Salida_1 TIM_CHANNEL_2 //El Timer 3 Canal 2 corresponde a PA7
#define ECHO_Salida_2 TIM_CHANNEL_3 //El Timer 3 Canal 3 corresponde a PB0

//Control Barreras
#define OFF 0
#define ON 1
#define IDLE 0
#define CONFIRM 1

//2 Servomotores
//El periodo del servomotor tiene que ser de 20mseg para el PWM
#define Servo_Timer &htim2
#define Servo_Entrada TIM_CHANNEL_3 //El Timer 2 Canal 3 corresponde a PA2
#define Servo_Salida TIM_CHANNEL_4 //El Timer 2 Canal 4 corresponde a PA3

//UART
#define OCUPADO 0
#define LISTO 1
#define VACIO 0
#define LLENO 1

uint16_t Contador_Sensor_Distancia_Entrada=0;
uint16_t Contador_Sensor_Distancia_Salida_1=0;
uint16_t Contador_Sensor_Distancia_Salida_2=0;
uint16_t Contador_Display=0;
uint16_t Contador_UART=0;
uint16_t Contador_Lector_RFID=0;

//El IC (Input Capture) del Timer 3 captura valores de 16bits
uint8_t Primer_Captura_Entrada=0;
uint16_t IC_Flanco_Subida_Entrada=0;
uint16_t IC_Flanco_Bajada_Entrada=0;
uint16_t IC_Diferencia_Entrada=0;
uint8_t Distancia_cm_Entrada=0;

uint8_t Primer_Captura_Salida_1=0;
uint16_t IC_Flanco_Subida_Salida_1=0;
uint16_t IC_Flanco_Bajada_Salida_1=0;
uint16_t IC_Diferencia_Salida_1=0;
uint8_t Distancia_cm_Salida_1=0;

uint8_t Primer_Captura_Salida_2=0;
uint16_t IC_Flanco_Subida_Salida_2=0;
uint16_t IC_Flanco_Bajada_Salida_2=0;
uint16_t IC_Diferencia_Salida_2=0;
uint8_t Distancia_cm_Salida_2=0;

//Uart
char Tx_Buffer_Uart[200];
uint8_t Tx_Estado=LISTO;
char Rx_Buffer_Uart[1];
uint8_t Rx_Dato=VACIO;

//Control Barreras
uint8_t Flag_Control_Barrera_Entrada=0;
uint8_t Flag_Control_Barrera_Salida=0;
uint8_t Flag_Cruce_Entrada=OFF;
uint8_t Flag_Cruce_Salida_1=OFF;
uint8_t Flag_Cruce_Salida_2=OFF;
uint8_t Flag_RFID=OFF;

uint8_t Cantidad_Autos=0;
uint8_t Estacionamientos_Disponibles=0;
char Estacionamientos[0];

//Lector RFID
uint8_t Estado;
uint8_t String[16]; //MAX length = 16
uint8_t Numero_Serie[5];

char Distancia_Entrada[4]="";
char Distancia_Salida_1[4]="";
char Distancia_Salida_2[4]="";

int main(void)
{
	HAL_Init();
	SystemClock_Config();

	MX_GPIO_Init();
	MX_TIM1_Init();
	MX_TIM2_Init();
	MX_TIM3_Init();
	MX_USART6_UART_Init();
	MX_I2C1_Init();
	MX_SPI2_Init();

	Display_Inicializacion();
	MFRC522_Init();

	HAL_TIM_Base_Start_IT(&htim1); //Inicia el Timer 1 como base de tiempos por interrupcion, interrumpe cada 1mseg

	HAL_TIM_IC_Start_IT(ECHO_Timer, ECHO_Entrada); //Inicia el Canal 1 del Timer 3 como Input Capture (IC) Direct Mode
	HAL_TIM_IC_Start_IT(ECHO_Timer, ECHO_Salida_1); //Inicia el Canal 2 del Timer 3 como Input Capture (IC) Direct Mode
	HAL_TIM_IC_Start_IT(ECHO_Timer, ECHO_Salida_2); //Inicia el Canal 3 del Timer 3 como Input Capture (IC) Direct Mode

	HAL_TIM_PWM_Start(Servo_Timer, Servo_Entrada);
	HAL_TIM_PWM_Start(Servo_Timer, Servo_Salida);

	HAL_UART_Receive_IT(&huart6, (uint8_t *)Rx_Buffer_Uart, 1);

	Display_Ubicar_Cursor(0, 0);
	Display_Enviar_String("Estacionamientos");
	Display_Ubicar_Cursor(1, 0);
	Display_Enviar_String("disponibles: ");

	while (1)
	{
		Sensores_Distancia();
		Lectura_RFID();
		Control_Barrera_Entrada();
		Control_Barrera_Salida();
		UART_Tx_Rx();
		Mensaje_Display();
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) //El Timer 1 está configurado para que interrumpa cada 1ms
{
	Contador_Sensor_Distancia_Entrada++;
	Contador_Sensor_Distancia_Salida_1++;
	Contador_Sensor_Distancia_Salida_2++;

	Contador_Lector_RFID++;
	Contador_Display++;
	Contador_UART++;
}

void Sensores_Distancia(void)
{
	if(Contador_Sensor_Distancia_Entrada>=925)
	{
		//Activo los Triggers de ambos sensores de ultrasonido
		HAL_GPIO_WritePin(TRIG_Entrada, 1);
		HAL_Delay(0.0001); //El delay tiene que ser de aproximadamente 10useg
		//Desactivo los Triggers de ambos sensores de ultrasonido
		HAL_GPIO_WritePin(TRIG_Entrada, 0);

		__HAL_TIM_ENABLE_IT(ECHO_Timer, TIM_IT_CC1);
		Contador_Sensor_Distancia_Entrada=0;
	}

	if(Contador_Sensor_Distancia_Salida_1>=950)
	{
		HAL_GPIO_WritePin(TRIG_Salida_1, 1);
		HAL_Delay(0.0001); //El delay tiene que ser de aproximadamente 10useg
		HAL_GPIO_WritePin(TRIG_Salida_1, 0);

		__HAL_TIM_ENABLE_IT(ECHO_Timer, TIM_IT_CC2);
		Contador_Sensor_Distancia_Salida_1=25;
	}

	if(Contador_Sensor_Distancia_Salida_2>=975)
	{
		HAL_GPIO_WritePin(TRIG_Salida_2, 1);
		HAL_Delay(0.0001); //El delay tiene que ser de aproximadamente 10useg
		HAL_GPIO_WritePin(TRIG_Salida_2, 0);

		__HAL_TIM_ENABLE_IT(ECHO_Timer, TIM_IT_CC3);
		Contador_Sensor_Distancia_Salida_2=50;
	}
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) //Llamada del Timer 3 en modo Input Capture, cada vez que se detecte un flanco de subida
{
	if(htim->Channel==HAL_TIM_ACTIVE_CHANNEL_1) //Si la interrupcion se debe al Canal 1 del Timer 3
	{
		if(Primer_Captura_Entrada==0)
		{
			IC_Flanco_Subida_Entrada=HAL_TIM_ReadCapturedValue(htim, ECHO_Entrada);
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, ECHO_Entrada, TIM_INPUTCHANNELPOLARITY_FALLING); //Seteo el Input Capture para que detecte un flanco de bajada
			Primer_Captura_Entrada=1;
		}
		else if (Primer_Captura_Entrada==1)
		{
			IC_Flanco_Bajada_Entrada=HAL_TIM_ReadCapturedValue(htim, ECHO_Entrada);
			__HAL_TIM_SET_COUNTER(htim, 0);

			if(IC_Flanco_Bajada_Entrada>IC_Flanco_Subida_Entrada)
			{
				IC_Diferencia_Entrada=IC_Flanco_Bajada_Entrada-IC_Flanco_Subida_Entrada;
			}

			else if(IC_Flanco_Subida_Entrada>IC_Flanco_Bajada_Entrada)
			{
				IC_Diferencia_Entrada=(0xffff-IC_Flanco_Subida_Entrada)+IC_Flanco_Bajada_Entrada;
			}

			Distancia_cm_Entrada=0.017*IC_Diferencia_Entrada;

			__HAL_TIM_SET_CAPTUREPOLARITY(htim, ECHO_Entrada, TIM_INPUTCHANNELPOLARITY_RISING); //Seteo el Input Capture para que detecte un flanco de subida
			__HAL_TIM_DISABLE_IT(ECHO_Timer, TIM_IT_CC1);
			Primer_Captura_Entrada=0;
		}
	}

	if(htim->Channel==HAL_TIM_ACTIVE_CHANNEL_2) //Si la interrupcion se debe al Canal 2 del Timer 2
	{
		if(Primer_Captura_Salida_1==0)
		{
			IC_Flanco_Subida_Salida_1=HAL_TIM_ReadCapturedValue(htim, ECHO_Salida_1);
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, ECHO_Salida_1, TIM_INPUTCHANNELPOLARITY_FALLING); //Seteo el Input Capture para que detecte un flanco de bajada
			Primer_Captura_Salida_1=1;
		}
		else if (Primer_Captura_Salida_1==1)
		{
			IC_Flanco_Bajada_Salida_1=HAL_TIM_ReadCapturedValue(htim, ECHO_Salida_1);
			__HAL_TIM_SET_COUNTER(htim, 0);

			if(IC_Flanco_Bajada_Salida_1>IC_Flanco_Subida_Salida_1)
			{
				IC_Diferencia_Salida_1=IC_Flanco_Bajada_Salida_1-IC_Flanco_Subida_Salida_1;
			}

			else if(IC_Flanco_Subida_Salida_1>IC_Flanco_Bajada_Salida_1)
			{
				IC_Diferencia_Salida_1=(0xffff-IC_Flanco_Subida_Salida_1)+IC_Flanco_Bajada_Salida_1;
			}

			Distancia_cm_Salida_1=0.017*IC_Diferencia_Salida_1;

			__HAL_TIM_SET_CAPTUREPOLARITY(htim, ECHO_Salida_1, TIM_INPUTCHANNELPOLARITY_RISING); //Seteo el Input Capture para que detecte un flanco de subida
			__HAL_TIM_DISABLE_IT(ECHO_Timer, TIM_IT_CC2);
			Primer_Captura_Salida_1=0;
		}
	}

	if(htim->Channel==HAL_TIM_ACTIVE_CHANNEL_3) //Si la interrupcion se debe al Canal 3 del Timer 3
	{
		if(Primer_Captura_Salida_2==0)
		{
			IC_Flanco_Subida_Salida_2=HAL_TIM_ReadCapturedValue(htim, ECHO_Salida_2);
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, ECHO_Salida_2, TIM_INPUTCHANNELPOLARITY_FALLING); //Seteo el Input Capture para que detecte un flanco de bajada
			Primer_Captura_Salida_2=1;
		}
		else if (Primer_Captura_Salida_2==1)
		{
			IC_Flanco_Bajada_Salida_2=HAL_TIM_ReadCapturedValue(htim, ECHO_Salida_2);
			__HAL_TIM_SET_COUNTER(htim, 0);

			if(IC_Flanco_Bajada_Salida_2>IC_Flanco_Subida_Salida_2)
			{
				IC_Diferencia_Salida_2=IC_Flanco_Bajada_Salida_2-IC_Flanco_Subida_Salida_2;
			}

			else if(IC_Flanco_Subida_Salida_2>IC_Flanco_Bajada_Salida_2)
			{
				IC_Diferencia_Salida_2=(0xffff-IC_Flanco_Subida_Salida_2)+IC_Flanco_Bajada_Salida_2;
			}

			Distancia_cm_Salida_2=0.017*IC_Diferencia_Salida_2;

			__HAL_TIM_SET_CAPTUREPOLARITY(htim, ECHO_Salida_2, TIM_INPUTCHANNELPOLARITY_RISING); //Seteo el Input Capture para que detecte un flanco de subida
			__HAL_TIM_DISABLE_IT(ECHO_Timer, TIM_IT_CC3);
			Primer_Captura_Salida_2=0;
		}
	}

	Flag_Control_Barrera_Entrada=1;
	Flag_Control_Barrera_Salida=1;
}

void Lectura_RFID(void) //3,3v=RST=3v, GND=0v, MISO: PC2, MOSI: PC3, SCK: PB10, SDA: PA4
{
	if(Contador_Lector_RFID>=999)
	{
		Estado=MFRC522_Request(PICC_REQIDL, String);
		Estado=MFRC522_Anticoll(String);
		memcpy(Numero_Serie, String, 5); //Se alamacena el numero de serie de la tarjeta RFID

		if(Estado==MI_OK)
		{
			if((Numero_Serie[0]==50) && (Numero_Serie[1]==132) && (Numero_Serie[2]==177) && (Numero_Serie[3]==1) && (Numero_Serie[4]==6)) //Tarjeta 1
			{
				__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Entrada, 2); //Abro la barrera. Con un CCR de 2 el servo esta a 90°, ciclo de actividad de 2ms
				Flag_RFID=ON;
			}

			else if((Numero_Serie[0]==34) && (Numero_Serie[1]==209) && (Numero_Serie[2]==153) && (Numero_Serie[3]==1) && (Numero_Serie[4]==107)) //Trajeta 2
			{
				__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Entrada, 2); //Abro la barrera. Con un CCR de 2 el servo esta a 90°, ciclo de actividad de 2ms
				Flag_RFID=ON;
			}

			else if((Numero_Serie[0]==50) && (Numero_Serie[1]==80) && (Numero_Serie[2]==255) && (Numero_Serie[3]==1) && (Numero_Serie[4]==156))
			{
				__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Entrada, 2); //Abro la barrera. Con un CCR de 2 el servo esta a 90°, ciclo de actividad de 2ms
				Flag_RFID=ON;
			}

			else //if((Numero_Serie[1]!=132) && (Numero_Serie[1]!=209) && (Numero_Serie[1]!=80))
			{
				Flag_RFID=OFF;
			}
		}

		Contador_Lector_RFID=0;
	}
}

void Control_Barrera_Entrada(void)
{
//----------Barrera de Entrada--------->Trabajan el Canal 1 del Timer 3 y el Lector RFID

	if(Flag_Control_Barrera_Entrada==1)
	{
		static uint8_t Estado_Ultrasonido_Entrada=IDLE; //Estado inicial

		switch(Estado_Ultrasonido_Entrada)
		{
			case IDLE:
				if(Distancia_cm_Entrada<5 && Distancia_cm_Entrada>0 && Flag_RFID==ON && Cantidad_Autos<8)
				{
					Estado_Ultrasonido_Entrada=CONFIRM;
				}
			break;

			case CONFIRM:
				if(Distancia_cm_Entrada>5)
				{
					Estado_Ultrasonido_Entrada=IDLE;
					__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Entrada, 1); //Cierro la barrera. Con un CCR de 1 el servo esta a 0°, ciclo de actividad de 1ms
					Flag_Cruce_Entrada=ON;
				}
			break;
		}

		if(Flag_Cruce_Entrada==ON)
		{
			Flag_Cruce_Entrada=OFF;
			Flag_RFID=OFF;
			Cantidad_Autos++; //Cuento autos
		}

		Flag_Control_Barrera_Entrada=0;
	}
}

void Control_Barrera_Salida(void)
{
	//El PWM para los servos esta configurado para que al 100% de ciclo de actividad (19) sea un estado alto de 20ms
	/*__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Salida, 1); //Con un CCR de 1 el servo esta a 0°, ciclo de actividad de 1ms
	HAL_Delay(3000);
	__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Salida, 2); //Con un CCR de 2 el servo esta a 90°, ciclo de actividad de 2ms
	HAL_Delay(3000);*/

//----------Barrera de Salida--------->Trabajan el Canal 2 y el Canal 3 del Timer 3

	if(Flag_Control_Barrera_Salida==1)
	{
		static uint8_t Estado_Ultrasonido_Salida_1=IDLE; //Estado inicial

		switch(Estado_Ultrasonido_Salida_1)
		{
			case IDLE:
				if(Distancia_cm_Salida_1<5 && Distancia_cm_Salida_1>0 && Cantidad_Autos>0)
				{
					Estado_Ultrasonido_Salida_1=CONFIRM;
					__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Salida, 2); //Abro la barrera. Con un CCR de 2 el servo esta a 90°, ciclo de actividad de 2ms
				}
			break;

			case CONFIRM:
				if(Distancia_cm_Salida_1>5)
				{
					Estado_Ultrasonido_Salida_1=IDLE;
					Flag_Cruce_Salida_1=ON;
				}
			break;
		}

		static uint8_t Estado_Ultrasonido_Salida_2=IDLE; //Estado inicial

		switch(Estado_Ultrasonido_Salida_2)
		{
			case IDLE:
				if(Distancia_cm_Salida_2<5 && Distancia_cm_Salida_2>0 && Flag_Cruce_Salida_1==ON && Cantidad_Autos>0)
				{
					Estado_Ultrasonido_Salida_2=CONFIRM;
				}
			break;

			case CONFIRM:
				if(Distancia_cm_Salida_2>5)
				{
					Estado_Ultrasonido_Salida_2=IDLE;
					__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Salida, 1); //Cierro la barrera. Con un CCR de 1 el servo esta a 0°, ciclo de actividad de 1ms
					Flag_Cruce_Salida_2=ON;
				}
				break;
		}

		if(Flag_Cruce_Salida_2==ON)
		{
			Flag_Cruce_Salida_1=OFF;
			Flag_Cruce_Salida_2=OFF;
			Cantidad_Autos--; //Descuento autos
		}

		Flag_Control_Barrera_Salida=0;
	}
}

void UART_Tx_Rx(void)
{
	if(Contador_UART>=999)
	{
		if(Tx_Estado==LISTO) //Transmito
		{
			switch(Cantidad_Autos)
			{
				case 8:
					sprintf(Tx_Buffer_Uart, " %d/8 \n LLENO \n", Cantidad_Autos);
					HAL_UART_Transmit_IT(&huart6, (uint8_t *)Tx_Buffer_Uart, strlen(Tx_Buffer_Uart));
					break;

				default:
					sprintf(Tx_Buffer_Uart, " %d/8 \n DISPONIBLE \n", Cantidad_Autos);
					HAL_UART_Transmit_IT(&huart6, (uint8_t *)Tx_Buffer_Uart, strlen(Tx_Buffer_Uart));
					break;
			}

			Tx_Estado==OCUPADO;
		}

	Contador_UART=0;
	}

	if(Rx_Dato==LLENO) //Recibo
	{
		switch(Rx_Buffer_Uart[0])
		{
			case 'a': //Abrir la barrera de entrada
				__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Entrada, 2); //Abro la barrera. Con un CCR de 2 el servo esta a 90°, ciclo de actividad de 2ms
			break;

			case 'b': //Cerrar la barrera de entrada
				__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Entrada, 1); //Cierro la barrera. Con un CCR de 1 el servo esta a 0°, ciclo de actividad de 1ms
			break;

			case 'c': //Abrir la barrera de salida
				__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Salida, 2); //Abro la barrera. Con un CCR de 2 el servo esta a 90°, ciclo de actividad de 2ms
			break;

			case 'd': //Cerrar la barrera de salida
				__HAL_TIM_SET_COMPARE(Servo_Timer, Servo_Salida, 1); //Cierro la barrera. Con un CCR de 1 el servo esta a 0°, ciclo de actividad de 1ms
			break;

			default:
			break;
		}

		//Rx_Buffer_Uart[0]="";
		Rx_Dato=VACIO;
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	Tx_Estado=LISTO;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	Rx_Dato=LLENO;
	HAL_UART_Receive_IT(&huart6, (uint8_t *)Rx_Buffer_Uart, 1);
}

void Mensaje_Display(void)
{
	if(Contador_Display>=999)
	{
		/*Display_Ubicar_Cursor(0, 0);
		Display_Enviar_String("D1: ");
		sprintf(Distancia_Entrada, "%d", Distancia_cm_Entrada);
		Display_Enviar_String(Distancia_Entrada);
		Display_Enviar_String("   ");

		Display_Ubicar_Cursor(0, 9);
		Display_Enviar_String("D2: ");
		sprintf(Distancia_Salida_1, "%d", Distancia_cm_Salida_1);
		Display_Enviar_String(Distancia_Salida_1);
		Display_Enviar_String("   ");

		Display_Ubicar_Cursor(1, 0);
		Display_Enviar_String("Distancia 3: ");
		sprintf(Distancia_Salida_2, "%d", Distancia_cm_Salida_2);
		Display_Enviar_String(Distancia_Salida_2);
		Display_Enviar_String("   ");*/

		Estacionamientos_Disponibles=8-Cantidad_Autos;

		Display_Ubicar_Cursor(1, 13);
		sprintf(Estacionamientos, "%d", Estacionamientos_Disponibles);
		Display_Enviar_String(Estacionamientos);
		//Display_Enviar_String(" ");

		Contador_Display=0;
		HAL_GPIO_TogglePin(LED_Azul);
	}
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
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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
  htim1.Init.Prescaler = 16-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000-1;
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
  if (HAL_TIM_OC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
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
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 16000-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 20-1;
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
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 16-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65536-1;
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
  if (HAL_TIM_IC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 9600;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA4 PA8 PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

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

