#include "main.h"
void SystemClock_Config(void);
// 定义全局变量
PID_TypeDef PID_x, PID_y;  // 两个PID结构体PID_x和PID_y

int coords[2];  // 当前坐标数组
TIM_HandleTypeDef htim3;
uint16_t targetX = 640;  // 当前x坐标
uint16_t targetY = 360;  // 当前y坐标
/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    uint16_t pwmval_x, pwmval_y;  // 控制占空比的变量(CCR寄存器数值)

    delay_init(72);      // 延时函数初始化
    usart_init(115200);  // 串口初始化为115200
    led_init();          // LED端口初始化
    key_init();          // 初始化与按键连接的硬件接口

    // 使用 HAL 库初始化定时器3的 PWM 功能
    TIM3_PWM_Init(1000 - 1, 144 - 1);

    // PID 参数初始化
    pid_init(0.03, 0, 0.30, &PID_x);
    pid_init(0.03, 0, 0.30, &PID_y);

    // 初始化坐标值
    coords[0] = 640;
    coords[1] = 360;

    // 初始化舵机角度
    pwmval_x = 730;
    pwmval_y = 730;

    while (1) {
        // 1、从串口读取当前坐标值，存入 coords 数组中
        recieveData();

        // 2、使用 PID 计算控制舵机 PWM 占空比的参数
        pwmval_x = pwmval_x + pid(coords[0], targetX, &PID_x);
        pwmval_y = pwmval_y - pid(coords[1], targetY, &PID_y);

        // 3、分别将控制参数赋值给定时器3的 PWM 通道1、通道2
        if (pwmval_x > 300 && pwmval_x < 1200)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwmval_x);  // 设置通道1占空比
        if (pwmval_y > 400 && pwmval_y < 900)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwmval_y);  // 设置通道2占空比
    }
    // // 测试 X 轴极限
    // while (1) {
    //     __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 200);  // 从小值开始
    //     delay_ms(1000);                                     // 等待舵机转动
    // //测试 Y 轴极限
    // while (1) {
    //     __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 400);
    //     delay_ms(1000);
    // }
}

void SystemClock_Config(void) {
    RCC_ClkInitTypeDef clkinitstruct = {0};
    RCC_OscInitTypeDef oscinitstruct = {0};

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    oscinitstruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscinitstruct.HSEState       = RCC_HSE_ON;
    oscinitstruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    oscinitstruct.PLL.PLLState   = RCC_PLL_ON;
    oscinitstruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    oscinitstruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&oscinitstruct) != HAL_OK) {
        /* Initialization Error */
        while (1)
            ;
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
       clocks dividers */
    clkinitstruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    clkinitstruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clkinitstruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
    clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2) != HAL_OK) {
        /* Initialization Error */
        while (1)
            ;
    }
}

#ifdef USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line) {
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1) {
    }
}
#endif

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
