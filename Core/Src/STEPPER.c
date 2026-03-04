/*
 * File: STEPPER.c
 * Driver Name: [[ STEPPER Motor ]]
 * SW Layer:   ECUAL
 * Created on: Jun 28, 2020
 * Author:     Khaled Magdy
 * -------------------------------------------
 * For More Information, Tutorials, etc.
 * Visit Website: www.DeepBlueMbedded.com
 *
 */
#include "main.h"//补充了一些宏定义的参数
#include "DWT_Delay.h"
#include "STEPPER.h"
#include "STEPPER_cfg.h"


static STEPPER_info gs_STEPPER_info[STEPPER_UNITS] = {0};

static uint8_t UNIPOLAR_WD_PATTERN[4][4] = {
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
};
static uint8_t UNIPOLAR_FS_PATTERN[4][4] = {
		{1, 1, 0, 0},
		{0, 1, 1, 0},
		{0, 0, 1, 1},
		{1, 0, 0, 1}
};
static uint8_t UNIPOLAR_HS_PATTERN[8][4] = {
		{1, 0, 0, 0},
		{1, 1, 0, 0},
		{0, 1, 0, 0},
		{0, 1, 1, 0},
		{0, 0, 1, 0},
		{0, 0, 1, 1},
		{0, 0, 0, 1},
		{1, 0, 0, 1}
};
//	A+橙/A-绿/B+黄/B-紫
static uint8_t POLAR_FZ_PATTERN[8][4] = {
		{1, 0, 1, 0},
		{1, 0, 0, 0},
		{1, 0, 0, 1},
		{0, 0, 0, 1},
		{0, 1, 0, 1},
		{0, 1, 0, 0},
		{0, 1, 1, 0},
		{0, 0, 1, 0}
};
/*
static uint8_t POLAR_FZ_PATTERN[8][4] = {
		{1, 0, 0, 1},
		{1, 0, 0, 1},
		{1, 0, 1, 0},
		{1, 0, 1, 0},
		{0, 1, 1, 0},
		{0, 1, 1, 0},
		{0, 1, 0, 1},
		{0, 1, 0, 1}
};*/


/* 工具函数：返回实例对应的软限位范围 */
static inline int32_t stepper_min_pos(uint8_t inst)
{
    if(inst == STEPPER_MOTOR1) return LEFT_PAN_STEPS;
    else if(inst == STEPPER_MOTOR2) return DOWN_TILT_STEPS;
    else if(inst == STEPPER_MOTOR3) return 0; // Zoom 最小 0
    else if(inst == STEPPER_MOTOR4) return 0; // Focus 最小 0
    else return 0;
}
static inline int32_t stepper_max_pos(uint8_t inst)
{
    if(inst == STEPPER_MOTOR1) return RIGHT_PAN_STEPS;
    else if(inst == STEPPER_MOTOR2) return UP_TILT_STEPS;
    else if(inst == STEPPER_MOTOR3) return REAL_ZOOM_STEPS; // Zoom 最大行程
    else if(inst == STEPPER_MOTOR4) return REAL_FOCUS_STEPS; // Focus 最大行程
    else return 0;
}

//----------------------------[ Functions' Definitions ]---------------------------

