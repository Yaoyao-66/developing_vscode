/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <stdio.h>
#include <string.h>
#include "STEPPER.h"
#include "pelcod_commands.h"
#include "pelco_preset.h"   // 预置点功能
#include "lens_control.h"
//#include "tmi8150b.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
//volatile uint8_t LED_Blink = 0;

volatile uint8_t PTZ_Busy = 0;
uint8_t uart_rx_byte;//单个字节接收缓冲位置
uint8_t frame_pos = 0;//接收过程种数据的位置
uint8_t pelco_frame_ready = 0;//接收完毕标志
uint8_t S_RxData[PELCO_FRAME_LEN];//
uint8_t S_TxData[7];
uint8_t S_address;
uint8_t S_command_H;
uint8_t S_command_L;
uint8_t S_data_H;
uint8_t S_data_L;
uint8_t S_checksum;

uint8_t S_step_rpm_speed;

uint16_t S_step_remaining_steps;

uint8_t S_Step_direction = DIR_CW;

uint8_t S_Servo1_angle = 0;
uint16_t S_Servo1_angle_value = 0;

uint8_t S_Servo2_angle = 0;
uint16_t S_Servo2_angle_value = 0;

uint8_t lens_auto_fz = 0 ;//镜头是自动聚焦状态还是手动控制状态，自动控制为1，手动控制为0；自检完之后，设置为1
uint8_t reveive_debug;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */

PelcoCmd Pelco_Parse(uint8_t cmd1, uint8_t cmd2)
{
    //镜头以及光圈的控制
    if (cmd2 == PELCO_CMD2_ZOOM_TELE) {lens_auto_fz = 0;return PELCO_ZOOM_TELE;}
    if (cmd2 == PELCO_CMD2_ZOOM_WIDE) {lens_auto_fz = 0;return PELCO_ZOOM_WIDE;}

    if (cmd1 == PELCO_CMD1_FOCUS_NEAR) {lens_auto_fz = 0;return PELCO_FOCUS_NEAR;}
    if (cmd2 == PELCO_CMD2_FOCUS_FAR) {lens_auto_fz = 0;return PELCO_FOCUS_FAR;}

    if (cmd1 == PELCO_CMD1_IRIS_OPEN) {lens_auto_fz = 0;return PELCO_IRIS_OPEN;}
    if (cmd1 == PELCO_CMD1_IRIS_CLOSE) {lens_auto_fz = 0;return PELCO_IRIS_CLOSE;}
    //这里还需要增加变焦镜头的停止相关条件,这段代码导致没有松手停止电机转动
    if ((cmd1 == PELCO_CMD1_CMD2_FZ_STOP)|| (cmd2 == PELCO_CMD1_CMD2_FZ_STOP)){lens_auto_fz = 1;return PELCO_ALL_STOP;}

    //电机方向的控制
    //较小范围的条件判断放前面
    if (cmd2 == PELCO_CMD2_LEFT_UP) return PELCO_PAN_LEFT_UP;
    if (cmd2 == PELCO_CMD2_LEFT_DOWN)  return PELCO_PAN_LEFT_DOWN;
    if (cmd2 == PELCO_CMD2_RIGHT_UP)  return PELCO_PAN_RIGHT_UP;
    if (cmd2 == PELCO_CMD2_RIGHT_DOWN)  return PELCO_PAN_RIGHT_DOWN;
    //较大范围的条件判断放后面
    if (cmd2 == PELCO_CMD2_PAN_LEFT) return PELCO_PAN_LEFT;
    if (cmd2 == PELCO_CMD2_PAN_RIGHT) return PELCO_PAN_RIGHT;
    if (cmd2 == PELCO_CMD2_TILT_UP) return PELCO_TILT_UP;
    if (cmd2 == PELCO_CMD2_TILT_DOWN) return PELCO_TILT_DOWN;
    //自动居中
    if(cmd2 == PELCO_CMD_RESET) return PELCO_PAN_TILT_CENTER;

    //预置点的3个操作
	if (cmd2 == PELCO_CMD2_PRESET_SET) return PELCO_PRESET_SET;
	if (cmd2 == PELCO_CMD2_PRESET_CLEAR) return PELCO_PRESET_CLEAR;
	if (cmd2 == PELCO_CMD2_PRESET_CALL) return PELCO_PRESET_CALL;
    //不匹配的命令，忽略
    return PELCO_UNMATCH_CMD;  // 默认停止
}


