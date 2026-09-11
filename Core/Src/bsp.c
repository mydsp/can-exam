#include "bsp.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ---- USART1 PA9/PA10 115200 轮询发送 ---- */
static UART_HandleTypeDef huart1;

void BSP_UART_Init(void)
{
  __HAL_RCC_USART1_CLK_ENABLE();

  huart1.Instance          = USART1;
  huart1.Init.BaudRate     = UART_BAUD;
  huart1.Init.WordLength   = UART_WORDLENGTH_8B;
  huart1.Init.StopBits     = UART_STOPBITS_1;
  huart1.Init.Parity       = UART_PARITY_NONE;
  huart1.Init.Mode         = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
  }
}

void bsp_printf(const char *fmt, ...)
{
  char buf[96];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n <= 0) {
    return;
  }
  if (n >= (int)sizeof(buf) - 2) {
    n = (int)sizeof(buf) - 2;
  }
  buf[n++] = '\r';
  buf[n++] = '\n';
  HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)n, 100);
}

/* ---- LED PC13（低电平点亮） ---- */
void BSP_LED_Init(void)
{
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitTypeDef io = {0};
  io.Pin   = GPIO_PIN_13;
  io.Mode  = GPIO_MODE_OUTPUT_PP;
  io.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   /* 初始灭 */
  HAL_GPIO_Init(GPIOC, &io);
}

void BSP_LED_Toggle(void)
{
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

/* ---- 按键 PB0（内部上拉，按下接地为低） ---- */
void BSP_Key_Init(void)
{
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef io = {0};
  io.Pin  = GPIO_PIN_0;
  io.Mode = GPIO_MODE_INPUT;
  io.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &io);
}

bool BSP_Key_Pressed(void)
{
  return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET;
}
