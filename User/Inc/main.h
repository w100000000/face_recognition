#ifndef __MAIN_H
#define __MAIN_H
#include "delay.h"
#include "key.h"
#include "led.h"
#include "pid.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_cortex.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_tim.h"
#include "stm32f1xx_hal_uart.h"
#include "timer.h"
#include "usart.h"

void SystemClock_Config(void);
void assert_failed(uint8_t* file, uint32_t line);

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
