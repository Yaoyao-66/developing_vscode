/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
extern SPI_HandleTypeDef hspi2;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define REAL_ZOOM_STEPS	  4684//定义水平电机持续运行的最大步数，不卡限位4000/4096*360=351°
#define REAL_FOCUS_STEPS    6022//定义垂直电机持续运行的最大步数，不卡限位1024/4096*360=90°

#define MAX_ZOOM_STEPS     REAL_ZOOM_STEPS+100//定义水平电机持续运行的最大步数,会卡到机壳的限位
#define MAX_FOCUS_STEPS    REAL_FOCUS_STEPS+200//定义垂直电机持续运行的最大步数,会卡到机壳的限位

#define LEFT_ZOOM_STEPS	  -(REAL_ZOOM_STEPS/2)
#define RIGHT_ZOOM_STEPS    (REAL_ZOOM_STEPS/2)

#define DOWN_FOCUS_STEPS   -(REAL_FOCUS_STEPS/2)//定义垂直电机持续运行的最大步数，不卡限位
#define UP_FOCUS_STEPS	   (REAL_FOCUS_STEPS/2)//定义水平电机持续运行的最大步数，不卡限位


#define REAL_PAN_STEPS	  4000//定义水平电机持续运行的最大步数，不卡限位4000/4096*360=351°
#define REAL_TILT_STEPS    1024//定义垂直电机持续运行的最大步数，不卡限位1024/4096*360=90°

#define MAX_PAN_STEPS     REAL_PAN_STEPS+50//定义水平电机持续运行的最大步数,会卡到机壳的限位
#define MAX_TILT_STEPS    REAL_TILT_STEPS+16//定义垂直电机持续运行的最大步数,会卡到机壳的限位

#define LEFT_PAN_STEPS	  -(REAL_PAN_STEPS/2)
#define RIGHT_PAN_STEPS    (REAL_PAN_STEPS/2)

#define DOWN_TILT_STEPS   -(REAL_TILT_STEPS/2)//定义垂直电机持续运行的最大步数，不卡限位
#define UP_TILT_STEPS	   (REAL_TILT_STEPS/2)//定义水平电机持续运行的最大步数，不卡限位

//这里是pelco-d发的数据范围，实际需要进行转化，在STEPPER_SetSpeed()函数里面继续
//60000毫秒，RPM表示每分钟多少转，常规的24BJY48步进电机，内部步距角一般是5.625°/64步，外面齿轮的减速比也是1/64，所以外圈电机转1圈需要4096步
//假如电机的空载牵入频率500Hz，那么实际外圈看相当于500/4096=0.122RMP，即每秒0.122圈，每分钟0.122*60=7.3RMP，设置RMP_K_factor=7.3
#define RMP_K_factor_PTZ		7.3f  //24BJY48步进电机，按照4096步/圈来算，使用在STEPPER_SetSpeed()里面
#define RMP_K_factor_FZ			12.0f
#define MAX_PELCOD_SPEED 	0x3F
#define MIN_PELCOD_SPEED 	0x00
//参考前面，可能单独使用
#define MAX_PAN_SPEED 	 	0x3F  //同前面
#define MIN_PAN_SPEED 		0x00
#define MAX_TILT_SPEED 		0x3F  //同前面
#define MIN_TILT_SPEED 		0x00

#define MAX_ZOOM_SPEED 	 	0x3F  //同前面
#define MIN_ZOOM_SPEED 		0x00
#define MAX_FOCUS_SPEED 	0x3F  //同前面
#define MIN_FOCUS_SPEED 	0x00

#define STEPPER_MOTOR1   0
#define STEPPER_MOTOR2   1
#define STEPPER_MOTOR3   2
#define STEPPER_MOTOR4   3
#define SLAVE_ADRESS1 1
#define SLAVE_ADRESS2 2
#define SLAVE_ADRESS3 3

#define RS485_PORT GPIOB
#define RS485_PIN  GPIO_PIN_1

#define PELCO_FRAME_LEN 7   // Pelco-D 协议固定长度 7 字节
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
typedef enum {
  PELCO_ALL_STOP = 0,           // 停止 PTZ

  PELCO_PAN_LEFT,           // 左
  PELCO_PAN_RIGHT,          // 右
  PELCO_TILT_UP,            // 上
  PELCO_TILT_DOWN,          // 下

  PELCO_PAN_LEFT_UP,        // 左上
  PELCO_PAN_LEFT_DOWN,      // 左下
  PELCO_PAN_RIGHT_UP,       // 右上
  PELCO_PAN_RIGHT_DOWN,     // 右下

  PELCO_PAN_TILT_CENTER,    //复位自动居中

  PELCO_FZ_STOP,
  PELCO_ZOOM_TELE,    // 拉近
  PELCO_ZOOM_WIDE,          // 拉远
  PELCO_ZOOM_STOP,          // 变倍停止

  PELCO_FOCUS_NEAR,         // 聚焦近
  PELCO_FOCUS_FAR,          // 聚焦远
  PELCO_FOCUS_STOP,         // 聚焦停止

  PELCO_IRIS_OPEN,          // 光圈开
  PELCO_IRIS_CLOSE,         // 光圈关
  PELCO_IRIS_STOP,          // 光圈停止

  PELCO_AUTOSCAN_ON,  		// 自动扫描开
  PELCO_AUTOSCAN_OFF,       // 自动扫描关

  PELCO_AUX_ON,              // 辅助设备开（如灯/雨刷）
  PELCO_AUX_OFF,             // 辅助设备关

  PELCO_PRESET_SET,          // 预置点设置
  PELCO_PRESET_CLEAR,        // 预置点清除
  PELCO_PRESET_CALL,         // 预置点调用

  PELCO_UNMATCH_CMD			//不匹配的命令
} PelcoCmd;
/* Private variables ---------------------------------------------------------*/


/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
