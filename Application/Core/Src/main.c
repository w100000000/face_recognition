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

// 配置参数
#define SCREEN_WIDTH 1280     // 图像宽度
#define SCREEN_HEIGHT 720     // 图像高度
#define FACE_TIMEOUT_MS 3000  // 人脸超时时间(ms)
#define SMOOTH_FACTOR 10      // 回中平滑系数，利用加权平均(越大越平滑)
#define CONTROL_PERIOD_MS 20  // 控制环目标周期(ms)
#define JITTER_REPORT_SAMPLES 500
#define CPU_REPORT_INTERVAL_MS 1000

// 状态定义
typedef enum {
    STATE_IDLE      = 0,  // 空闲（无人脸检测）
    STATE_TRACKING  = 1,  // 追踪中（有有效人脸）
    STATE_LOST      = 2,  // 目标丢失（刚丢失，可能还会回来）
    STATE_RETURNING = 3   // 回中（平滑回到中点）
} TrackingState_t;

const char* state_names[] = {"IDLE", "TRACKING", "LOST", "RETURNING"};

// 定义全局变量
PID_TypeDef PID_x, PID_y;                       // 两个PID结构体PID_x和PID_y
TrackingState_t g_tracking_state = STATE_IDLE;  // 追踪状态
uint32_t g_lost_time             = 0;           // 目标丢失时刻
const uint32_t LOST_TIMEOUT_MS   = 3000;        // 目标丢失后3秒开始回中

QueueHandle_t qFaceData = NULL;

int coords[2];           // 当前坐标数组
uint16_t targetX = 640;  // 当前x坐标
uint16_t targetY = 360;  // 当前y坐标

uint32_t system_time_sec              = 0;  // 系统运行秒数
uint32_t last_tick                    = 0;  // 上次更新时间的tick值
static uint16_t pwmval_x              = 1500;
static uint16_t pwmval_y              = 1500;
volatile uint32_t g_idle_hook_counter = 0;
volatile int32_t g_jitter_min_ms      = 0;
volatile int32_t g_jitter_max_ms      = 0;
volatile uint32_t g_jitter_avg_abs_ms = 0;
volatile uint32_t g_jitter_sample_n   = 0;

extern char g_recognized_name[32];  // 从串口解析模块获取的识别人名

// 看门狗句柄
IWDG_HandleTypeDef hiwdg;

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  更新追踪状态
 * @param  face_data 人脸数据
 * @retval None
 */
void update_tracking_state(FaceData_t* face_data) {
    uint32_t current_time = HAL_GetTick();
    uint32_t data_age     = current_time - face_data->timestamp;

    // 数据有效性检查
    uint8_t data_valid = (face_data->valid && data_age < FACE_TIMEOUT_MS && face_data->x >= 0 &&
                          face_data->x <= SCREEN_WIDTH && face_data->y >= 0 && face_data->y <= SCREEN_HEIGHT);

    // 状态转移逻辑
    switch (g_tracking_state) {
        case STATE_IDLE:
            if (data_valid) {
                g_tracking_state = STATE_TRACKING;
                LED0(0);  // LED灭
            }
            break;

        case STATE_TRACKING:
            if (!data_valid) {
                g_tracking_state = STATE_LOST;
                g_lost_time      = current_time;
                LED0(1);  // LED亮起
            }
            break;

        case STATE_LOST:
            if (data_valid) {
                // 目标重新出现，回到追踪
                g_tracking_state = STATE_TRACKING;
                LED0(0);
            } else if (current_time - g_lost_time > LOST_TIMEOUT_MS) {
                // 丢失超过3秒，开始平滑回中
                g_tracking_state = STATE_RETURNING;
            }
            break;

        case STATE_RETURNING:
            if (data_valid) {
                // 追踪期间收到新人脸，直接回到追踪
                g_tracking_state = STATE_TRACKING;
                LED0(0);
            } else if (abs(coords[0] - 640) < 5 && abs(coords[1] - 360) < 5) {
                // 已回到中点
                g_tracking_state = STATE_IDLE;
            }
            break;

        default:
            g_tracking_state = STATE_IDLE;
    }
}