void STEPPERS_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint8_t i = 0, j = 0;

    DWT_Delay_Init();

    /*--------[ Configure The Stepper IN(1-4) GPIO Pins ]-------*/
    for(i = 0; i<STEPPER_UNITS; i++)
    {
    	for(j=0; j<4; j++)
    	{
    		if(STEPPER_CfgParam[i].IN_GPIO[j] == GPIOA)
    		{
    		    __HAL_RCC_GPIOA_CLK_ENABLE();
    		}
    		else if(STEPPER_CfgParam[i].IN_GPIO[j] == GPIOB)
    		{
    		    __HAL_RCC_GPIOB_CLK_ENABLE();
    		}
    		else if(STEPPER_CfgParam[i].IN_GPIO[j] == GPIOC)
    		{
    		    __HAL_RCC_GPIOC_CLK_ENABLE();
    		}/*
    		else if(STEPPER_CfgParam[i].IN_GPIO[j] == GPIOD)
    		{
    		    __HAL_RCC_GPIOD_CLK_ENABLE();
    		}
    		else if(STEPPER_CfgParam[i].IN_GPIO[j] == GPIOE)
    		{
    		    __HAL_RCC_GPIOE_CLK_ENABLE();
    		}*/
    		GPIO_InitStruct.Pin = STEPPER_CfgParam[i].IN_PIN[j];
    		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    		GPIO_InitStruct.Pull = GPIO_NOPULL;
    		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    		HAL_GPIO_Init(STEPPER_CfgParam[i].IN_GPIO[j], &GPIO_InitStruct);
    	}
    	gs_STEPPER_info[i].Dir = DIR_CW;
    	gs_STEPPER_info[i].Step_Index = 0;
    	gs_STEPPER_info[i].Steps = 0;
    	gs_STEPPER_info[i].Ticks = 0;
    	gs_STEPPER_info[i].Max_Ticks = 0;
    	gs_STEPPER_info[i].Blocked = 0;
    	gs_STEPPER_info[i].Motor_pos = 0;//新增加的电机位置
    	if(STEPPER_CfgParam[i].STEPPING_Mode == FULL_STEP_DRIVE || STEPPER_CfgParam[i].STEPPING_Mode == WAVE_DRIVE)
    	{
    		gs_STEPPER_info[i].Max_Index = 4;
    	}
    	else if(STEPPER_CfgParam[i].STEPPING_Mode == HALF_STEP_DRIVE)
    	{
    		gs_STEPPER_info[i].Max_Index = 8;
    	}
    }
}

void STEPPERS_Init_TMR(TIM_HandleTypeDef* TMR_Handle)
{
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    uint32_t ARR_Value = 0;

	STEPPERS_Init();

	/*--------[ Configure The Stepper Timer Base If Enabled ]-------*/
    if(STEPPER_TIMER_EN == 1)
    {
    	ARR_Value = (STEPPER_TIMER_CLK * 10.0 * STEPPER_TIME_BASE);
    	TMR_Handle->Instance = STEPPER_TIMER;
    	TMR_Handle->Init.Prescaler = 99;
    	TMR_Handle->Init.CounterMode = TIM_COUNTERMODE_UP;
    	TMR_Handle->Init.Period = ARR_Value-1;
    	TMR_Handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    	TMR_Handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    	HAL_TIM_Base_Init(TMR_Handle);
    	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    	HAL_TIM_ConfigClockSource(TMR_Handle, &sClockSourceConfig);
    	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    	HAL_TIMEx_MasterConfigSynchronization(TMR_Handle, &sMasterConfig);
    	HAL_TIM_Base_Start_IT(TMR_Handle);
    }
}

void STEPPER_SetSpeed(uint8_t au8_STEPPER_Instance, uint16_t au16_RPM)
{
	uint32_t Total_Steps = 0;
	float speed_float;
	if((au16_RPM<MIN_PELCOD_SPEED)||(au16_RPM>MAX_PELCOD_SPEED)) return;//不合适的速度值
	gs_STEPPER_info[au8_STEPPER_Instance].RPM = au16_RPM;
	if(STEPPER_CfgParam[au8_STEPPER_Instance].STEPPING_Mode == HALF_STEP_DRIVE)
	{
		Total_Steps = STEPPER_CfgParam[au8_STEPPER_Instance].STEPS_PER_REV << 1;
	}
	else
	{
		Total_Steps = STEPPER_CfgParam[au8_STEPPER_Instance].STEPS_PER_REV;
	}
	//60000毫秒，RPM表示每分钟多少转，常规的24BJY48步进电机，内部步距角一般是5.625°/64步，外面齿轮的减速比也是1/64，所以外圈电机转1圈需要4096步
	//假如电机的空载牵入频率500Hz，那么实际外圈看相当于500/4096=0.122RMP，即每秒0.122圈，每分钟0.122*60=7.3RMP，设置RMP_K_factor=7.3
	//那么我们现在就需要在pelco-d速度命令0x3F的时候，角速度7.3RMP；当STEPPER_TIME_BASE=1，Total_Steps=4096，au16_RPM应该=2
	if((au8_STEPPER_Instance == STEPPER_MOTOR1 )||(au8_STEPPER_Instance == STEPPER_MOTOR2 ))
	speed_float = (float)RMP_K_factor_PTZ * (float)au16_RPM / (float)MAX_PELCOD_SPEED;
	//变焦镜头慢一些，算6RPM的速度吧，大约是410Hz
	if((au8_STEPPER_Instance == STEPPER_MOTOR3 )||(au8_STEPPER_Instance == STEPPER_MOTOR4 ))
	speed_float = (float)RMP_K_factor_FZ * (float)au16_RPM / (float)MAX_PELCOD_SPEED;
	gs_STEPPER_info[au8_STEPPER_Instance].Max_Ticks = (60000.0)/(STEPPER_TIME_BASE * Total_Steps * speed_float);
}

