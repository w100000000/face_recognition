#ifndef __USART_H
#define __USART_H

#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_uart.h"
#include "sys.h"

#define USART_TX_GPIO_CLK_ENABLE()    \
    do {                              \
        __HAL_RCC_GPIOA_CLK_ENABLE(); \
    } while (0)  // PA口时钟使能
#define USART_RX_GPIO_CLK_ENABLE()    \
    do {                              \
        __HAL_RCC_GPIOA_CLK_ENABLE(); \
    } while (0)  // PA口时钟使能

#define USART_UX USART1
#define USART_UX_IRQn USART1_IRQn
#define USART_UX_IRQHandler USART1_IRQHandler
#define USART_UX_CLK_ENABLE()          \
    do {                               \
        __HAL_RCC_USART1_CLK_ENABLE(); \
    } while (0)  // USART1时钟使能
/******************************************************************************************/

#define USART_REC_LEN 200  // 最大接收字节数200
#define USART_EN_RX 1      // 使能（1）/禁止（0）串口1接收
#define RXBUFFERSIZE 1     // 接收缓存区大小

extern UART_HandleTypeDef g_uart1_handle;

extern uint8_t g_usart_rx_buf[USART_REC_LEN];  // 接收缓冲,最大USART_REC_LEN个字节.末字节为换行符
extern uint16_t g_usart_rx_sta;                // 接收状态标记
extern uint8_t g_rx_buffer[RXBUFFERSIZE];      // HAL库USART接收Buffer

void usart_init(uint32_t bound);  // 串口初始化函数

#endif