/**
 * @brief  根据状态执行对应动作
 * @param  face_data 人脸数据
 * @retval None
 */
void execute_tracking_action(FaceData_t* face_data) {
    uint32_t current_time = HAL_GetTick();
    uint32_t data_age     = current_time - face_data->timestamp;

    switch (g_tracking_state) {
        case STATE_IDLE:
            // 空闲：保持中点，不更新坐标
            break;

        case STATE_TRACKING:
            // 追踪：使用人脸坐标
            if (face_data->valid && data_age < FACE_TIMEOUT_MS && face_data->x >= 0 && face_data->x <= SCREEN_WIDTH &&
                face_data->y >= 0 && face_data->y <= SCREEN_HEIGHT) {
                coords[0] = face_data->x;
                coords[1] = face_data->y;
            }
            break;

        case STATE_LOST:
            // 目标丢失：保持当前位置，但不更新坐标
            break;

        case STATE_RETURNING:
            // 回中：平滑移动到中点
            coords[0] = (coords[0] * (SMOOTH_FACTOR - 1) + (SCREEN_WIDTH / 2)) / SMOOTH_FACTOR;
            coords[1] = (coords[1] * (SMOOTH_FACTOR - 1) + (SCREEN_HEIGHT / 2)) / SMOOTH_FACTOR;
            break;

        default:
            break;
    }
}

/**
 * @brief  初始化独立看门狗（超时约4秒）
 * @param  None
 * @retval None
 */
