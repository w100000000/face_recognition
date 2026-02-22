#ifndef __MAIN_H
#define __MAIN_H
#include "24c02.h"
#include "delay.h"
#include "key.h"
#include "lcd.h"
#include "led.h"
#include "pid.h"
#include "serial_coord_parse.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_cortex.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_tim.h"
#include "stm32f1xx_hal_uart.h"
#include "stm32f1xx_hal_iwdg.h"
#include "timer.h"
#include "usart.h"
#include "usmart.h"

void SystemClock_Config(void);
void IWDG_Init(void);
void assert_failed(uint8_t* file, uint32_t line);

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
