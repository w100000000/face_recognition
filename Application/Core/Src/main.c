#include "main.h"

#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

void SystemClock_Config(void);
static void TaskControlLoop(void* argument);
static void TaskUartRxParse(void* argument);
static void TaskUiLcd(void* argument);
// 定义全局变量
PID_TypeDef PID_x, PID_y;  // 两个PID结构体PID_x和PID_y

QueueHandle_t qFaceData = NULL;

int coords[2];           // 当前坐标数组
uint16_t targetX = 640;  // 当前x坐标
uint16_t targetY = 360;  // 当前y坐标

uint32_t system_time_sec = 0;  // 系统运行秒数
uint32_t last_tick       = 0;  // 上次更新时间的tick值
static uint16_t pwmval_x = 1500;
static uint16_t pwmval_y = 1500;

extern char g_recognized_name[32];  // 从串口解析模块获取的识别人名
/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    delay_init(72);      // 延时函数初始化
    usart_init(115200);  // 串口初始化为115200
    usmart_init(72);     // 初始化USMART调试组件
    led_init();          // LED端口初始化
    key_init();          // 初始化与按键连接的硬件接口
    beep_init();         // 初始化蜂鸣器
    at24c02_init();      // 初始化24C02 EEPROM
    lcd_init();          // 初始化LCD

    // 使用                                                                              HAL 库初始化定时器3的 PWM 功能
    TIM3_PWM_Init(20000 - 1, 72 - 1);  // 50Hz舵机PWM频率

    // PID 参数初始化
    pid_init(0.05, 0, 0.30, &PID_x);
    pid_init(0.03, 0, 0.30, &PID_y);

    // 尝试从24C02读取保存的PID参数
    pid_load_from_eeprom();

    // 初始化坐标值
    coords[0] = 640;
    coords[1] = 360;

    // 初始化舵机角度
    pwmval_x = 1500;  // 中间位置
    pwmval_y = 1500;  // 中间位置
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwmval_x);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwmval_y);

    // LCD 初始化显示
    lcd_clear(WHITE);                                      // 清屏为白色
    lcd_show_string(10, 10, 200, 24, 16, "Name:", BLUE);   // 显示标签
    lcd_show_string(10, 40, 200, 24, 16, "Time:", BLUE);   // 时间标签
    lcd_show_string(70, 10, 150, 24, 16, "No Face", RED);  // 默认显示没检测到人脸

    last_tick = HAL_GetTick();  // 记录初始时间

    qFaceData = xQueueCreate(1, sizeof(FaceData_t));
    if (qFaceData == NULL) {
        while (1) {
        }
    }

    g_uart_rx_sem = xSemaphoreCreateBinary();
    if (g_uart_rx_sem == NULL) {
        while (1) {
        }
    }

    xTaskCreate(TaskControlLoop, "Ctrl", 256, NULL, 4, NULL);
    xTaskCreate(TaskUartRxParse, "Uart", 256, NULL, 3, NULL);
    xTaskCreate(TaskUiLcd, "Ui", 256, NULL, 2, NULL);
    vTaskStartScheduler();

    while (1) {
    }
    // 测试 X 轴极限
    // while (1) {
    //     __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 600);   // 最小角度(0.5ms)
    //     delay_ms(2000);                                      // 等待舵机转动
    //     __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 1500);  // 中间角度(1.5ms)
    //     delay_ms(2000);
    //     __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 2400);  // 最大角度(2.5ms)
    //     delay_ms(2000);
    // }
    // 测试 Y 轴极限
    // while (1) {
    //     __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 500);  // 最小角度
    //     delay_ms(2000);
    //     __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 1500);  // 中间角度
    //     delay_ms(2000);
    //     __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 2000);  // 最大角度
    //     delay_ms(2000);
    // }
}

static void TaskControlLoop(void* argument) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    FaceData_t faceData     = {0};
    (void)argument;

    while (1) {
        // 按下 KEY0 立即回到舵机中点
        if (key_scan(0) == KEY0_PRES) {
            targetX  = 640;
            targetY  = 360;
            pwmval_x = 1500;
            pwmval_y = 1500;
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwmval_x);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwmval_y);
        }

        if (xQueuePeek(qFaceData, (void*)&faceData, 0) == pdTRUE && faceData.valid) {
            coords[0] = faceData.x;
            coords[1] = faceData.y;
        }

        // PID 计算并输出舵机PWM
        pwmval_x = pwmval_x + pid(coords[0], targetX, &PID_x);
        pwmval_y = pwmval_y - pid(coords[1], targetY, &PID_y);

        if (pwmval_x > 600 && pwmval_x < 2400)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwmval_x);
        if (pwmval_y > 500 && pwmval_y < 2000)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwmval_y);

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(20));
    }
}

static void TaskUartRxParse(void* argument) {
    (void)argument;

    while (1) {
        if (xSemaphoreTake(g_uart_rx_sem, portMAX_DELAY) == pdTRUE) {
            recieveData();
        }
    }
}

static void TaskUiLcd(void* argument) {
    (void)argument;

    while (1) {
        // 每秒更新一次时间和姓名显示
        if (HAL_GetTick() - last_tick >= 1000) {
            system_time_sec++;
            last_tick = HAL_GetTick();

            uint16_t hours   = system_time_sec / 3600;
            uint16_t minutes = (system_time_sec % 3600) / 60;
            uint16_t seconds = system_time_sec % 60;

            char time_str[16];
            sprintf(time_str, "%02d:%02d:%02d", hours, minutes, seconds);
            lcd_fill(70, 40, 200, 56, WHITE);
            lcd_show_string(70, 40, 150, 24, 16, time_str, BLUE);

            lcd_fill(70, 10, 220, 26, WHITE);
            lcd_show_string(70, 10, 150, 24, 16, g_recognized_name, RED);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
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
