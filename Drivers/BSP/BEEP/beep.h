#ifndef __BEEP_H
#define __BEEP_H

#include "sys.h"

// 蜂鸣器引脚定义
#define BEEP_GPIO_PORT GPIOB
#define BEEP_GPIO_PIN GPIO_PIN_8
#define BEEP_GPIO_CLK_ENABLE()        \
    do {                              \
        __HAL_RCC_GPIOB_CLK_ENABLE(); \
    } while (0)  // 蜂鸣器引脚PB8，PB口时钟使能

// 蜂鸣器控制宏定义
#define BEEP(x)                                                               \
    do {                                                                      \
        x ? HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_SET)    \
          : HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_RESET); \
    } while (0)

// 蜂鸣器状态切换宏定义
#define BEEP_TOGGLE()                                      \
    do {                                                   \
        HAL_GPIO_TogglePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN); \
    } while (0)

void beep_init(void);

#endif
