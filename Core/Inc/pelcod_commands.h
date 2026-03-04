/*
 * pelcod_commands.h
 *
 *  Created on: 2025年12月5日
 *      Author: Administrator
 */
/***************************************************************************************
 * Pelco-D 协议（B 级指令集）指令定义
 * 作者：ChatGPT（根据 Pelco 官方指令表整理）
 *
 * 本文件包含：
 *  1. Pelco-D 帧格式定义
 *  2. CMD1 / CMD2 的标准位功能（镜头、方向控制）
 *  3. 标准扩展命令（预置点、辅助输出、复位）
 *  4. 校验计算宏
 *
 * B 级指令集说明：
 *  - 包含方向控制（Pan / Tilt）
 *  - 包含镜头控制（Zoom / Focus / Iris）
 *  - 包含预置点设置/调用/清除
 *  - 包含辅助开关 Aux ON/OFF
 *  - 包含摄像机或球机复位
 *  - 不包含 C 级的绝对位置、查询等高阶命令
 *
 ***************************************************************************************/

 #ifndef __PELCOD_COMMANDS_H__
 #define __PELCOD_COMMANDS_H__

 #include <stdint.h>

 /* ------------------------------------------------------------------------------------
  * Pelco-D 帧结构（共 7 字节）：
  *
  * BYTE1  BYTE2   BYTE3  BYTE4  BYTE5   BYTE6   BYTE7
  * 0xFF   地址     CMD1   CMD2   DATA1  DATA2   校验
  *
  * 校验 = (地址 + CMD1 + CMD2 + DATA1 + DATA2) 的低 8 位
  * ------------------------------------------------------------------------------------ */

 #define PELCOD_SYNC   0xFF    /* 帧头固定值 */

 /* ====================================================================================
  * CMD1 位定义（镜头控制类指令）
  * ==================================================================================== */

 /* CMD1 各位功能说明（符合 Pelco 官方标准） ，CMD1 —— Epiphan 文档中未使用 CMD1，因此全部置 0*/
 //#define PELCO_CMD1_ZOOM_TELE      0x00    /* 镜头拉近（变焦 Tele） */
 //#define PELCO_CMD1_ZOOM_WIDE      0x00    /* 镜头拉远（变焦 Wide） */
 //#define PELCO_CMD1_FOCUS_NEAR     0x00    /* 对焦 Near */
 //#define PELCO_CMD1_FOCUS_FAR      0x00    /* 对焦 Far */
 //#define PELCO_CMD1_IRIS_OPEN      0x00    /* 光圈打开 */
 //#define PELCO_CMD1_IRIS_CLOSE     0x00    /* 光圈关闭 */

 /* ====================================================================================
  * CMD2 位定义（方向控制类指令）
  * ==================================================================================== */

#define PELCO_CMD2_TILT_UP        0x08    /* 向上 */
#define PELCO_CMD2_TILT_DOWN      0x10    /* 向下 */
#define PELCO_CMD2_PAN_LEFT       0x04    /* 向左 */
#define PELCO_CMD2_PAN_RIGHT      0x02    /* 向右 */
/* ====================================================================================
 * CMD2 —— 对角组合（Epiphan 明确采用按位 OR）
 * ==================================================================================== */

#define PELCO_CMD2_LEFT_UP        (PELCO_CMD2_PAN_LEFT  | PELCO_CMD2_TILT_UP)     /* 0x0C */
#define PELCO_CMD2_LEFT_DOWN      (PELCO_CMD2_PAN_LEFT  | PELCO_CMD2_TILT_DOWN)   /* 0x14 */
#define PELCO_CMD2_RIGHT_UP       (PELCO_CMD2_PAN_RIGHT | PELCO_CMD2_TILT_UP)     /* 0x0A */
#define PELCO_CMD2_RIGHT_DOWN     (PELCO_CMD2_PAN_RIGHT | PELCO_CMD2_TILT_DOWN)   /* 0x12 */

#define PELCO_CMD2_PT_STOP     	0x00   /* 0x00 */

/* ====================================================================================
 * CMD1、CMD2 —— 镜头控制（Zoom / Focus）
 * ==================================================================================== */

