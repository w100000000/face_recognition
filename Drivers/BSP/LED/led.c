#include "led.h"

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"

// 初始化LED0和LED1的GPIO口.并使能这两个口的时钟
void led_init(void) {
    GPIO_InitTypeDef gpio_init_struct;
    LED0_GPIO_CLK_ENABLE();
    LED1_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin   = LED0_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull  = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LED0_GPIO_PORT, &gpio_init_struct);  // 初始化LED0引脚

    gpio_init_struct.Pin = LED1_GPIO_PIN;
    HAL_GPIO_Init(LED1_GPIO_PORT, &gpio_init_struct);  // 初始化LED1引脚

    LED0(1);
    LED1(1);  // 默认关闭LED0和LED1
}