void PTZ_SelfTest_And_Center()
{
	PTZ_Busy = 1;   // 禁用串口指令处理（之前我教你的标志位）
	STEPPER_AUTO_PTZ_CENTER();
	PTZ_Busy = 0;   // 恢复串口指令处理
}

void FZ_SelfTest_And_Center()
{
	PTZ_Busy = 1;   // 禁用串口指令处理（之前我教你的标志位）
	STEPPER_AUTO_FZ_CENTER();
	PTZ_Busy = 0;   // 恢复串口指令处理
}

void calculate_checksum(uint8_t *data){
	data[6] = data[5]+data[4]+data[3]+data[2]+data[1];
}

uint8_t check_sum(uint8_t command_H, uint8_t command_L, uint8_t data_H, uint8_t data_L, uint8_t slave_adress){
	return (command_H + command_L + data_H + data_L + slave_adress);
}

float map( uint8_t data_H, uint8_t data_L, uint16_t max_val, uint16_t min_val){
    uint16_t bit16_val = ((uint16_t)data_H << 8) | data_L;
    return min_val + ( (float)(bit16_val) / 65535.0f ) * (max_val - min_val);
}
//把pelco-d数据整合到数组中
void generate_message(uint8_t* data ,uint8_t slave_adress, uint8_t command_H, uint8_t command_L, uint8_t data_H, uint8_t data_L){
	data[0] = 0xFF,
	data[1] = slave_adress;
	data[2] = command_H;
	data[3] = command_L;
	data[4] = data_H;
	data[5] = data_L;
	data[6] = 0x00;
}
//发送数据到master，串口或者485都可以，校验数据自动补充
void S_transmit_message(uint8_t *data, uint8_t slave_adress, uint8_t command_H, uint8_t command_L, uint8_t data_H, uint8_t data_L, uint8_t size){
	generate_message(data, slave_adress, command_H, command_L, data_H, data_L);
	calculate_checksum(data);
	HAL_GPIO_WritePin(RS485_PORT, RS485_PIN, GPIO_PIN_SET);//485 direction control  send
	HAL_UART_Transmit_IT(&huart3, data, size);
}

void S_Step_RPM_Response(uint8_t *data, uint8_t slave_adress, uint8_t speed){ //speed is between 14-0

	uint16_t result = (uint16_t)(((float)speed / 14.0f) * 65535.0f + 0.5f);
	uint8_t data_H = (result >> 8) & 0xFF;
	uint8_t data_L = result & 0xFF;

	S_transmit_message(data, slave_adress, 0x00, 0x57, data_H, data_L,7);
	HAL_UARTEx_ReceiveToIdle_IT(&huart3, &uart_rx_byte, 1);
}

void S_Step_remaining_steps_Response(uint8_t *data, uint8_t slave_adress, uint16_t remaining_steps){

	uint8_t data_H = (remaining_steps >> 8) & 0xFF;
	uint8_t data_L = remaining_steps & 0xFF;

	S_transmit_message(data, slave_adress, 0x00, 0x97, data_H, data_L,7);
	HAL_UARTEx_ReceiveToIdle_IT(&huart3, &uart_rx_byte, 1);
}

void S_Step_direction_Response(uint8_t *data, uint8_t slave_adress, uint8_t Step_direction){ // DIR_CW = 0 or DIR_CCW = 1
	S_transmit_message(data, slave_adress, 0x00, 0x77, 0x00, Step_direction,7);
	HAL_UARTEx_ReceiveToIdle_IT(&huart3, &uart_rx_byte, 1);
}

void S_Servo1_angle_Response(uint8_t *data, uint8_t slave_adress, uint8_t angle){ //angle < 360

	S_transmit_message(data, slave_adress, 0x00, 0x17, 0x00, angle,7);
	HAL_UARTEx_ReceiveToIdle_IT(&huart3, &uart_rx_byte, 1);
}