static void STEPPER_One_Step(uint8_t i)
{
	// For UniPolar Stepper Motors
	if(STEPPER_CfgParam[i].STEPPER_Cfg == STEPPER_UNIPOLAR)
	{
		if(STEPPER_CfgParam[i].STEPPING_Mode == WAVE_DRIVE)
		{
			for (uint8_t j = 0; j < 4; j++)
			{
				HAL_GPIO_WritePin(STEPPER_CfgParam[i].IN_GPIO[j], STEPPER_CfgParam[i].IN_PIN[j], UNIPOLAR_WD_PATTERN[gs_STEPPER_info[i].Step_Index][j]);
			}
		}
		else if(STEPPER_CfgParam[i].STEPPING_Mode == FULL_STEP_DRIVE)
		{
			for (uint8_t j = 0; j < 4; j++)
			{
				HAL_GPIO_WritePin(STEPPER_CfgParam[i].IN_GPIO[j], STEPPER_CfgParam[i].IN_PIN[j], UNIPOLAR_FS_PATTERN[gs_STEPPER_info[i].Step_Index][j]);
			}
		}
		else if(STEPPER_CfgParam[i].STEPPING_Mode == HALF_STEP_DRIVE)
		{
			for (uint8_t j = 0; j < 4; j++)
			{
				HAL_GPIO_WritePin(STEPPER_CfgParam[i].IN_GPIO[j], STEPPER_CfgParam[i].IN_PIN[j], UNIPOLAR_HS_PATTERN[gs_STEPPER_info[i].Step_Index][j]);
			}
		}
	}
	// For BiPolar Stepper Motors
	else if(STEPPER_CfgParam[i].STEPPER_Cfg == STEPPER_BIPOLAR)
	{
		if(STEPPER_CfgParam[i].STEPPING_Mode == HALF_STEP_DRIVE)
		{
			for (uint8_t j = 0; j < 4; j++)
			{
				HAL_GPIO_WritePin(STEPPER_CfgParam[i].IN_GPIO[j], STEPPER_CfgParam[i].IN_PIN[j], POLAR_FZ_PATTERN[gs_STEPPER_info[i].Step_Index][j]);
			}
		}
	}
	// Update & Check The Index
	if(gs_STEPPER_info[i].Dir == DIR_CCW)
	{
		if(gs_STEPPER_info[i].Step_Index == 0)
		{
			gs_STEPPER_info[i].Step_Index = gs_STEPPER_info[i].Max_Index;
		}
		gs_STEPPER_info[i].Step_Index--;
	}
	else if(gs_STEPPER_info[i].Dir == DIR_CW)
	{
		gs_STEPPER_info[i].Step_Index++;
		if(gs_STEPPER_info[i].Step_Index == gs_STEPPER_info[i].Max_Index)
		{
			gs_STEPPER_info[i].Step_Index = 0;
		}
	}
}

