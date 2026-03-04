/**
  ******************************************************************************
  * @file    lens_config.h
  * @brief   镜头驱动系统全局配置与常量定义
  ******************************************************************************
  */

#ifndef __LENS_CONFIG_H
#define __LENS_CONFIG_H

#include "stm32f1xx_hal.h"

/* --- 硬件接口配置 --- */
#define TMI_CS_PORT         GPIOB
#define TMI_CS_PIN          GPIO_PIN_0

/* --- 电机通道定义 --- */
#define CH_ZOOM             12      // 变倍电机对应 TMI8150B 通道 12
#define CH_FOCUS            34      // 聚焦电机对应 TMI8150B 通道 34

/* --- 镜头行程与限制 --- */
#define ZOOM_MAX_STEPS      2011    // 根据步数表定义的最大变倍步数
#define ZOOM_MIN_STEPS      0
#define FOCUS_MAX_STEPS     2000    // 聚焦电机预估最大行程
#define FOCUS_MIN_STEPS     0

/* --- 跟踪聚焦参数 --- */
#define TRACKING_MAP_SIZE   112     // 步数表条目数
#define SYNC_STEP_CHUNK     5       // 变倍时的同步步进块大小 (越小越平滑)

/* --- Pelco-D 协议配置 --- */
#define PELCO_D_ADDR        0x01    // 默认设备地址
#define PELCO_D_LEN         7       // 标准数据包长度
#define AFD_EXT_LEN         7       // AFD 扩展指令长度

/* --- 电机方向定义 --- */
#define DIR_CW              0       // 顺时针
#define DIR_CCW             1       // 逆时针

/*0 - 白天模式 (切入白片/彩色), 1 - 夜间模式 (切入黑片/黑白)*/
#define DAY_IR              0       // 白天模式
#define NIGHT_IR            1      // 夜间模式

#define ZOOM_TELE           DIR_CW
#define ZOOM_WIDE           DIR_CCW
#define FOCUS_FAR           DIR_CW
#define FOCUS_NEAR          DIR_CCW

/* --- 数据结构定义 --- */

/**
 * @brief 镜头电机状态结构体
 */
typedef struct {
    int32_t current_pos;    // 当前步数位置
    int32_t target_pos;     // 目标步数位置
    uint16_t max_pos;       // 最大步数限制
    uint16_t min_pos;       // 最小步数限制
    uint8_t speed_level;    // 速度等级 (Pelco-D 0-63)
    uint8_t is_moving;      // 移动状态标志
    uint8_t tmi_channel;    // 对应的 TMI8150B 通道 (12 或 34)
} MotorState_t;

/**
 * @brief 跟踪聚焦步数表结构体
 */
typedef struct {
    uint16_t zoom_step;     // 变倍步数
    uint16_t focus_step;    // 对应的聚焦步数
} TrackingMap_t;

/**
 * @brief 系统全局状态结构体
 */
typedef struct {
    MotorState_t zoom_motor;
    MotorState_t focus_motor;
    uint16_t afd_value;     // 接收到的 AFD 聚焦值
    uint8_t is_homing;      // 是否正在自检
    uint8_t af_enabled;     // 自动聚焦使能
} LensSystem_t;

#endif /* __LENS_CONFIG_H */
