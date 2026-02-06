#include "./BSP/KEY/key.h"

#include "./SYSTEM/delay/delay.h"

// 按键初始化函数
void key_init(void) {
    GPIO_InitTypeDef gpio_init_struct;
    KEY0_GPIO_CLK_ENABLE();  // KEY0时钟使能
    KEY1_GPIO_CLK_ENABLE();  // KEY1时钟使能
    WKUP_GPIO_CLK_ENABLE();  // WKUP时钟使能

    // 设置KEY0 KEY1 WKUP引脚模式并初始化
    // 设置KEY0引脚模式，上拉输入
    gpio_init_struct.Pin   = KEY0_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_INPUT;
    gpio_init_struct.Pull  = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(KEY0_GPIO_PORT, &gpio_init_struct);

    // 设置KEY1引脚模式，上拉输入
    gpio_init_struct.Pin   = KEY1_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_INPUT;
    gpio_init_struct.Pull  = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(KEY1_GPIO_PORT, &gpio_init_struct);

    // 设置WKUP引脚模式，下拉输入
    gpio_init_struct.Pin   = WKUP_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_INPUT;
    gpio_init_struct.Pull  = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(WKUP_GPIO_PORT, &gpio_init_struct);
}

// 按键扫描函数
uint8_t key_scan(uint8_t mode) {
    static uint8_t key_up = 1;  // 标志按键松开

    if (mode)
        key_up = 1;  // mode=1
                     // 支持连续按，当按键按下一直不放时，每次调用都会返回键值

    if (key_up && (KEY0 == 0 || KEY1 == 0 || WK_UP == 1))  // 按键松开标志为1, 且有任意一个按键按下了
    {
        delay_ms(10);  // 去抖
        key_up = 0;
        if (KEY0 == 0)
            return KEY0_PRES;
        if (KEY1 == 0)
            return KEY1_PRES;
        if (WK_UP == 1)
            return WKUP_PRES;
    } else if (KEY0 == 1 && KEY1 == 1 && WK_UP == 0)  // 没有任何按键按下，标记按键松开
    {
        key_up = 1;
    }
    return 0;  // 无按键按下
}
