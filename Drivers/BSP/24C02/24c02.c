
#include "24c02.h"

#include "delay.h"
#include "myiic.h"
// 初始化IIC接口
void at24c02_init(void) {
    iic_init();
}

// 在AT24C02指定地址读出一个数据
uint8_t at24c02_read_one_byte(uint16_t addr) {
    uint8_t temp = 0;
    iic_start();          // 发送起始信号
    iic_send_byte(0XA0);  // 发送设备地址：写模式
    iic_wait_ack();
    iic_send_byte(addr);  // 发送字节地址（0-255），可以一次性发送完
    iic_wait_ack();

    iic_start();          // 重新发送起始信号
    iic_send_byte(0XA1);  // 发送设备地址：读模式
    iic_wait_ack();
    temp = iic_read_byte(0);  // 接收一个字节数据
    iic_stop();               // 停止条件
    return temp;
}

// 在AT24C02指定地址写入一个数据
void at24c02_write_one_byte(uint16_t addr, uint8_t data) {
    iic_start();          // 发送起始信号
    iic_send_byte(0XA0);  // 发送设备地址：写模式
    iic_wait_ack();
    iic_send_byte(addr);  // 发送字节地址（0-255）
    iic_wait_ack();
    iic_send_byte(data);  // 发送数据
    iic_wait_ack();
    iic_stop();    // 停止条件
    delay_ms(10);  // EEPROM写入需要延时
}

// 检测AT24C02是否正常（先在末地址写一个值再读取一个值）
uint8_t at24c02_check(void) {
    uint8_t temp;
    uint16_t addr = EE_TYPE;
    temp          = at24c02_read_one_byte(addr);

    if (temp == 0X55) {
        return 0;
    } else  // 首次初始化
    {
        at24c02_write_one_byte(addr, 0X55);  // 先写入数据
        temp = at24c02_read_one_byte(255);   // 再读取数据

        if (temp == 0X55)
            return 0;
    }

    return 1;
}

// 从AT24C02指定地址开始读出指定个数的数据
void at24c02_read(uint16_t addr, uint8_t* pbuf, uint16_t datalen) {
    while (datalen--) {
        *pbuf++ = at24c02_read_one_byte(addr++);
    }
}

// 在AT24C02指定地址开始写入指定个数的数据
void at24c02_write(uint16_t addr, uint8_t* pbuf, uint16_t datalen) {
    while (datalen--) {
        at24c02_write_one_byte(addr, *pbuf);
        addr++;
        pbuf++;
    }
}
