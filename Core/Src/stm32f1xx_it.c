#include "main.h"

/* 中断服务函数：本工程全部采用轮询，唯一必须实现的是 SysTick（HAL 心跳） */

void NMI_Handler(void)            { while (1); }
void HardFault_Handler(void)      { while (1); }
void MemManage_Handler(void)      { while (1); }
void BusFault_Handler(void)       { while (1); }
void UsageFault_Handler(void)     { while (1); }
void SVC_Handler(void)            {}
void DebugMon_Handler(void)       {}
void PendSV_Handler(void)         {}

void SysTick_Handler(void)
{
  HAL_IncTick();
}