void STEPPER_Step_Blocking(uint8_t au8_STEPPER_Instance, uint32_t au32_Steps, uint8_t au8_DIR)
{
	uint32_t i = 0;
	uint32_t DelayTimeMs = 0;

	gs_STEPPER_info[au8_STEPPER_Instance].Blocked = 1;
	DelayTimeMs = (60000/(gs_STEPPER_info[au8_STEPPER_Instance].RPM * STEPPER_CfgParam[au8_STEPPER_Instance].STEPS_PER_REV));
	// Send The Control Signals
	for(i=0; i<au32_Steps; i++)
	{
		STEPPER_One_Step(au8_STEPPER_Instance);
		DWT_Delay_ms(DelayTimeMs);
	}
	gs_STEPPER_info[au8_STEPPER_Instance].Blocked = 0;
}



void STEPPER_Step_NonBlocking(uint8_t au8_STEPPER_Instance, uint32_t au32_Steps, uint8_t au8_DIR)
{
	if (au32_Steps == 0) return;
	if(gs_STEPPER_info[au8_STEPPER_Instance].self_check==0)//没有自检
	{
		int32_t cur = gs_STEPPER_info[au8_STEPPER_Instance].Motor_pos;
		int32_t min_pos = stepper_min_pos(au8_STEPPER_Instance);
		int32_t max_pos = stepper_max_pos(au8_STEPPER_Instance);
		/* ---------- 已被锁定，判断方向 ---------- */
		if (gs_STEPPER_info[au8_STEPPER_Instance].Blocked)
		{
			if (gs_STEPPER_info[au8_STEPPER_Instance].Blocked_Dir == au8_DIR)
			{
				/* 同方向禁止动作 */
				return;
			}
			else
			{
				/* 反向：解除锁定 */
				gs_STEPPER_info[au8_STEPPER_Instance].Blocked = 0;
			}
		}

		/* ---------- 根据方向计算最大可走步数 ---------- */
		uint32_t allowed = 0;

		if (au8_DIR == DIR_CW)
		{
			if (cur >= max_pos) return;
			allowed = (uint32_t)(max_pos - cur);
		}
		else
		{
			if (cur <= min_pos) return;
			allowed = (uint32_t)(cur - min_pos);
		}

		/* ---------- 越过上限 → 裁剪 ---------- */
		if (au32_Steps > allowed) au32_Steps = allowed;
		if (au32_Steps == 0) return;
	}
	gs_STEPPER_info[au8_STEPPER_Instance].Steps += au32_Steps;
	gs_STEPPER_info[au8_STEPPER_Instance].Dir = au8_DIR;
}

void STEPPER_Stop(uint8_t au8_STEPPER_Instance)
{
	gs_STEPPER_info[au8_STEPPER_Instance].Steps = 0;
}

void STEPPER_Main(void)
{
	uint8_t i = 0;

	for(i=0; i<STEPPER_UNITS; i++)
	{
		if((gs_STEPPER_info[i].Ticks >= gs_STEPPER_info[i].Max_Ticks) && (gs_STEPPER_info[i].Blocked != 1) && (gs_STEPPER_info[i].Steps > 0))
		{
			STEPPER_One_Step(i);
			gs_STEPPER_info[i].Steps--;
			gs_STEPPER_info[i].Ticks = 0;
		}
		else
		{
			gs_STEPPER_info[i].Ticks++;
		}
	}
}

