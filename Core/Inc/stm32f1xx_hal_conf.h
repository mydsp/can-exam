/**
  ******************************************************************************
  * @brief  HAL 配置 - F103C8T6 裁剪版（只开 CAN/UART/GPIO/RCC 所需模块）
  ******************************************************************************
  */
#ifndef __STM32F1xx_HAL_CONF_H
#define __STM32F1xx_HAL_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Module Selection - 只开实际用到的模块 */
#define HAL_MODULE_ENABLED
#define HAL_CAN_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/* Include 链 - 拉入 CMSIS device (stm32f1xx.h) + 各模块头 */
#ifdef USE_HAL_DRIVER
 #include "stm32f1xx.h"
 #ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f1xx_hal_rcc.h"
 #endif
 #ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32f1xx_hal_gpio.h"
 #endif
 #ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32f1xx_hal_cortex.h"
 #endif
 #ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f1xx_hal_flash.h"
 #endif
 #ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32f1xx_hal_dma.h"
 #endif
 #ifdef HAL_UART_MODULE_ENABLED
  #include "stm32f1xx_hal_uart.h"
 #endif
 #ifdef HAL_CAN_MODULE_ENABLED
  #include "stm32f1xx_hal_can.h"
 #endif
#endif

/* 时钟值 - Blue Pill / 最小系统板标准 8MHz 无源晶振 */
#if !defined  (HSE_VALUE)
  #define HSE_VALUE              8000000U
#endif
#if !defined  (HSE_STARTUP_TIMEOUT)
  #define HSE_STARTUP_TIMEOUT    100U
#endif
#if !defined  (HSI_VALUE)
  #define HSI_VALUE              8000000U
#endif
#if !defined  (LSI_VALUE)
  #define LSI_VALUE              40000U
#endif
#if !defined  (LSE_VALUE)
  #define LSE_VALUE              32768U
#endif
#if !defined  (LSE_STARTUP_TIMEOUT)
  #define LSE_STARTUP_TIMEOUT    5000U
#endif
#if !defined  (VDD_VALUE)
  #define VDD_VALUE              3300U
#endif
#if !defined  (TICK_INT_PRIORITY)
  #define TICK_INT_PRIORITY      15U
#endif
#if !defined  (USE_RTOS)
  #define USE_RTOS               0U
#endif
#if !defined  (PREFETCH_ENABLE)
  #define PREFETCH_ENABLE        1U
#endif

/* 回调注册统一走默认句柄方式，不开启 */
#define USE_HAL_CAN_REGISTER_CALLBACKS  0U
#define USE_HAL_UART_REGISTER_CALLBACKS 0U

/* 不定义 USE_FULL_ASSERT，assert_param 为空操作 */
#define assert_param(expr) ((void)0U)

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_HAL_CONF_H */
