#include "lens_control.h"
#include "STEPPER.h"
#include <math.h>

Lens_State_t g_lens = {0, 0, 0, 1, 0};

/* 采样后的对焦曲线数据 (INF 距离) */
const uint16_t zoom_table[] = {
0, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400, 420, 440, 460, 480, 500, 520, 540, 560, 580, 600, 620, 640, 660, 680, 700, 720, 740, 760, 780, 800, 820, 840, 860, 880, 900, 920, 940, 960, 980, 1000, 1020, 1040, 1060, 1080, 1100, 1120, 1140, 1160, 1180, 1201, 1221, 1241, 1261, 1281, 1301, 1321, 1341, 1361, 1380, 1400, 1420, 1440, 1460, 1480, 1500, 1520, 1540, 1560, 1580, 1600, 1621, 1641, 1661, 1681, 1701, 1721, 1741, 1761, 1781, 1801, 1821, 1841, 1861, 1881, 1901, 1921, 1941, 1961, 1981, 2001, 2021, 2041, 2061, 2081, 2101, 2121, 2141, 2161, 2181, 2201, 2221, 2241, 2261, 2281, 2301, 2321, 2341, 2342
};

const uint16_t focus_table_inf[] = {
429, 530, 627, 719, 808, 893, 974, 1052, 1127, 1199, 1268, 1334, 1397, 1458, 1517, 1573, 1628, 1680, 1730, 1778, 1825, 1870, 1913, 1954, 1994, 2032, 2070, 2105, 2140, 2173, 2205, 2235, 2265, 2294, 2321, 2347, 2373, 2397, 2421, 2444, 2466, 2487, 2507, 2526, 2545, 2563, 2580, 2597, 2613, 2628, 2643, 2657, 2671, 2683, 2696, 2708, 2719, 2730, 2740, 2750, 2759, 2768, 2777, 2785, 2792, 2799, 2806, 2813, 2819, 2824, 2830, 2835, 2839, 2843, 2847, 2851, 2854, 2857, 2860, 2862, 2864, 2866, 2868, 2869, 2870, 2871, 2872, 2872, 2872, 2872, 2872, 2871, 2870, 2869, 2868, 2866, 2865, 2863, 2861, 2858, 2856, 2853, 2851, 2848, 2844, 2841, 2838, 2834, 2830, 2826, 2822, 2818, 2814, 2809, 2804, 2799, 2795, 2790, 3011
};

#define TABLE_SIZE (sizeof(zoom_table)/sizeof(zoom_table[0]))

void Lens_Init(void) {
    g_lens.zoom_pos = STEPPER_GetMotorPos(STEPPER_MOTOR3);
    g_lens.focus_pos = STEPPER_GetMotorPos(STEPPER_MOTOR4);
    g_lens.af_enabled = 1;
}

/* 线性插值计算目标 Focus 位置 */
int32_t Lens_CalculateFocus(int32_t zoom_pos) {
    if (zoom_pos <= zoom_table[0]) return focus_table_inf[0];
    if (zoom_pos >= zoom_table[TABLE_SIZE-1]) return focus_table_inf[TABLE_SIZE-1];

    for (int i = 0; i < TABLE_SIZE - 1; i++) {
        if (zoom_pos >= zoom_table[i] && zoom_pos <= zoom_table[i+1]) {
            float ratio = (float)(zoom_pos - zoom_table[i]) / (zoom_table[i+1] - zoom_table[i]);
            return (int32_t)(focus_table_inf[i] + ratio * (focus_table_inf[i+1] - focus_table_inf[i]));
        }
    }
    return focus_table_inf[0];
}

void Lens_SyncFocus(void) {
    int32_t current_zoom = STEPPER_GetMotorPos(STEPPER_MOTOR3);
    int32_t target_focus = Lens_CalculateFocus(current_zoom);
    int32_t current_focus = STEPPER_GetMotorPos(STEPPER_MOTOR4);
    
    int32_t diff = target_focus - current_focus;
    if (diff) {
        uint8_t dir = (diff > 0) ? DIR_CW : DIR_CCW;
        STEPPER_SetSpeed(STEPPER_MOTOR4, 0x3F); // 同步时使用最高速
        STEPPER_Step_NonBlocking(STEPPER_MOTOR4, abs(diff), dir);
    }
}

void Lens_ZoomMove(uint8_t dir, uint8_t speed) {
    STEPPER_SetSpeed(STEPPER_MOTOR3, speed);
    STEPPER_Step_NonBlocking(STEPPER_MOTOR3, 10000, dir); // 10000步作为持续移动
    g_lens.is_moving = 1;
}

void Lens_FocusMove(uint8_t dir, uint8_t speed) {
    g_lens.af_enabled = 0; // 手动调节时关闭自动同步
    STEPPER_SetSpeed(STEPPER_MOTOR4, speed);
    STEPPER_Step_NonBlocking(STEPPER_MOTOR4, 10000, dir);
}

void Lens_Stop(void) {
    STEPPER_Stop(STEPPER_MOTOR3);
    STEPPER_Stop(STEPPER_MOTOR4);
    g_lens.is_moving = 0;
    g_lens.af_enabled = 1; // 停止后恢复自动同步逻辑
}

void Lens_Process(void) {
    if (g_lens.is_moving && g_lens.af_enabled) {
        Lens_SyncFocus();
    }
}

void Lens_HandleAFValue(uint16_t af_value) {
    g_lens.last_af_value = af_value;
    // 这里可以根据 AF 值进行微调，目前先记录
}