void STEPPER_TMR_OVF_ISR(TIM_HandleTypeDef* htim)
{
	uint8_t i = 0;
	if(htim->Instance == STEPPER_TIMER)
	{
		for(i=0; i<STEPPER_UNITS; i++)
		{
			if((gs_STEPPER_info[i].Ticks >= gs_STEPPER_info[i].Max_Ticks) && (gs_STEPPER_info[i].Blocked != 1) && (gs_STEPPER_info[i].Steps > 0))
			{
				if(gs_STEPPER_info[i].self_check==0)//没有自检
				{
					int32_t min_pos = stepper_min_pos(i);
					int32_t max_pos = stepper_max_pos(i);
					/* ---------- 预测下一步 ---------- */
					int32_t next_pos = gs_STEPPER_info[i].Motor_pos + ((gs_STEPPER_info[i].Dir == DIR_CW) ? 1 : -1);
					/* ---------- 执行前软限位检查 ---------- */
					if (next_pos > max_pos || next_pos < min_pos)
					{
						/* 越界 → 锁定方向 */
						gs_STEPPER_info[i].Steps = 0;
						gs_STEPPER_info[i].Blocked = 1;
						gs_STEPPER_info[i].Blocked_Dir = gs_STEPPER_info[i].Dir;

						/* 关闭线圈 */
						for (uint8_t j = 0; j < 4; j++)
						{
							HAL_GPIO_WritePin(STEPPER_CfgParam[i].IN_GPIO[j], STEPPER_CfgParam[i].IN_PIN[j], GPIO_PIN_RESET);
						}
						continue;
					}
					gs_STEPPER_info[i].Motor_pos = next_pos;
				}
				STEPPER_One_Step(i);
				/* ---------- 更新位置与剩余步数 ---------- */
				gs_STEPPER_info[i].Steps--;
				gs_STEPPER_info[i].Ticks = 0;

			}
			else
			{
				gs_STEPPER_info[i].Ticks++;
				if(gs_STEPPER_info[i].Steps == 0)//将电机输出都控制为高，避免电机发热烧坏
				{
					for (uint8_t j = 0; j < 4; j++)
					{
						HAL_GPIO_WritePin(STEPPER_CfgParam[i].IN_GPIO[j], STEPPER_CfgParam[i].IN_PIN[j], GPIO_PIN_RESET);
					}
				}
			}
		}
	}
}

// updated
void STEPPER_GET_STEP(uint8_t au8_STEPPER_Instance, uint16_t* returned_Steps)
{
	*returned_Steps = (uint16_t)gs_STEPPER_info[au8_STEPPER_Instance].Steps;
}

void STEPPER_GET_STEP_INDEX(uint8_t au8_STEPPER_Instance, uint8_t* returned_step_index)
{
	*returned_step_index = gs_STEPPER_info[au8_STEPPER_Instance].Step_Index;
}

void STEPPER_GET_DIR(uint8_t au8_STEPPER_Instance, uint8_t* returned_direction)
{
	*returned_direction = gs_STEPPER_info[au8_STEPPER_Instance].Dir;
}

void STEPPER_AUTO_PTZ_CENTER()
{
	uint8_t i = 0;
	/* ------------ 水平电机（MOTOR1） ------------ */
	gs_STEPPER_info[STEPPER_MOTOR1].self_check = 1;
    // 设置一个安全速度（不要太快）
    STEPPER_SetSpeed(STEPPER_MOTOR1, 0x20);

    // 先 CW 半圈
    STEPPER_Step_NonBlocking(STEPPER_MOTOR1, REAL_PAN_STEPS/2, DIR_CW);
    while(gs_STEPPER_info[STEPPER_MOTOR1].Steps);
    DWT_Delay_ms(500);
    // 再 CCW 一圈
    STEPPER_Step_NonBlocking(STEPPER_MOTOR1, MAX_ZOOM_STEPS, DIR_CCW);
    while(gs_STEPPER_info[STEPPER_MOTOR1].Steps);
    DWT_Delay_ms(500);

    // 再 CW 半圈 → 回到中心
    STEPPER_Step_NonBlocking(STEPPER_MOTOR1, REAL_PAN_STEPS/2, DIR_CW);
    while(gs_STEPPER_info[STEPPER_MOTOR1].Steps);
    DWT_Delay_ms(500);


    /* ------------ 垂直电机（MOTOR2） ------------ */
    gs_STEPPER_info[STEPPER_MOTOR2].self_check=1;
    STEPPER_SetSpeed(STEPPER_MOTOR2, 0x20);
    // 先 CW 半圈
    STEPPER_Step_NonBlocking(STEPPER_MOTOR2, REAL_TILT_STEPS/2, DIR_CW);
    while(gs_STEPPER_info[STEPPER_MOTOR2].Steps);
    DWT_Delay_ms(500);
    // 再 CCW 一圈
    STEPPER_Step_NonBlocking(STEPPER_MOTOR2, MAX_TILT_STEPS, DIR_CCW);
    while(gs_STEPPER_info[STEPPER_MOTOR2].Steps);
    DWT_Delay_ms(500);

    // 再 CCW 回到中间（你的机械垂直范围是 90°）
    STEPPER_Step_NonBlocking(STEPPER_MOTOR2, REAL_TILT_STEPS/2, DIR_CW);
    while(gs_STEPPER_info[STEPPER_MOTOR2].Steps);
    DWT_Delay_ms(500);
    for(i=0; i<STEPPER_UNITS; i++)
    {
    	//自检结束后最好手动 Reset 步进电机位置计数
    	gs_STEPPER_info[i].Step_Index = 0;
    	gs_STEPPER_info[i].Steps = 0;
    	gs_STEPPER_info[i].Motor_pos = 0;
    	gs_STEPPER_info[i].self_check = 0;
   }
}

