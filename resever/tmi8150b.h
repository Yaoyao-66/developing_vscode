/**
  ******************************************************************************
  * @file    tmi8150b.h
  * @brief   TMI8150B 电机驱动 IC 驱动头文件
  ******************************************************************************
  */

#ifndef __TMI8150B_H
#define __TMI8150B_H

#include "lens_config.h"

/* TMI8150B 寄存器地址定义 */
#define TMI_REG_CHIP_ID0    0x00    // 芯片 ID 寄存器 0  0x81
#define TMI_REG_CHIP_ID1    0x01    // 芯片 ID 寄存器 1  0x50
#define TMI_REG_GCTRL       0x02    // 全局控制寄存器 (复位、使能、工作模式)
#define TMI_REG_GCCON       0x03    // 全局时钟控制 (分频配置)

/* 通道 12 寄存器地址 (Zoom 电机) */
#define TMI_REG_CH12_CN     0x04    // 通道 12 控制寄存器 (使能、自动/手动模式)
#define TMI_REG_CH12_MD0    0x05    // 通道 12 工作模式寄存器 0 (细分、方向)
#define TMI_REG_CH12_MD1    0x06    // 通道 12 工作模式寄存器 1 (时钟分频)
#define TMI_REG_CH12_PHASE  0x07    // 通道 12 当前相位 (只读)
#define TMI_REG_CH12_CYCNT0 0x08    // 通道 12 当前圈数低 8 位 (只读)
#define TMI_REG_CH12_CYCNT1 0x09    // 通道 12 当前圈数高 5 位 (只读)
#define TMI_REG_CH12_PHSET  0x0A    // 通道 12 相位设定 (目标相位)
#define TMI_REG_CH12_CYSET0 0x0B    // 通道 12 圈数设定低 8 位 (目标圈数)
#define TMI_REG_CH12_CYSET1 0x0C    // 通道 12 圈数设定高 5 位 (目标圈数)
#define TMI_REG_CH1_PULSE   0x0D    // 通道 1 PWM 宽度 (手动模式使用)
#define TMI_REG_CH2_PULSE   0x0E    // 通道 2 PWM 宽度 (手动模式使用)
#define TMI_REG_CH12_PHASEH 0x0F    // 通道 12 相位高位
#define TMI_REG_CH12_PULSEH 0x10    // 通道 12 PWM 宽度高位

#define TMI_REG_CHIR_CN     0x11    // 通道 IR 控制寄存器 (红外滤光片控制),通道 IR 控制寄存器，控制 IR 通道使能、转向、启停

/* 通道 34 寄存器地址 (Focus 电机) */
#define TMI_REG_CH34_CN     0x14    // 通道 34 控制寄存器 (使能、自动/手动模式)
#define TMI_REG_CH34_MD0    0x15    // 通道 34 工作模式寄存器 0 (细分、方向)
#define TMI_REG_CH34_MD1    0x16    // 通道 34 工作模式寄存器 1 (时钟分频)
#define TMI_REG_CH34_PHASE  0x17    // 通道 34 当前相位 (只读)
#define TMI_REG_CH34_CYCNT0 0x18    // 通道 34 当前圈数低 8 位 (只读)
#define TMI_REG_CH34_CYCNT1 0x19    // 通道 34 当前圈数高 5 位 (只读)
#define TMI_REG_CH34_PHSET  0x1A    // 通道 34 相位设定 (目标相位)
#define TMI_REG_CH34_CYSET0 0x1B    // 通道 34 圈数设定低 8 位 (目标圈数)
#define TMI_REG_CH34_CYSET1 0x1C    // 通道 34 圈数设定高 5 位 (目标圈数)
#define TMI_REG_CH3_PULSE   0x1D    // 通道 3 PWM 宽度 (手动模式使用)
#define TMI_REG_CH4_PULSE   0x1E    // 通道 4 PWM 宽度 (手动模式使用)
#define TMI_REG_CH34_PHASEH 0x12    // 通道 34 相位高位
#define TMI_REG_CH34_PULSEH 0x13    // 通道 34 PWM 宽度高位

#define TMI_REG_CH_PWMSET   0x1F    // 通道 12 和通道 34 PWM 更新模式寄存器


/* 常用位定义 */
#define TMI_GCTRL_GRESET    (1 << 7)	//全局复位/工作位
#define TMI_GCTRL_GENABLE   (1 << 3)	//驱动通道全局使能位

/* 函数声明 */
void TMI8150B_Init(void);
void TMI8150B_WriteReg(uint8_t addr, uint8_t data);
uint8_t TMI8150B_ReadReg(uint8_t addr);

//IR_CUT切换函数 0 - 白天模式 (切入白片/彩色), 1 - 夜间模式 (切入黑片/黑白)
void Lens_SetIRMode(uint8_t mode);
/* 电机控制函数 */
void TMI8150B_SetSteps(uint8_t channel_pair, int32_t steps, uint8_t direction);
void TMI8150B_Stop(uint8_t channel_pair);
uint16_t TMI8150B_GetSteps(uint8_t channel_pair);
int8_t TMI8150B_WaitMoveComplete(uint8_t channel_pair, uint32_t timeout_ms);

#endif /* __TMI8150B_H */