void S_Servo2_angle_Response(uint8_t *data, uint8_t slave_adress, uint8_t angle){ //angle < 360

	S_transmit_message(data, slave_adress, 0x00, 0xB7, 0x00, angle,7);
	HAL_UARTEx_ReceiveToIdle_IT(&huart3, &uart_rx_byte, 1);
}



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
  MX_USART3_UART_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  STEPPERS_Init_TMR(&htim2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
  HAL_UARTEx_ReceiveToIdle_IT(&huart3, &uart_rx_byte, 1);//这里功能是启动串口中断
  HAL_Delay(2000);

  //PTZ_SelfTest_And_Center();
  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13, GPIO_PIN_RESET);//关闭指示灯
  //STEPPER_PRESET_Init();
  FZ_SelfTest_And_Center();
  Lens_Init();


  /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
     Lens_Process();
	 if(pelco_frame_ready == 1 )
	  {
		 // HAL_UART_Transmit_IT(&huart3, S_RxData, sizeof(S_RxData));
		 // HAL_UART_Transmit_IT(&huart1, S_RxData, sizeof(S_RxData));
		  S_address   = S_RxData[1];
		  S_command_H = S_RxData[2];
		  S_command_L = S_RxData[3];
		  S_data_H    = S_RxData[4];
		  S_data_L    = S_RxData[5];
		  S_checksum  = S_RxData[6];
		  if(check_sum(S_command_H,S_command_L,S_data_H,S_data_L, SLAVE_ADRESS1) == S_checksum)
		  {
			  if(S_address == SLAVE_ADRESS1)  //常规pelco-d的命令，地址1
			  {
				  //HAL_UART_Transmit_IT(&huart3, S_RxData, sizeof(S_RxData));//与摄像头串口通讯，也做485复用
				  HAL_UART_Transmit_IT(&huart1, S_RxData, sizeof(S_RxData));//单独的调试串口
				  //以下检查云台的控制
				  switch(Pelco_Parse(S_command_H, S_command_L))
				  {
					  case PELCO_PAN_LEFT:
						  S_Step_direction = DIR_CCW;
						  STEPPER_SetSpeed(STEPPER_MOTOR1, S_data_H); //
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR1, REAL_PAN_STEPS, S_Step_direction);//一直运行
						  break;
					  case PELCO_PAN_RIGHT:
						  S_Step_direction = DIR_CW;
						  STEPPER_SetSpeed(STEPPER_MOTOR1, S_data_H); //
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR1, REAL_PAN_STEPS, S_Step_direction);//一直运行
						  break;
					  case PELCO_TILT_UP:
						  S_Step_direction = DIR_CCW;
						  STEPPER_SetSpeed(STEPPER_MOTOR2, S_data_L); //
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR2, REAL_TILT_STEPS, S_Step_direction);//一直运行
						  break;
					  case PELCO_TILT_DOWN:
						  S_Step_direction = DIR_CW;
						  STEPPER_SetSpeed(STEPPER_MOTOR2, S_data_L); //
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR2, REAL_TILT_STEPS, S_Step_direction);//一直运行
						  break;
						  //左上、左下、右上、右下
					  case PELCO_PAN_LEFT_UP:
						  S_Step_direction = DIR_CCW;
						  STEPPER_SetSpeed(STEPPER_MOTOR1, S_data_H); //
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR1, REAL_PAN_STEPS, S_Step_direction);//一直运行
						  S_Step_direction = DIR_CW;
						  STEPPER_SetSpeed(STEPPER_MOTOR2, S_data_L);
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR2, REAL_TILT_STEPS, S_Step_direction);//一直运行
						  break;
					  case PELCO_PAN_LEFT_DOWN:
						  S_Step_direction = DIR_CCW;
						  STEPPER_SetSpeed(STEPPER_MOTOR1, S_data_H); //
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR1, REAL_PAN_STEPS, S_Step_direction);//一直运行
						  S_Step_direction = DIR_CCW;
						  STEPPER_SetSpeed(STEPPER_MOTOR2, S_data_L);
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR2, REAL_TILT_STEPS, S_Step_direction);//一直运行
						  break;
					  case PELCO_PAN_RIGHT_UP:
						  S_Step_direction = DIR_CW;
						  STEPPER_SetSpeed(STEPPER_MOTOR1, S_data_H); //
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR1, REAL_PAN_STEPS, S_Step_direction);//一直运行
						  S_Step_direction = DIR_CW;
						  STEPPER_SetSpeed(STEPPER_MOTOR2, S_data_L); //
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR2, REAL_TILT_STEPS, S_Step_direction);//一直运行
						  break;
					  case PELCO_PAN_RIGHT_DOWN:
						  S_Step_direction = DIR_CW;
						  STEPPER_SetSpeed(STEPPER_MOTOR1, S_data_H); //
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR1, REAL_PAN_STEPS, S_Step_direction);//一直运行
						  S_Step_direction = DIR_CCW;
						  STEPPER_SetSpeed(STEPPER_MOTOR2, S_data_L); //
						  STEPPER_Step_NonBlocking(STEPPER_MOTOR2, REAL_TILT_STEPS, S_Step_direction);//一直运行
						  break;
					  case PELCO_PAN_TILT_CENTER:
						  PTZ_SelfTest_And_Center();
						  break;
							  case PELCO_ALL_STOP:
						  STEPPER_Stop(STEPPER_MOTOR1); // 停止运行
						  STEPPER_Stop(STEPPER_MOTOR2); // 停止运行
						  Lens_Stop();
						  break;
					  case PELCO_UNMATCH_CMD:
						  STEPPER_Stop(STEPPER_MOTOR1); // 停止运行
						  STEPPER_Stop(STEPPER_MOTOR2); // 停止运行
						  Lens_Stop();
						  break;
					//以下检测lens的控制
					  case PELCO_ZOOM_TELE:
						  Lens_ZoomMove(DIR_CCW, S_data_H ? S_data_H : 0x3F);
						  break;
					  case PELCO_ZOOM_WIDE:
						  Lens_ZoomMove(DIR_CW, S_data_H ? S_data_H : 0x3F);
						  break;
					  case PELCO_FOCUS_NEAR:
						  Lens_FocusMove(DIR_CCW, S_data_L ? S_data_L : 0x3F);
						  break;
					  case PELCO_FOCUS_FAR:
						  Lens_FocusMove(DIR_CW, S_data_L ? S_data_L : 0x3F);
						  break;
					  case PELCO_IRIS_OPEN:
						  //Iris_Open();
						  break;
					  case PELCO_IRIS_CLOSE:
						  //Iris_Close();
						  break;
					  case PELCO_FZ_STOP:
						  break;

					  //预置点
					  case PELCO_PRESET_SET:
						  STEPPER_SET_PRESET(S_data_L);
						  break;
					  case PELCO_PRESET_CLEAR:
						  STEPPER_CLEAR_PRESET(S_data_L);
						  break;
					  case PELCO_PRESET_CALL:
						  STEPPER_GOTO_PRESET(S_data_L);
						  break;
					  default:
						  break;
				  }
			  }
			  if(S_address == 0xAF) //自动聚焦的地址，实际是借用了pelco-d的格式而已
			  {
                  uint16_t af_val = ((uint16_t)S_data_H << 8) | S_data_L;
                  Lens_HandleAFValue(af_val);
			  }
			  if(S_address == 0xAE)  //3D定位，{0xFF,0xAE,左上x,左上y,右下_x1,右下_x2,sum}
			  {
				  STEPPER_GOTO_POS(500 , 200);
			  }
		  }
		  pelco_frame_ready = 0;
	  }
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  hspi2.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
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
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 19999;
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
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 19999;
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
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  htim4.Init.Prescaler = 71;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 19999;
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
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3
                           PA4 PA5 PA6 PA7
                           PA8 PA9 PA11 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
						  |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
						  |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB3
                           PB5 PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PA10 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


  /* SPI2 SCK + MOSI */
  GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;          // 关键！！
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* PB12 —— NSS（你是软件 NSS，用普通 GPIO） */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // NSS 拉高
  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART3)
    {
    	/* 如果PTZ正在自检，则丢弃所有串口数据 */
		if (PTZ_Busy)
		{
			frame_pos = 0;
			pelco_frame_ready = 0;
			HAL_UARTEx_ReceiveToIdle_IT(&huart3, &uart_rx_byte, 1);
			return;
		}
    	if ((uart_rx_byte == 0xFF)&&(frame_pos > 0))   // 检测到帧头标志
        {
            frame_pos = 0;
        }

        // 存入消息数组种
        S_RxData[frame_pos++] = uart_rx_byte;

        // 达到7字节
        if (frame_pos >= PELCO_FRAME_LEN)
        {
            frame_pos = 0;
            pelco_frame_ready = 1;
        }
        HAL_UARTEx_ReceiveToIdle_IT(&huart3, &uart_rx_byte, 1);
    }
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {

    if (huart->Instance == USART3) {

    	HAL_GPIO_WritePin(RS485_PORT, RS485_PIN, GPIO_PIN_RESET);//485 direction control

        // Send completed
    }
}
//实测GPIOC13的切换频率为500HZ，那中断的频率就是1000Hz，实测电机控制引脚的频率是125Hz
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM2){
/*		if(LED_Blink)
		{
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13, GPIO_PIN_SET);
			LED_Blink= 0 ;
		}
		else
		{
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13, GPIO_PIN_RESET);
			LED_Blink= 1 ;
		}
*/
		STEPPER_TMR_OVF_ISR(&htim2);
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