void STEPPER_AUTO_FZ_CENTER()
{
    uint8_t i = 0;
    /* ------------ Zoom 电机（MOTOR3） ------------ */
    gs_STEPPER_info[STEPPER_MOTOR3].self_check = 1;
    STEPPER_SetSpeed(STEPPER_MOTOR3, 0x20);
    // 撞向 Wide 端限位 (CCW)
    STEPPER_Step_NonBlocking(STEPPER_MOTOR3, MAX_ZOOM_STEPS, DIR_CCW);
    while(gs_STEPPER_info[STEPPER_MOTOR3].Steps);
    DWT_Delay_ms(500);
    // 此时位置定义为 0
    gs_STEPPER_info[STEPPER_MOTOR3].Motor_pos = 0;
    gs_STEPPER_info[STEPPER_MOTOR3].self_check = 0;
    gs_STEPPER_info[STEPPER_MOTOR3].Blocked = 0;

    /* ------------ Focus 电机（MOTOR4） ------------ */
    gs_STEPPER_info[STEPPER_MOTOR4].self_check = 1;
    STEPPER_SetSpeed(STEPPER_MOTOR4, 0x20);
    // 撞向 Near 端限位 (CCW)
    STEPPER_Step_NonBlocking(STEPPER_MOTOR4, MAX_FOCUS_STEPS, DIR_CCW);
    while(gs_STEPPER_info[STEPPER_MOTOR4].Steps);
    DWT_Delay_ms(500);
    // 此时位置定义为 0
    gs_STEPPER_info[STEPPER_MOTOR4].Motor_pos = 0;
    gs_STEPPER_info[STEPPER_MOTOR4].self_check = 0;
    gs_STEPPER_info[STEPPER_MOTOR4].Blocked = 0;

    // 移动到步数表起始位置 (INF 对应的 429 步)
    STEPPER_Step_NonBlocking(STEPPER_MOTOR4, 429, DIR_CW);
    while(gs_STEPPER_info[STEPPER_MOTOR4].Steps);
}

/*//调用预置点
void STEPPER_GOTO_PRESET(uint8_t preset_id)
{
//读取预置点之后，根据预置点的位置，用gs_STEPPER_info[i].Motor_posl来和预置点对比，预置点有±
}
//设置预置点
void STEPPER_SET_PRESET(uint8_t preset_id,uint16_t pan_pos, uint16_t tilt_pos)
{
	//把当前的编号，水平、垂直位置保存到flash里面，
}
//清除预置点
void STEPPER_CLEAR_PRESET(uint8_t preset_id)
{

}*/

int32_t STEPPER_GetMotorPos(uint8_t motor)
{
    if (motor >= STEPPER_UNITS) return 0;
    return gs_STEPPER_info[motor].Motor_pos;
}

void STEPPER_SetMotorPos(uint8_t motor, int32_t pos)
{
    if (motor >= STEPPER_UNITS) return;
    gs_STEPPER_info[motor].Motor_pos = pos;
}





