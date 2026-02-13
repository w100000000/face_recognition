#include "beep.h"

#include "stm32f1xx_hal_gpio.h"

// 蜂鸣器初始化函数
void beep_init(void) {
    GPIO_InitTypeDef gpio_init_struct;
    BEEP_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin   = BEEP_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_OUTPUT_PP;  // 推挽输出
    gpio_init_struct.Pull  = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BEEP_GPIO_PORT, &gpio_init_struct);

    BEEP(0);
}
