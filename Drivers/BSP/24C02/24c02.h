

#ifndef __24C02_H
#define __24C02_H

#include "sys.h"

#define AT24C02 255

/* 开发板使用的是24c02，所以定义EE_TYPE为AT24C02 */

#define EE_TYPE AT24C02

void at24c02_init(void);                                            /* 初始化IIC */
uint8_t at24c02_check(void);                                        /* 检查器件 */
uint8_t at24c02_read_one_byte(uint16_t addr);                       /* 指定地址读取一个字节 */
void at24c02_write_one_byte(uint16_t addr, uint8_t data);           /* 指定地址写入一个字节 */
void at24c02_write(uint16_t addr, uint8_t* pbuf, uint16_t datalen); /* 从指定地址开始写入指定长度的数据 */
void at24c02_read(uint16_t addr, uint8_t* pbuf, uint16_t datalen);  /* 从指定地址开始读出指定长度的数据 */

#endif
