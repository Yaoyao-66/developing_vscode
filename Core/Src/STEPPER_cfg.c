/*
 * File: STEPPER_cfg.c
 * Driver Name: [[ STEPPER Motor ]]
 * SW Layer:   ECUAL
 * Created on: Jun 28, 2020
 * Author:     Khaled Magdy
 * -------------------------------------------
 * For More Information, Tutorials, etc.
 * Visit Website: www.DeepBlueMbedded.com
 *
 */

#include "STEPPER.h"

const STEPPER_CfgType STEPPER_CfgParam[STEPPER_UNITS] =
{
    // Stepper Motor 1 Configurations
    {
		{GPIOA, GPIOA, GPIOA, GPIOA},
		{GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7},
		4096,//2048,转1圈需要4096步
		STEPPER_UNIPOLAR,
		HALF_STEP_DRIVE
    },
	// Stepper Motor 2 Configurations
    {
	    {GPIOA, GPIOA, GPIOA, GPIOA},
		{GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3},
		4096,//2048,//转1圈需要4096步
		STEPPER_UNIPOLAR,
		HALF_STEP_DRIVE
	},
	// Stepper Motor 3 Configurations   zoom
	//PA10:ZOOM A+;PA9:ZOOM A-;PA15:ZOOM B+;PB3:ZOOM B-;
    {
	    {GPIOA, GPIOA, GPIOA, GPIOB},
		{GPIO_PIN_10, GPIO_PIN_9, GPIO_PIN_15, GPIO_PIN_3},
		4684,//2048,//转1圈需要4096步
		STEPPER_BIPOLAR,
		HALF_STEP_DRIVE
	},
	// Stepper Motor 4 Configurations   focus
	//PA8:FOCUS A+;PB1:FOCUS A-;PB5:FOCUS B+;PB4:FOCUS B-;
    {
	    {GPIOA, GPIOB, GPIOB, GPIOB},
		{GPIO_PIN_8, GPIO_PIN_1, GPIO_PIN_5, GPIO_PIN_4},
		6022,//2048,//转1圈需要4096步
		STEPPER_BIPOLAR,
		HALF_STEP_DRIVE
	}
};
