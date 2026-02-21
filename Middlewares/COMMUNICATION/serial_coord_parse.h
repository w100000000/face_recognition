#ifndef __SERIAL_COORD_PARSE_H
#define __SERIAL_COORD_PARSE_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"

typedef struct {
    int16_t x;
    int16_t y;
    uint32_t timestamp;
    char name[32];
    uint8_t valid;
} FaceData_t;

extern QueueHandle_t qFaceData;

extern char g_recognized_name[32];  // 存储识别到的人名

void recieveData(void);

#endif
