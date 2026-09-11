#ifndef __MAIN_H
#define __MAIN_H

#include <stdbool.h>
#include "stm32f1xx_hal.h"

/* ---- 引脚规划（Blue Pill / 最小系统板通用） ----
 * USART1  PA9(TX) / PA10(RX)   115200，接 USB 转串口看日志
 * CAN1    PA11(RX) / PA12(TX)  F103 默认映射，无需 AFIO remap
 *         -> TJA1050: PA12 -> TXD(直连)，RXD -> 分压(1k/2k) -> PA11
 * LED     PC13                 板载灯，低电平点亮
 * KEY     PB0                  内部上拉，按键接 PB0<->GND，低有效
 */
#define UART_BAUD            115200

/* CAN 位时序：APB1=36MHz / 预分频9 = 4MHz tq；1+5+2=8 tq/bit -> 500kbps，采样点 6/8=75% */
#define CAN_BITRATE          500000U

/* 应用层协议（软3 题面）：主机按键 -> 发 0x11；从机收到 -> 回 0x22 */
#define CAN_ID_CMD           0x11U
#define CAN_ID_ACK           0x22U

void Error_Handler(void);

#endif
