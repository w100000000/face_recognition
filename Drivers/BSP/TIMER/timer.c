#include "timer.h"

#include <stdint.h>

#include "led.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_tim.h"
#include "usart.h"

TIM_HandleTypeDef htim3;

static void Error_Handler(void);

// TIM3中断初始化
void TIM3_Int_Init(uint16_t arr, uint16_t psc) {
    __HAL_RCC_TIM3_CLK_ENABLE();  // 使能定时器时钟

    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = psc;                     // 预分频系数
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;      // 递增计数模式
    htim3.Init.Period            = arr;                     // 自动重装载值
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;  // 时钟分频
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(TIM3_IRQn, 0, 3);  // 设置中断优先级
    HAL_NVIC_EnableIRQ(TIM3_IRQn);          // 使能中断

    HAL_TIM_Base_Start_IT(&htim3);  // 启动定时器中断
}

// 定时器3中断服务函数
void TIM3_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim3);  // 调用HAL库函数处理定时器中断
}

// 定时器周期中断回调函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM3) {
        LED1_TOGGLE();
    }
}

// TIM3 PWM部分初始化
void TIM3_PWM_Init(uint16_t arr, uint16_t psc) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    TIM_OC_InitTypeDef sConfigOC     = {0};
    // 使能GPIO和定时器时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();  // 改为GPIOA
    __HAL_RCC_TIM3_CLK_ENABLE();
    // 配置GPIO
    GPIO_InitStruct.Pin   = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;  // 复用推挽输出
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  // 改为GPIOA
    // 配置定时器
    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = psc;
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = arr;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    // 配置PWM通道
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;      // PWM模式1
    sConfigOC.Pulse      = 1500;                 // 初始CCR值，使舵机在中间位置(1.5ms)
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;  // 极性为高
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }
    // 启动PWM输出
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
}
// 错误处理函数
static void Error_Handler(void) {
    HAL_TIM_Base_Stop_IT(&htim3);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
    while (1) {
        LED1_TOGGLE();
        HAL_Delay(100);
    }
}