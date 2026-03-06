#include "lens_control.h"
#include "STEPPER.h"
#include <math.h>
#include <stdlib.h>
#define TABLE_SIZE (sizeof(zoom_table)/sizeof(zoom_table[0]))
#define LENS_CONTINUOUS_STEPS     10000U
#define LENS_SYNC_DEADBAND_STEPS  4
#define LENS_AF_MIN_STEP          2
#define LENS_AF_START_STEP        24
#define LENS_AF_SETTLE_MS         45U
#define LENS_AF_IMPROVE_MARGIN    3U
#define LENS_AF_IDLE_TIMEOUT_MS   300U

typedef struct {
    uint8_t active;
    uint8_t direction;
    uint8_t reverse_used;
    uint8_t wait_sample;
    uint16_t step_size;
    uint16_t best_score;
    uint32_t action_deadline_ms;
} Lens_AF_State_t;

Lens_State_t g_lens = {0, 0, 0, 1, 0};
static Lens_AF_State_t g_af_state = {0};

/* 采样后的对焦曲线数据 (INF 距离) */
/* 采样后的对焦曲线数据 (INF 距离) */
const uint16_t zoom_table[] = {
0, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400, 420, 440, 460, 480, 500, 520, 540, 560, 580, 600, 620, 640, 660, 680, 700, 720, 740, 760, 780, 800, 820, 840, 860, 880, 900, 920, 940, 960, 980, 1000, 1020, 1040, 1060, 1080, 1100, 1120, 1140, 1160, 1180, 1201, 1221, 1241, 1261, 1281, 1301, 1321, 1341, 1361, 1380, 1400, 1420, 1440, 1460, 1480, 1500, 1520, 1540, 1560, 1580, 1600, 1621, 1641, 1661, 1681, 1701, 1721, 1741, 1761, 1781, 1801, 1821, 1841, 1861, 1881, 1901, 1921, 1941, 1961, 1981, 2001, 2021, 2041, 2061, 2081, 2101, 2121, 2141, 2161, 2181, 2201, 2221, 2241, 2261, 2281, 2301, 2321, 2341, 2342
};
const uint16_t focus_table_inf[] = {
429, 530, 627, 719, 808, 893, 974, 1052, 1127, 1199, 1268, 1334, 1397, 1458, 1517, 1573, 1628, 1680, 1730, 1778, 1825, 1870, 1913, 1954, 1994, 2032, 2070, 2105, 2140, 2173, 2205, 2235, 2265, 2294, 2321, 2347, 2373, 2397, 2421, 2444, 2466, 2487, 2507, 2526, 2545, 2563, 2580, 2597, 2613, 2628, 2643, 2657, 2671, 2683, 2696, 2708, 2719, 2730, 2740, 2750, 2759, 2768, 2777, 2785, 2792, 2799, 2806, 2813, 2819, 2824, 2830, 2835, 2839, 2843, 2847, 2851, 2854, 2857, 2860, 2862, 2864, 2866, 2868, 2869, 2870, 2871, 2872, 2872, 2872, 2872, 2871, 2870, 2869, 2868, 2866, 2865, 2863, 2861, 2858, 2856, 2853, 2851, 2848, 2844, 2841, 2838, 2834, 2830, 2826, 2822, 2818, 2814, 2809, 2804, 2799, 2795, 2790, 3011};

static void Lens_AFReset(void)
{
    g_af_state.active = 0;
    g_af_state.wait_sample = 0;
    g_af_state.reverse_used = 0;
    g_af_state.step_size = LENS_AF_START_STEP;
    g_af_state.best_score = g_lens.last_af_value;
}

static void Lens_AFStep(uint8_t dir)
{
    STEPPER_SetSpeed(STEPPER_MOTOR4, 0x18);
    STEPPER_Step_NonBlocking(STEPPER_MOTOR4, g_af_state.step_size, dir);
    g_af_state.direction = dir;
    g_af_state.wait_sample = 1;
    g_af_state.action_deadline_ms = HAL_GetTick() + LENS_AF_SETTLE_MS;
}

void Lens_Init(void) {
    g_lens.zoom_pos = STEPPER_GetMotorPos(STEPPER_MOTOR3);
    g_lens.focus_pos = STEPPER_GetMotorPos(STEPPER_MOTOR4);
    g_lens.af_enabled = 1;
    g_lens.is_moving = 0;
    g_lens.af_value_fresh = 0;
    g_lens.last_af_rx_ms = HAL_GetTick();

    Lens_AFReset();
    Lens_SyncFocus();
}

