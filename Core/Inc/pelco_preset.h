#ifndef PELCO_PRESET_H_
#define PELCO_PRESET_H_

#include <stdint.h>

#define MAX_PRESETS   64     // Flash 限制
#define PRESET_MAGIC  0x12345678

#define FLASH_PRESET_ADDRESS  0x0800FC00   // STM32F103C8 最后一页

typedef struct {
    int16_t  pan_pos;
    int16_t  tilt_pos;
    uint8_t  used;       // 是否已保存
//    int32_t  zoom_pos;
//    int32_t  focus_pos;
} STEPPER_PRESET_t;

void STEPPER_PRESET_Init(void);
void STEPPER_SET_PRESET(uint8_t preset_id);
void STEPPER_CLEAR_PRESET(uint8_t preset_id);
void STEPPER_GOTO_PRESET(uint8_t preset_id);
void STEPPER_GOTO_POS(int32_t x_pan,int32_t y_tilt);

#endif