#define PELCO_CMD2_ZOOM_TELE   0x20    /* Tele（拉近） */
#define PELCO_CMD2_ZOOM_WIDE   0x40    /* Wide（拉远） */
#define PELCO_CMD1_FOCUS_NEAR  0x01    /* Near */
#define PELCO_CMD2_FOCUS_FAR   0x80    /* Far */

#define PELCO_CMD1_IRIS_OPEN   0x04    /* 光圈大 */
#define PELCO_CMD1_IRIS_CLOSE  0x02    /* 光圈小 */

#define PELCO_CMD1_CMD2_FZ_STOP     	0x00   /* 0x00 */
 /* ====================================================================================
  * 扩展命令（CMD2 单字节命令）
  * ==================================================================================== */

/* ====================================================================================
 * CMD2 —— 预置位（Set / Clear / Call）
 * ==================================================================================== */

#define PELCO_CMD2_PRESET_SET      0x03    /* 设置预置位 */
#define PELCO_CMD2_PRESET_CLEAR    0x05    /* 清除预置位 */
#define PELCO_CMD2_PRESET_CALL     0x07    /* 调用预置位 */

/* ====================================================================================
 * CMD2 —— 辅助控制（Aux）
 * ==================================================================================== */

#define PELCO_CMD2_AUX_ON          0x09    /* 辅助灯光打开 */
#define PELCO_CMD2_AUX_OFF         0x0B    /* 辅助灯光关闭 */
/* ====================================================================================
 * CMD2 —— 自动扫描 & 停止
 * ==================================================================================== */
/* 复位 */
#define PELCO_CMD_RESET           0x0F    /* 复位云台或球机 */
#define PELCO_CMD_STOP            0x00    /* 停止动作 */

/* ====================================================================================
 * Pelco-D 扩展 —— 位置查询 (Query Position)
 * ==================================================================================== */

/* 查询云台水平位置（Query Pan Position） */
#define PELCO_CMD2_QUERY_PAN_POS          0x51    /* 查询水平(Pan)位置 */
#define PELCO_CMD2_QUERY_PAN_POS_RESP     0x59    /* 查询水平位置的响应高低字节 */

/* 查询云台垂直位置（Query Tilt Position） */
#define PELCO_CMD2_QUERY_TILT_POS         0x53    /* 查询垂直(Tilt)位置 */
#define PELCO_CMD2_QUERY_TILT_POS_RESP    0x5B    /* 查询垂直位置响应 */

/* 查询镜头变倍(Zoom)位置 */
#define PELCO_CMD2_QUERY_ZOOM_POS         0x55    /* 查询 Zoom 位置 */
#define PELCO_CMD2_QUERY_ZOOM_POS_RESP    0x5D    /* 响应 Zoom 位置（高低字节） */


/***************************************************************************************
 * DATA1 / DATA2 说明（Pelco-D 标准 B 级指令集）
 *
 * 一、方向控制时（CMD2 有方向位）
 *   DATA1 = 水平速度（Pan Speed）
 *       范围：0x00 ~ 0x3F（0~63）
 *       - 当 CMD2 包含 PELCO_CMD2_PAN_LEFT 或 PELCO_CMD2_PAN_RIGHT 时有效
 *       - 数值越大，水平移动速度越快
 *
 *   DATA2 = 垂直速度（Tilt Speed）
 *       范围：0x00 ~ 0x3F（0~63）
 *       - 当 CMD2 包含 PELCO_CMD2_TILT_UP 或 PELCO_CMD2_TILT_DOWN 时有效
 *       - 数值越大，垂直移动速度越快
 *
 *
 * 二、镜头控制类（CMD1 = Zoom / Focus / Iris）
 *   - Zoom Tele / Wide
 *   - Focus Near / Far
 *   - Iris Open / Close
 *
 *   DATA1 = 0x00
 *   DATA2 = 0x00
 *
 *   镜头类指令不使用速度字节。
 *
 *
 * 三、扩展命令（预置点 / 辅助开关 / 复位）
 *   CMD2 为固定命令字（例如 0x03/0x05/0x07/0x09 等）
 *
 *   DATA1 = 预置点编号（0~0xFF）
 *   DATA2 = 0x00
 *
 *   示例：
 *       清除预置点 10：CMD2=0x05  DATA1=10  DATA2=0
 *
 ***************************************************************************************/

 #endif /* __PELCOD_COMMANDS_H__ */


