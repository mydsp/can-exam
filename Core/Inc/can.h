#ifndef __CAN_H
#define __CAN_H

#include "main.h"

/* 软3 要求"模块化思路，写出通用的 can 通信函数"——
 * 本文件即通用接口：换应用场景时只改 main，不动 can.c */

/* 注意：不叫 CAN_MODE_* —— 那是 HAL bxCAN 的模式宏，会撞名 */
typedef enum {
  CAN_WORK_NORMAL   = 0,   /* 正常总线模式（双板联调用这个） */
  CAN_WORK_LOOPBACK = 1,   /* 片内回环，无需收发器/总线——单板自测用 */
} can_work_mode_t;

typedef struct {
  uint32_t id;             /* 标准 11 位 ID */
  uint8_t  data[8];
  uint8_t  len;
} can_frame_t;

/* 初始化：500kbps，全接收过滤，帧进 FIFO0。loopback 时单板自发自收 */
void CAN_Init(can_work_mode_t mode);

/* 发送一帧（标准帧，DLC<=8）。邮箱满/参数错返回 false，不阻塞 */
bool CAN_Send(uint32_t id, const uint8_t *data, uint8_t len);

/* 收一帧，timeout_ms 内无帧返回 false */
bool CAN_Recv(can_frame_t *f, uint32_t timeout_ms);

/* 丢弃 FIFO0 内所有积压帧（联调时清旧数据用） */
void CAN_FlushRx(void);

#endif