/* 线性插值计算目标 Focus 位置 */
int32_t Lens_CalculateFocus(int32_t zoom_pos) {
    if (zoom_pos <= zoom_table[0]) return focus_table_inf[0];
    if (zoom_pos >= zoom_table[TABLE_SIZE-1]) return focus_table_inf[TABLE_SIZE-1];

    for (uint16_t i = 0; i < TABLE_SIZE - 1; i++)  {
        if (zoom_pos >= zoom_table[i] && zoom_pos <= zoom_table[i+1]) {
            float ratio = (float)(zoom_pos - zoom_table[i]) / (float)(zoom_table[i+1] - zoom_table[i]);
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
    if (labs(diff) <= LENS_SYNC_DEADBAND_STEPS) {
        return;
    }

    uint8_t dir = (diff > 0) ? DIR_CW : DIR_CCW;
    uint32_t steps = (uint32_t)abs(diff);

    STEPPER_SetSpeed(STEPPER_MOTOR4, 0x22);
    STEPPER_Step_NonBlocking(STEPPER_MOTOR4, steps, dir);
}

void Lens_ZoomMove(uint8_t dir, uint8_t speed) {
    g_lens.af_enabled = 1;
    Lens_AFReset();
    STEPPER_SetSpeed(STEPPER_MOTOR3, speed);
    STEPPER_Step_NonBlocking(STEPPER_MOTOR3, LENS_CONTINUOUS_STEPS, dir);
    g_lens.is_moving = 1;
    Lens_SyncFocus();
}

void Lens_FocusMove(uint8_t dir, uint8_t speed) {
    g_lens.af_enabled = 0; // 手动调节时关闭自动同步/自动对焦
    Lens_AFReset();
    STEPPER_SetSpeed(STEPPER_MOTOR4, speed);
    STEPPER_Step_NonBlocking(STEPPER_MOTOR4, LENS_CONTINUOUS_STEPS, dir);
}

void Lens_Stop(void) {
    STEPPER_Stop(STEPPER_MOTOR3);
    STEPPER_Stop(STEPPER_MOTOR4);
    g_lens.is_moving = 0;
    g_lens.af_enabled = 1;
    Lens_AFReset();
    Lens_SyncFocus();
}

static void Lens_AutoFocusProcess(void)
{
    uint32_t now = HAL_GetTick();

    if (!g_lens.af_enabled) return;
    if ((now - g_lens.last_af_rx_ms) > LENS_AF_IDLE_TIMEOUT_MS) return;

    if (!g_af_state.active) {
        g_af_state.active = 1;
        g_af_state.best_score = g_lens.last_af_value;
        g_af_state.step_size = LENS_AF_START_STEP;
        g_af_state.direction = DIR_CW;
        g_af_state.reverse_used = 0;
        Lens_AFStep(g_af_state.direction);
        return;
    }

    if (g_af_state.wait_sample && (int32_t)(now - g_af_state.action_deadline_ms) >= 0) {
        g_af_state.wait_sample = 0;

        if (!g_lens.af_value_fresh) {
            return;
        }

        g_lens.af_value_fresh = 0;

        if (g_lens.last_af_value > (uint16_t)(g_af_state.best_score + LENS_AF_IMPROVE_MARGIN)) {
            g_af_state.best_score = g_lens.last_af_value;
            Lens_AFStep(g_af_state.direction);
            return;
        }

        if (!g_af_state.reverse_used) {
            g_af_state.reverse_used = 1;
            g_af_state.direction = (g_af_state.direction == DIR_CW) ? DIR_CCW : DIR_CW;
            g_af_state.step_size /= 2;
            if (g_af_state.step_size < LENS_AF_MIN_STEP) g_af_state.step_size = LENS_AF_MIN_STEP;
            Lens_AFStep(g_af_state.direction);
            return;
        }

        Lens_AFReset();
    }
}

void Lens_Process(void) {
    int32_t zoom_now = STEPPER_GetMotorPos(STEPPER_MOTOR3);
    int32_t focus_now = STEPPER_GetMotorPos(STEPPER_MOTOR4);

    g_lens.is_moving = (zoom_now != g_lens.zoom_pos);
    
    if (g_lens.is_moving && g_lens.af_enabled) {
        Lens_SyncFocus();
    }
        if (!g_lens.is_moving) {
        Lens_AutoFocusProcess();
    }
    g_lens.zoom_pos = zoom_now;
    g_lens.focus_pos = focus_now;
}

void Lens_HandleAFValue(uint16_t af_value) {
    g_lens.last_af_value = af_value;
    // 这里可以根据 AF 值进行微调，目前先记录
    g_lens.af_value_fresh = 1;
    g_lens.last_af_rx_ms = HAL_GetTick();
}
