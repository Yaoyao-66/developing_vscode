#ifndef __LENS_CONTROL_H
#define __LENS_CONTROL_H

#include "main.h"

/* 镜头控制状态 */
typedef struct {
    int32_t zoom_pos;
    int32_t focus_pos;
    uint8_t is_moving;
    uint8_t af_enabled;
    uint16_t last_af_value;
    uint8_t af_value_fresh;
    uint32_t last_af_rx_ms;
} Lens_State_t;

extern Lens_State_t g_lens;

/* 函数声明 */
void Lens_Init(void);
void Lens_Process(void);
void Lens_ZoomMove(uint8_t dir, uint8_t speed);
void Lens_FocusMove(uint8_t dir, uint8_t speed);
void Lens_Stop(void);
int32_t Lens_CalculateFocus(int32_t zoom_pos);
void Lens_SyncFocus(void);
void Lens_HandleAFValue(uint16_t af_value);

#endif
