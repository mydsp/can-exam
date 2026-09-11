#ifndef __BSP_H
#define __BSP_H

#include "main.h"

/* 板级外设：串口打印 / LED / 按键。引脚见 main.h 注释 */

void BSP_UART_Init(void);
/* 线程不安全的简易格式化打印（printf 风格，最多 96 字节一行） */
void bsp_printf(const char *fmt, ...);

void BSP_LED_Init(void);          /* PC13，低电平点亮 */
void BSP_LED_Toggle(void);

void BSP_Key_Init(void);          /* PB0 内部上拉，低有效 */
bool BSP_Key_Pressed(void);

#endif
