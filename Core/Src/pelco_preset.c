#include "pelco_preset.h"
#include "stm32f1xx_hal.h"
#include "STEPPER.h"
#include "STEPPER_cfg.h"
#include "main.h"
#include <string.h>

static STEPPER_PRESET_t preset_table[MAX_PRESETS];

/* ============================
   Flash 操作函数
   ============================ */

static void FLASH_WritePresetTable(void)
{
    HAL_FLASH_Unlock();

    /* 擦除 1 KB 页 */
    FLASH_EraseInitTypeDef erase;
    uint32_t PageError;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = FLASH_PRESET_ADDRESS;
    erase.NbPages = 1;

    HAL_FLASHEx_Erase(&erase, &PageError);

    /* 写入 Magic */
    uint32_t magic = PRESET_MAGIC;
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_PRESET_ADDRESS, magic);

    /* 写入预置表 */
    uint32_t addr = FLASH_PRESET_ADDRESS + 4;

    for (int i = 0; i < MAX_PRESETS; i++)
    {
        uint32_t v1,v2;

        v1 = (preset_table[i].pan_pos) << 16 | (preset_table[i].tilt_pos) ;

        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, v1);
        addr += 4;

        v2 = ((uint8_t)(preset_table[i].used));
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, v2);
        addr += 4;
    }

    HAL_FLASH_Lock();
}

static void FLASH_LoadPresetTable(void)
{
    uint32_t magic = *(uint32_t*)FLASH_PRESET_ADDRESS;
    if (magic != PRESET_MAGIC)
    {
        memset(preset_table, 0, sizeof(preset_table));
        return;
    }

    uint32_t addr = FLASH_PRESET_ADDRESS + 4;

    for (int i = 0; i < MAX_PRESETS; i++)
    {
        uint32_t v1 = *(uint32_t*)(addr);
        addr += 4;

        uint32_t v2 = *(uint32_t*)(addr);
        addr += 4;

        preset_table[i].pan_pos   = (v1 >> 16) & 0xFFFF;
        preset_table[i].tilt_pos  = v1   & 0xFFFF;
        preset_table[i].used      = (uint8_t)(v2 & 0xFF);
//        preset_table[i].zoom_pos  = (v1)       & 0xFF;
//       preset_table[i].focus_pos = (v2)       & 0xFF;
    }
}

/* ================= 初始化 ================= */

void STEPPER_PRESET_Init(void)
{
    FLASH_LoadPresetTable();
}

/* ================= 设置预置点 ================= */

void STEPPER_SET_PRESET(uint8_t preset_id)
{
    if (preset_id >= MAX_PRESETS) return;
    preset_table[preset_id].pan_pos  = STEPPER_GetMotorPos(STEPPER_MOTOR1);
    preset_table[preset_id].tilt_pos = STEPPER_GetMotorPos(STEPPER_MOTOR2);
    preset_table[preset_id].used = 1;

    FLASH_WritePresetTable();
}

/* ================= 清除预置点 ================= */

void STEPPER_CLEAR_PRESET(uint8_t preset_id)
{
    if (preset_id >= MAX_PRESETS) return;

    memset(&preset_table[preset_id], 0, sizeof(STEPPER_PRESET_t));
    FLASH_WritePresetTable();
}

/* ================= 调用预置点 ================= */

void STEPPER_GOTO_PRESET(uint8_t preset_id)
{
    if (preset_id >= MAX_PRESETS) return;
    if (!preset_table[preset_id].used) return;

    int32_t cur_pan  = STEPPER_GetMotorPos(STEPPER_MOTOR1);
    int32_t cur_tilt = STEPPER_GetMotorPos(STEPPER_MOTOR2);

    int32_t delta_pan  = preset_table[preset_id].pan_pos  - cur_pan;
    int32_t delta_tilt = preset_table[preset_id].tilt_pos - cur_tilt;

    if (delta_pan != 0)
    {
        STEPPER_Step_NonBlocking(STEPPER_MOTOR1,(delta_pan > 0) ? delta_pan : -delta_pan,(delta_pan > 0) ? DIR_CW : DIR_CCW);
    }

    if (delta_tilt != 0)
    {
        STEPPER_Step_NonBlocking(STEPPER_MOTOR2,(delta_tilt > 0) ? delta_tilt : -delta_tilt,(delta_tilt > 0) ? DIR_CW : DIR_CCW);
    }
}

void STEPPER_GOTO_POS(int32_t x_pan,int32_t y_tilt)
{
    int32_t cur_pan  = STEPPER_GetMotorPos(STEPPER_MOTOR1);
    int32_t cur_tilt = STEPPER_GetMotorPos(STEPPER_MOTOR2);

    int32_t delta_pan  = x_pan  - cur_pan;
    int32_t delta_tilt = y_tilt - cur_tilt;

    if (delta_pan != 0)
    {
        STEPPER_Step_NonBlocking(STEPPER_MOTOR1,(delta_pan > 0) ? delta_pan : -delta_pan,(delta_pan > 0) ? DIR_CW : DIR_CCW);
    }

    if (delta_tilt != 0)
    {
        STEPPER_Step_NonBlocking(STEPPER_MOTOR2,(delta_tilt > 0) ? delta_tilt : -delta_tilt,(delta_tilt > 0) ? DIR_CW : DIR_CCW);
    }
}
