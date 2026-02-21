#include "serial_coord_parse.h"

#include <stdlib.h>
#include <string.h>

#include "led.h"
#include "sys.h"
#include "usart.h"

uint16_t t;
uint16_t len;
uint16_t times = 0;

extern int coords[2];
char g_recognized_name[32] = "No Face";  // Default display "No Face"

void recieveData(void) {
    uint8_t flag    = 0;
    char strX[6]    = {0};
    char strY[6]    = {0};
    uint8_t cnt_x   = 0;
    uint8_t cnt_y   = 0;
    uint8_t* adress = NULL;

    if (g_usart_rx_sta & 0x8000)  // 如果接收完数据
    {
        LED1_TOGGLE();  // 1号指示灯变更状态

        len                 = g_usart_rx_sta & 0x3FFF;  // 得到此次接收到的数据长度
        FaceData_t faceData = {0};
        faceData.timestamp  = HAL_GetTick();

        // 检查是否是 NAME: 协议
        if (len > 5 && g_usart_rx_buf[0] == 'N' && g_usart_rx_buf[1] == 'A' && g_usart_rx_buf[2] == 'M' &&
            g_usart_rx_buf[3] == 'E' && g_usart_rx_buf[4] == ':') {
            // 解析人名（从第5个字符开始到\r或\n结束）
            uint8_t name_len = 0;
            for (t = 5; t < len && name_len < 31; t++) {
                if (g_usart_rx_buf[t] == '\r' || g_usart_rx_buf[t] == '\n')
                    break;
                g_recognized_name[name_len++] = g_usart_rx_buf[t];
            }
            g_recognized_name[name_len] = '\0';  // 字符串结束符
            printf("Recognized: %s\r\n", g_recognized_name);

            faceData.x     = coords[0];
            faceData.y     = coords[1];
            faceData.valid = 1;
            strncpy(faceData.name, g_recognized_name, 31);
            faceData.name[31] = '\0';
            xQueueOverwrite(qFaceData, (void*)&faceData);
        }
        // 否则按坐标协议解析 #X$Y
        else {
            adress = &g_usart_rx_buf[0];  // 指针adress储存字符地址，从0-len过一遍

            // 根据协议取出坐标的字符形式到数strX和strY中
            for (t = 0; t < len; t++) {
                if (*adress >= '0' && *adress <= '9') {
                    if (flag == 1) {
                        if (cnt_x < (sizeof(strX) - 1)) {
                            strX[cnt_x] = *adress;
                            cnt_x++;
                        }
                    } else {
                        if (cnt_y < (sizeof(strY) - 1)) {
                            strY[cnt_y] = *adress;
                            cnt_y++;
                        }
                    }
                } else {
                    if (*adress == '#')
                        flag = 1;
                    if (*adress == '$')
                        flag = 2;
                }
                adress++;
            }

            // 转换字符串为整型，并存储到全局变量coords中
            coords[0] = atoi(strX);
            coords[1] = atoi(strY);
            printf("Parsed: X=%d Y=%d (strX=%s strY=%s)\r\n", coords[0], coords[1], strX, strY);

            faceData.x     = coords[0];
            faceData.y     = coords[1];
            faceData.valid = 1;
            strncpy(faceData.name, g_recognized_name, 31);
            faceData.name[31] = '\0';
            xQueueOverwrite(qFaceData, (void*)&faceData);
        }

        // 标志位清零
        g_usart_rx_sta = 0;
    } else {
        times++;
        if (times >= 30) {
            times = 0;
            LED0_TOGGLE();  // 闪烁0号指示灯,提示系统正在运行.
        }
    }
}