void IWDG_Init(void) {
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;  // 40KHz / 64 = 625Hz
    hiwdg.Init.Reload    = 2500;               // 2500 / 625Hz = 4秒超时

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        // 初始化失败处理
        while (1) {
        }
    }
}

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
    // usmart_init(72);     // 初始化USMART调试组件
    led_init();      // LED端口初始化
    key_init();      // 初始化与按键连接的硬件接口
    beep_init();     // 初始化蜂鸣器
    at24c02_init();  // 初始化24C02 EEPROM
    lcd_init();      // 初始化LCD
    delay_ms(100);   // 等待LCD稳定

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
    g_back_color = WHITE;                                        // 明确设置背景色
    lcd_fill(0, 0, lcddev.width - 1, lcddev.height - 1, WHITE);  // 强制全屏白底
    lcd_show_string(10, 10, 200, 24, 16, "Name:", BLUE);         // 显示标签
    lcd_show_string(10, 40, 200, 24, 16, "Time:", BLUE);         // 时间标签
    lcd_show_string(10, 100, 200, 24, 16, "Jit:", BLUE);         // 抖动标签
    lcd_show_string(70, 10, 150, 24, 16, "No Face", RED);        // 默认显示没检测到人脸

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

    // 启动看门狗（RTOS调度器启动后开始喂狗）
    IWDG_Init();

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
    TickType_t prevTick     = 0;
    int32_t jitterMin       = 0;
    int32_t jitterMax       = 0;
    uint32_t jitterCount    = 0;
    uint32_t jitterAbsSum   = 0;
    (void)argument;

    while (1) {
        TickType_t nowTick = xTaskGetTickCount();
        if (prevTick != 0) {
            int32_t periodMs = (int32_t)((nowTick - prevTick) * portTICK_PERIOD_MS);
            int32_t jitterMs = periodMs - CONTROL_PERIOD_MS;
            int32_t absJitterMs;

            if (jitterCount == 0) {
                jitterMin = jitterMs;
                jitterMax = jitterMs;
            } else {
                if (jitterMs < jitterMin) {
                    jitterMin = jitterMs;
                }
                if (jitterMs > jitterMax) {
                    jitterMax = jitterMs;
                }
            }

            absJitterMs = (jitterMs >= 0) ? jitterMs : -jitterMs;
            jitterAbsSum += (uint32_t)absJitterMs;
            jitterCount++;

            if (jitterCount >= JITTER_REPORT_SAMPLES) {
                uint32_t avgAbsJitter = jitterAbsSum / jitterCount;
                g_jitter_min_ms       = jitterMin;
                g_jitter_max_ms       = jitterMax;
                g_jitter_avg_abs_ms   = avgAbsJitter;
                g_jitter_sample_n     = jitterCount;

                jitterCount  = 0;
                jitterAbsSum = 0;
            }
        }
        prevTick = nowTick;

        // 喂狗（4秒超时，20ms喂一次）
        HAL_IWDG_Refresh(&hiwdg);

        // 按下 KEY0 立即回到舵机中点并重置状态
        if (key_scan(0) == KEY0_PRES) {
            targetX          = 640;
            targetY          = 360;
            pwmval_x         = 1500;
            pwmval_y         = 1500;
            g_tracking_state = STATE_IDLE;  // 重置状态
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwmval_x);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwmval_y);
        }

        // 读取人脸数据
        if (xQueuePeek(qFaceData, (void*)&faceData, 0) == pdTRUE) {
            // 更新状态
            update_tracking_state(&faceData);

            // 执行状态对应的动作
            execute_tracking_action(&faceData);
        }

        // PID 计算并输出舵机PWM
        pwmval_x = pwmval_x + pid(coords[0], targetX, &PID_x);
        pwmval_y = pwmval_y - pid(coords[1], targetY, &PID_y);

        if (pwmval_x > 600 && pwmval_x < 2400)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwmval_x);
        if (pwmval_y > 500 && pwmval_y < 2000)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwmval_y);

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(CONTROL_PERIOD_MS));
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
    uint32_t idleCounterLast = 0;
    uint32_t idleCounterPeak = 1;

    while (1) {
        // 每秒更新一次时间、姓名和状态显示
        if (HAL_GetTick() - last_tick >= CPU_REPORT_INTERVAL_MS) {
            system_time_sec++;
            last_tick = HAL_GetTick();

            uint32_t idleCounterNow = g_idle_hook_counter;
            uint32_t idleDelta      = idleCounterNow - idleCounterLast;
            idleCounterLast         = idleCounterNow;
            if (idleDelta > idleCounterPeak) {
                idleCounterPeak = idleDelta;
            }
            uint32_t idlePercent = (idleDelta * 100U) / idleCounterPeak;

            uint16_t hours   = system_time_sec / 3600;
            uint16_t minutes = (system_time_sec % 3600) / 60;
            uint16_t seconds = system_time_sec % 60;

            char time_str[16];
            sprintf(time_str, "%02d:%02d:%02d", hours, minutes, seconds);
            lcd_fill(70, 40, 200, 56, WHITE);
            lcd_show_string(70, 40, 150, 24, 16, time_str, BLUE);

            lcd_fill(70, 10, 220, 26, WHITE);
            lcd_show_string(70, 10, 150, 24, 16, g_recognized_name, RED);

            // 显示追踪状态
            char state_str[32];
            sprintf(state_str, "State: %s", state_names[g_tracking_state]);
            lcd_fill(10, 70, 200, 86, WHITE);
            uint16_t state_color = (g_tracking_state == STATE_TRACKING) ? GREEN : BLUE;
            lcd_show_string(10, 70, 200, 16, 12, state_str, state_color);

            char jitter_str[40];
            sprintf(jitter_str, "N%lu %ld/%ld A%lu", (unsigned long)g_jitter_sample_n, (long)g_jitter_min_ms,
                    (long)g_jitter_max_ms, (unsigned long)g_jitter_avg_abs_ms);
            lcd_fill(40, 100, 240, 116, WHITE);
            lcd_show_string(40, 100, 220, 16, 12, jitter_str, BLUE);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vApplicationIdleHook(void) {
    g_idle_hook_counter++;
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
