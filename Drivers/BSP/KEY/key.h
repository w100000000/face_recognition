#ifndef __KEY_H
#define __KEY_H

#include "sys.h"

#define KEY0_GPIO_PORT GPIOE
#define KEY0_GPIO_PIN GPIO_PIN_4
#define KEY0_GPIO_CLK_ENABLE()        \
    do {                              \
        __HAL_RCC_GPIOE_CLK_ENABLE(); \
    } while (0)  // KEY0引脚PE4，PE口时钟使能

#define KEY1_GPIO_PORT GPIOE
#define KEY1_GPIO_PIN GPIO_PIN_3
#define KEY1_GPIO_CLK_ENABLE()        \
    do {                              \
        __HAL_RCC_GPIOE_CLK_ENABLE(); \
    } while (0)  // KEY1引脚PE3，PE口时钟使能

#define WKUP_GPIO_PORT GPIOA
#define WKUP_GPIO_PIN GPIO_PIN_0
#define WKUP_GPIO_CLK_ENABLE()        \
    do {                              \
        __HAL_RCC_GPIOA_CLK_ENABLE(); \
    } while (0)  // WKUP引脚PA0，PA口时钟使能

// 读取按键状态宏定义
#define KEY0 HAL_GPIO_ReadPin(KEY0_GPIO_PORT, KEY0_GPIO_PIN)
#define KEY1 HAL_GPIO_ReadPin(KEY1_GPIO_PORT, KEY1_GPIO_PIN)
#define WK_UP HAL_GPIO_ReadPin(WKUP_GPIO_PORT, WKUP_GPIO_PIN)

// 按键状态定义
#define KEY0_PRES 1  // KEY0按下
#define KEY1_PRES 2  // KEY1按下
#define WKUP_PRES 3  // KEY_UP按下
uint8_t key_scan(uint8_t mode);

#endif
