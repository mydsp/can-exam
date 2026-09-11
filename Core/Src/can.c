#include "can.h"
#include <string.h>

/* ============ CAN1 底层（HAL） ============
 * F103 的 bxCAN 控制器在 APB1 上；引脚 PA11(RX)/PA12(TX) 为默认映射
 * 位时序：APB1 36MHz / prescaler 9 = 4MHz tq；1+5+2 = 8 tq/bit -> 500kbps
 * 采样点 = (1+5)/8 = 75%，双端同晶振同时钟树，两端天然一致
 */
static CAN_HandleTypeDef hcan1;

void CAN_Init(can_work_mode_t mode)
{
  __HAL_RCC_CAN1_CLK_ENABLE();

  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler         = 9;
  hcan1.Init.Mode              = (mode == CAN_WORK_LOOPBACK) ? CAN_MODE_LOOPBACK : CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth     = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1          = CAN_BS1_5TQ;
  hcan1.Init.TimeSeg2          = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff        = ENABLE;    /* 连续错误后自动离线，避免总线被错误帧刷屏 */
  hcan1.Init.AutoWakeUp        = DISABLE;
  hcan1.Init.AutoRetransmission = ENABLE;   /* 仲裁丢失/错误自动重发 */
  hcan1.Init.ReceiveFifoLocked  = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK) {
    Error_Handler();
  }

  /* 过滤器：全部放行，帧进 FIFO0。bxCAN 不配过滤器收不到任何帧，这步不能省 */
  CAN_FilterTypeDef f = {0};
  f.FilterActivation     = ENABLE;
  f.FilterMode           = CAN_FILTERMODE_IDMASK;
  f.FilterScale          = CAN_FILTERSCALE_32BIT;
  f.FilterIdHigh         = 0x0000;
  f.FilterIdLow          = 0x0000;
  f.FilterMaskIdHigh     = 0x0000;          /* mask=0 -> 所有位都"不关心" */
  f.FilterMaskIdLow      = 0x0000;
  f.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  f.FilterBank           = 0;
  f.SlaveStartFilterBank = 14;
  if (HAL_CAN_ConfigFilter(&hcan1, &f) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_CAN_Start(&hcan1) != HAL_OK) {
    Error_Handler();
  }
}

bool CAN_Send(uint32_t id, const uint8_t *data, uint8_t len)
{
  if (len > 8 || HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0) {
    return false;
  }
  CAN_TxHeaderTypeDef h = {0};
  h.StdId = id;
  h.IDE   = CAN_ID_STD;
  h.RTR   = CAN_RTR_DATA;
  h.DLC   = len;
  uint32_t mailbox;
  return HAL_CAN_AddTxMessage(&hcan1, &h, (uint8_t *)data, &mailbox) == HAL_OK;
}

bool CAN_Recv(can_frame_t *f, uint32_t timeout_ms)
{
  uint32_t t0 = HAL_GetTick();
  while (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) == 0) {
    if (HAL_GetTick() - t0 >= timeout_ms) {
      return false;
    }
  }
  CAN_RxHeaderTypeDef h;
  uint8_t buf[8];
  if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &h, buf) != HAL_OK) {
    return false;
  }
  f->id  = h.IDE ? h.ExtId : h.StdId;
  f->len = h.DLC > 8 ? 8 : h.DLC;
  memcpy(f->data, buf, f->len);
  return true;
}

void CAN_FlushRx(void)
{
  while (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0) {
    CAN_RxHeaderTypeDef h;
    uint8_t buf[8];
    if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &h, buf) != HAL_OK) {
      break;
    }
  }
}
