#include "main.h"
#include "can.h"
#include "bsp.h"

/* =====================================================================
 * 软3 CAN 通信 —— 同一份 can.c/can.h，两个角色由构建预设切换：
 *   cmake --preset master  -> 主机：按键按下发 0x11，收到的数据打印到串口
 *   cmake --preset slave   -> 从机：收 0x11 后回 0x22，并翻转 LED
 *
 * 上电日志第一行会打印角色，方便现场确认烧的是哪块
 * ===================================================================== */

static const char *RoleName(void)
{
#if defined(CAN_ROLE_MASTER)
  return "master";
#elif defined(CAN_ROLE_SLAVE)
  return "slave";
#else
#error "CAN_ROLE_MASTER or CAN_ROLE_SLAVE must be defined (see CMakePresets.json)"
#endif
}

/*
 * 统一时钟树：外部 8 MHz 晶振 -> PLL x9 = 72 MHz。
 * APB1 = 36 MHz（CAN1），APB2 = 72 MHz（USART1）。
 * 不能只依赖 system_stm32f1xx.c 的模板；该模板不会替应用配置 PLL。
 */
static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};

  osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  osc.HSEState       = RCC_HSE_ON;
  osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  osc.PLL.PLLState   = RCC_PLL_ON;
  osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  osc.PLL.PLLMUL     = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
    Error_Handler();
  }

  clk.ClockType      = RCC_CLOCKTYPE_HCLK |
                       RCC_CLOCKTYPE_SYSCLK |
                       RCC_CLOCKTYPE_PCLK1 |
                       RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV2;
  clk.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

/* 主机：按键边沿触发发送，随后持续打印收到的帧 */
static void RunMaster(void)
{
  bsp_printf("role=master  KEY=PB0  press to TX ID=0x11");
  bool key_last = false;
  can_frame_t f;

  for (;;) {
    bool key_now = BSP_Key_Pressed();
    if (key_now && !key_last) {
      uint8_t d = 0x11;
      if (CAN_Send(CAN_ID_CMD, &d, 1)) {
        bsp_printf("TX  ID=0x11  DATA=0x11");
      } else {
        bsp_printf("TX  failed (mailbox busy)");
      }
      /* 消抖：按住期间不再触发 */
      while (BSP_Key_Pressed()) {
        HAL_Delay(10);
      }
      HAL_Delay(20);
    }
    key_last = key_now;

    /* 题面要求：每次主机收到数据后打印在串口助手上 */
    if (CAN_Recv(&f, 5)) {
      bsp_printf("RX  ID=0x%03lX  DLC=%u  DATA0=0x%02X",
                 (unsigned long)f.id, (unsigned)f.len, (unsigned)f.data[0]);
    }
  }
}

/* 从机：每收到一帧就翻转 LED，并回 ACK 0x22 */
static void RunSlave(void)
{
  bsp_printf("role=slave   LED=PC13 toggling per RX frame");
  can_frame_t f;

  for (;;) {
    if (CAN_Recv(&f, 50)) {
      bsp_printf("RX  ID=0x%03lX  DLC=%u  -> toggle LED, reply 0x22",
                 (unsigned long)f.id, (unsigned)f.len);
      BSP_LED_Toggle();

      uint8_t d = 0x22;
      if (!CAN_Send(CAN_ID_ACK, &d, 1)) {
        bsp_printf("TX  failed (mailbox busy)");
      }
    }
  }
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  BSP_UART_Init();
  BSP_LED_Init();
  BSP_Key_Init();

  bsp_printf("STM32F103C8T6 CAN demo  role=%s  SYSCLK=%lu Hz  APB1=%lu Hz  bitrate=%u",
             RoleName(), (unsigned long)SystemCoreClock,
             (unsigned long)HAL_RCC_GetPCLK1Freq(), (unsigned)CAN_BITRATE);

  /* 先用 CAN_WORK_LOOPBACK 可单板自测（不接收发器也能跑）；
   * 双板联调使用 CAN_WORK_NORMAL，并重新编译对应角色固件 */
  CAN_Init(CAN_WORK_NORMAL);

#if defined(CAN_ROLE_MASTER)
  RunMaster();
#else
  RunSlave();
#endif
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {
    /* 系统性错误：挂死等待调试器 */
  }
}
