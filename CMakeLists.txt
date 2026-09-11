cmake_minimum_required(VERSION 3.22)

# 软3 CAN 通信：CAN_ROLE=master|slave 决定编译出主机固件还是从机固件
# can.c/can.h 两套固件共用，只有 main 的角色逻辑不同 —— 这就是题目要的"模块化思路"
set(CAN_ROLE "master" CACHE STRING "master or slave")
string(TOUPPER ${CAN_ROLE} CAN_ROLE_UPPER)
if(NOT (CAN_ROLE_UPPER STREQUAL "MASTER" OR CAN_ROLE_UPPER STREQUAL "SLAVE"))
  message(FATAL_ERROR "CAN_ROLE must be master or slave, got: ${CAN_ROLE}")
endif()

set(FW_DIR "E:/Stm32cube/repository/STM32Cube_FW_F1_V1.8.5")
project(can-${CAN_ROLE} C ASM)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(
  ${CMAKE_SOURCE_DIR}/Core/Inc
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Inc
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
  ${FW_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Include
  ${FW_DIR}/Drivers/CMSIS/Include
)

# STM32F103xB = C8T6；SYSCLK_FREQ_72MHz 让 system_stm32f1xx.c 启动时 HSE 8M x9 -> 72MHz
add_definitions(-DUSE_HAL_DRIVER -DSTM32F103xB -DSYSCLK_FREQ_72MHz -DCAN_ROLE_${CAN_ROLE_UPPER}=1)

file(GLOB APP_SOURCES "${CMAKE_SOURCE_DIR}/Core/Src/*.c")

set(STARTUP_SOURCES
  ${FW_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s
  ${FW_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c
)

# 只列实际用到的 HAL 模块：CAN + UART + GPIO + RCC，编译快、体积小
set(HAL_SOURCES
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c
  ${FW_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_can.c
)

add_executable(${CMAKE_PROJECT_NAME}
  ${APP_SOURCES}
  ${HAL_SOURCES}
  ${STARTUP_SOURCES}
)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
  -T${CMAKE_SOURCE_DIR}/STM32F103C8Tx_FLASH.ld
  -Wl,--gc-sections
  -Wl,-Map=${CMAKE_PROJECT_NAME}.map
  --specs=nano.specs --specs=nosys.specs
  -Wl,-u,__errno
  -Wl,--print-memory-usage
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O ihex  $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
)
