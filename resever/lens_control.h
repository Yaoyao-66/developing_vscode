/**
  ******************************************************************************
  * @file    lens_control.h
  * @brief   镜头变倍变焦控制逻辑头文件
  ******************************************************************************
  */

#ifndef __LENS_CONTROL_H
#define __LENS_CONTROL_H

#include "lens_config.h"

/* 全局系统状态变量声明 */
extern LensSystem_t LensSystem;

/* 函数声明 */
void Lens_Init(void);
void Lens_Process(void);
void Lens_SelfCheck(void);

/* 控制接口 */
void Lens_ZoomMove(uint8_t direction, uint8_t speed);
void Lens_FocusMove(uint8_t direction, uint8_t speed);
void Lens_Stop(void);

/* 跟踪聚焦 */
void Lens_UpdateTracking(void);

#endif /* __LENS_CONTROL_H */
