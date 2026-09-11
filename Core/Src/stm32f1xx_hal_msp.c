#include "main.h"

/* HAL 各外设初始化的底层收尾：时钟使能 + 引脚复用配置 */

void HAL_MspInit(void)
{
  __HAL_RCC_AFIO_CLK_ENABLE();
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
  if (hcan->Instance != CAN1) {
    return;
  }
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef io = {0};
  /* PA11 = CAN_RX：浮空输入（RM0008 规定的 CAN RX 复用配置） */
  io.Pin  = GPIO_PIN_11;
  io.Mode = GPIO_MODE_INPUT;
  io.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &io);
  /* PA12 = CAN_TX：复用推挽 */
  io.Pin  = GPIO_PIN_12;
  io.Mode = GPIO_MODE_AF_PP;
  io.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &io);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  if (huart->Instance != USART1) {
    return;
  }
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef io = {0};
  /* PA9 = USART1_TX：复用推挽 */
  io.Pin   = GPIO_PIN_9;
  io.Mode  = GPIO_MODE_AF_PP;
  io.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &io);
  /* PA10 = USART1_RX：浮空输入 */
  io.Pin  = GPIO_PIN_10;
  io.Mode = GPIO_MODE_INPUT;
  io.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &io);
}
