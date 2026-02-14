

#include "myiic.h"

#include <stdint.h>

#include "delay.h"

// 初始化IIC
void iic_init(void) {
    GPIO_InitTypeDef gpio_init_struct;

    IIC_SCL_GPIO_CLK_ENABLE();
    IIC_SDA_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin   = IIC_SCL_GPIO_PIN;
    gpio_init_struct.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull  = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(IIC_SCL_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin  = IIC_SDA_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;  // 开漏输出
    HAL_GPIO_Init(IIC_SDA_GPIO_PORT, &gpio_init_struct);

    iic_stop();
}

// 延时函数,用于控制IIC读写速度
static void iic_delay(void) {
    delay_us(2);  // 读写速度在250Khz以内
}

// 产生起始信号
void iic_start(void) {
    IIC_SDA(1);
    IIC_SCL(1);
    iic_delay();
    IIC_SDA(0);  // 当SCL为高时, SDA从高变成低, 表示起始信号
    iic_delay();
    IIC_SCL(0);
    iic_delay();
}

// 产生停止信号
void iic_stop(void) {
    IIC_SDA(0);  // 当SCL为高时, SDA从低变成高, 表示停止信号
    iic_delay();
    IIC_SCL(1);
    iic_delay();
    IIC_SDA(1);
    iic_delay();
}

// 等待应答
uint8_t iic_wait_ack(void) {
    uint8_t waittime = 0;
    uint8_t rack     = 0;

    IIC_SDA(1);  // 主机释放SDA线(此时外部器件可以拉低SDA线)
    iic_delay();
    IIC_SCL(1);  // SCL=1, 此时从机可以返回ACK
    iic_delay();

    while (IIC_READ_SDA) {
        waittime++;

        if (waittime > 250) {
            iic_stop();
            rack = 1;
            break;
        }
    }

    IIC_SCL(0);  // SCL=0, 结束ACK检查
    iic_delay();
    return rack;
}

// 产生ACK应答
void iic_ack(void) {
    IIC_SDA(0);  // SCL 0 -> 1  时 SDA = 0,表示应答
    iic_delay();
    IIC_SCL(1);
    iic_delay();
    IIC_SCL(0);
    iic_delay();
    IIC_SDA(1);  // 主机释放SDA线
    iic_delay();
}

// 不产生ACK应答
void iic_nack(void) {
    IIC_SDA(1);  // SCL 0 -> 1  时 SDA = 1,表示不应答
    iic_delay();
    IIC_SCL(1);
    iic_delay();
    IIC_SCL(0);
    iic_delay();
}

// 发送一个字节
void iic_send_byte(uint8_t data) {
    uint8_t t;

    for (t = 0; t < 8; t++) {
        IIC_SDA((data & 0x80) >> 7);
        iic_delay();
        IIC_SCL(1);
        iic_delay();
        IIC_SCL(0);
        data <<= 1;
    }
    IIC_SDA(1);  // 主机释放SDA线
}

// 主机读取一个字节
uint8_t iic_read_byte(uint8_t ack) {
    uint8_t i, receive = 0;

    for (i = 0; i < 8; i++) {
        receive <<= 1;
        IIC_SCL(1);
        iic_delay();

        if (IIC_READ_SDA) {
            receive++;
        }

        IIC_SCL(0);
        iic_delay();
    }

    if (!ack) {
        iic_nack();
    } else {
        iic_ack();
    }

    return receive;
}
