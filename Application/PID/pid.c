#include "pid.h"

#include <stdint.h>

#include "24c02.h"

extern PID_TypeDef PID_x;
extern PID_TypeDef PID_y;

/* EEPROM地址定义 */
#define PID_X_KP_ADDR 0  /* X轴Kp: 0-3 */
#define PID_X_KI_ADDR 4  /* X轴Ki: 4-7 */
#define PID_X_KD_ADDR 8  /* X轴Kd: 8-11 */
#define PID_Y_KP_ADDR 12 /* Y轴Kp: 12-15 */
#define PID_Y_KI_ADDR 16 /* Y轴Ki: 16-19 */
#define PID_Y_KD_ADDR 20 /* Y轴Kd: 20-23 */
// 0、PID初始化函数，给各参数赋值
// 参数(4个)：Kp，Ki，Kd，处理的PID结构体的地址
void pid_init(float Kp, float Ki, float Kd, PID_TypeDef* PID) {
    PID->Kp          = Kp;
    PID->Ki          = Ki;
    PID->Kd          = Kd;
    PID->error       = 0;
    PID->last_error  = 0;
    PID->last_error2 = 0;
    PID->last_error3 = 0;
    PID->last_error4 = 0;
    PID->last_error5 = 0;
    PID->p_out       = 0;
    PID->i_out       = 0;
    PID->d_out       = 0;
    PID->output      = 0;
}

// 1、位置PID
int pid(int present, uint16_t target, PID_TypeDef* PID) {
    PID->error = target - present;  // 本次误差 = 目标值 - 实际值

    PID->p_out = PID->Kp * PID->error;                      // 比例
    PID->i_out += PID->Ki * PID->error;                     // 积分
    PID->d_out = PID->Kd * (PID->error - PID->last_error);  // 微分

    PID->output = PID->p_out + PID->i_out + PID->d_out;  // 输出

    PID->last_error = PID->error;

    return PID->output;
}

// 2、改进版位置PID(对微分项进行改善，考虑历史信息，降噪)
int better_pid(int present, uint16_t target, PID_TypeDef* PID) {
    PID->error = target - present;  // 本次误差 = 目标值 - 实际值

    PID->p_out = PID->Kp * PID->error;   // 比例
    PID->i_out += PID->Ki * PID->error;  // 积分
    PID->d_out = PID->Kd * 1 / 16 *
                 (PID->error + 3 * PID->last_error + 2 * PID->last_error2 - 2 * PID->last_error3 -
                  3 * PID->last_error4 - PID->last_error5);  // 微分

    PID->output = PID->p_out + PID->i_out + PID->d_out;  // 输出

    PID->last_error5 = PID->last_error4;  // 上次误差 = 本次误差
    PID->last_error4 = PID->last_error3;  // 上次误差 = 本次误差
    PID->last_error3 = PID->last_error2;  // 上次误差 = 本次误差
    PID->last_error2 = PID->last_error;   // 上次误差 = 本次误差
    PID->last_error  = PID->error;        // 上次误差 = 本次误差

    return PID->output;
}

void pid_set_x(uint32_t kp, uint32_t ki, uint32_t kd) {
    pid_init(kp / 1000.0f, ki / 1000.0f, kd / 1000.0f, &PID_x);
    pid_save_to_eeprom(); /* 保存EEPROM */
}

void pid_set_y(uint32_t kp, uint32_t ki, uint32_t kd) {
    pid_init(kp / 1000.0f, ki / 1000.0f, kd / 1000.0f, &PID_y);
    pid_save_to_eeprom(); /* 保存EEPROM */
}

/* 从24C02读取保存的PID参数 */
void pid_load_from_eeprom(void) {
    uint8_t buf[4];
    uint32_t kp_x, ki_x, kd_x;
    uint32_t kp_y, ki_y, kd_y;

    /* 读取X轴PID参数 */
    at24c02_read(PID_X_KP_ADDR, buf, 4);
    kp_x = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

    at24c02_read(PID_X_KI_ADDR, buf, 4);
    ki_x = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

    at24c02_read(PID_X_KD_ADDR, buf, 4);
    kd_x = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

    /* 读取Y轴PID参数 */
    at24c02_read(PID_Y_KP_ADDR, buf, 4);
    kp_y = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

    at24c02_read(PID_Y_KI_ADDR, buf, 4);
    ki_y = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

    at24c02_read(PID_Y_KD_ADDR, buf, 4);
    kd_y = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

    /* 检查参数是否有效（不是0xFFFFFFFF,表示未旧） */
    if (kp_x != 0xFFFFFFFF && ki_x != 0xFFFFFFFF && kd_x != 0xFFFFFFFF) {
        pid_init(kp_x / 1000.0f, ki_x / 1000.0f, kd_x / 1000.0f, &PID_x);
        printf("[PID] Load X from EEPROM: Kp=%d Ki=%d Kd=%d\r\n", kp_x, ki_x, kd_x);
    }

    if (kp_y != 0xFFFFFFFF && ki_y != 0xFFFFFFFF && kd_y != 0xFFFFFFFF) {
        pid_init(kp_y / 1000.0f, ki_y / 1000.0f, kd_y / 1000.0f, &PID_y);
        printf("[PID] Load Y from EEPROM: Kp=%d Ki=%d Kd=%d\r\n", kp_y, ki_y, kd_y);
    }
}

/* 保存当前PID参数到24C02 */
void pid_save_to_eeprom(void) {
    uint8_t buf[4];
    uint32_t kp_x, ki_x, kd_x;
    uint32_t kp_y, ki_y, kd_y;

    /* 将浮点参数转为整数 */
    kp_x = (uint32_t)(PID_x.Kp * 1000);
    ki_x = (uint32_t)(PID_x.Ki * 1000);
    kd_x = (uint32_t)(PID_x.Kd * 1000);

    kp_y = (uint32_t)(PID_y.Kp * 1000);
    ki_y = (uint32_t)(PID_y.Ki * 1000);
    kd_y = (uint32_t)(PID_y.Kd * 1000);

    /* 保存X轴PID参数 */
    buf[0] = (kp_x >> 24) & 0xFF;
    buf[1] = (kp_x >> 16) & 0xFF;
    buf[2] = (kp_x >> 8) & 0xFF;
    buf[3] = kp_x & 0xFF;
    at24c02_write(PID_X_KP_ADDR, buf, 4);

    buf[0] = (ki_x >> 24) & 0xFF;
    buf[1] = (ki_x >> 16) & 0xFF;
    buf[2] = (ki_x >> 8) & 0xFF;
    buf[3] = ki_x & 0xFF;
    at24c02_write(PID_X_KI_ADDR, buf, 4);

    buf[0] = (kd_x >> 24) & 0xFF;
    buf[1] = (kd_x >> 16) & 0xFF;
    buf[2] = (kd_x >> 8) & 0xFF;
    buf[3] = kd_x & 0xFF;
    at24c02_write(PID_X_KD_ADDR, buf, 4);

    /* 保存Y轴PID参数 */
    buf[0] = (kp_y >> 24) & 0xFF;
    buf[1] = (kp_y >> 16) & 0xFF;
    buf[2] = (kp_y >> 8) & 0xFF;
    buf[3] = kp_y & 0xFF;
    at24c02_write(PID_Y_KP_ADDR, buf, 4);

    buf[0] = (ki_y >> 24) & 0xFF;
    buf[1] = (ki_y >> 16) & 0xFF;
    buf[2] = (ki_y >> 8) & 0xFF;
    buf[3] = ki_y & 0xFF;
    at24c02_write(PID_Y_KI_ADDR, buf, 4);

    buf[0] = (kd_y >> 24) & 0xFF;
    buf[1] = (kd_y >> 16) & 0xFF;
    buf[2] = (kd_y >> 8) & 0xFF;
    buf[3] = kd_y & 0xFF;
    at24c02_write(PID_Y_KD_ADDR, buf, 4);

    printf("[PID] Save to EEPROM: X(%.3f,%.3f,%.3f) Y(%.3f,%.3f,%.3f)\r\n", PID_x.Kp, PID_x.Ki, PID_x.Kd, PID_y.Kp,
           PID_y.Ki, PID_y.Kd);
}

/* \u4ece24C02\u8bfb\u53d6\u4fdd\u5b58\u7684PID\u53c2\u6570 */
void pid_load_from_eeprom(void) {
    uint8_t buf[4];
    uint32_t kp_x, ki_x, kd_x;
    uint32_t kp_y, ki_y, kd_y;

    /* \u8bfb\u53d6X\u8f74PID\u53c2\u6570 */\n at24c02_read(PID_X_KP_ADDR, buf, 4);
    \n kp_x = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    \n    \n at24c02_read(PID_X_KI_ADDR, buf, 4);
    \n ki_x = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    \n    \n at24c02_read(PID_X_KD_ADDR, buf, 4);
    \n kd_x = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    \n    \n /* \u8bfb\u53d6Y\u8f74PID\u53c2\u6570 */\n at24c02_read(PID_Y_KP_ADDR, buf, 4);
    \n kp_y = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    \n    \n at24c02_read(PID_Y_KI_ADDR, buf, 4);
    \n ki_y = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    \n    \n at24c02_read(PID_Y_KD_ADDR, buf, 4);
    \n kd_y = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    \n    \n    /* \u68c0\u67e5\u53c2\u6570\u662f\u5426\u6709\u6548\uff08\u4e0d\u662f0xFFFFFFFF,\u8868\u793a\u672a\u65e7 */\n    if (kp_x != 0xFFFFFFFF && ki_x != 0xFFFFFFFF && kd_x != 0xFFFFFFFF) {
        \n pid_init(kp_x / 1000.0f, ki_x / 1000.0f, kd_x / 1000.0f, &PID_x);\n        printf(\"[PID] Load X from EEPROM: Kp=%d Ki=%d Kd=%d\\r\\n\", kp_x, ki_x, kd_x);\n    }\n    \n    if (kp_y != 0xFFFFFFFF && ki_y != 0xFFFFFFFF && kd_y != 0xFFFFFFFF) {\n        pid_init(kp_y / 1000.0f, ki_y / 1000.0f, kd_y / 1000.0f, &PID_y);\n        printf(\"[PID] Load Y from EEPROM: Kp=%d Ki=%d Kd=%d\\r\\n\", kp_y, ki_y, kd_y);\n    }\n}\n\n/* \u4fdd\u5b58\u5f53\u524dPID\u53c2\u6570\u523024C02 */\nvoid pid_save_to_eeprom(void) {\n    uint8_t buf[4];\n    uint32_t kp_x, ki_x, kd_x;\n    uint32_t kp_y, ki_y, kd_y;\n    \n    /* \u4f1a\u8bda\u53c2\u65702\u4e008\u6574\u6570 */\n    kp_x = (uint32_t)(PID_x.Kp * 1000);\n    ki_x = (uint32_t)(PID_x.Ki * 1000);\n    kd_x = (uint32_t)(PID_x.Kd * 1000);\n    \n    kp_y = (uint32_t)(PID_y.Kp * 1000);\n    ki_y = (uint32_t)(PID_y.Ki * 1000);\n    kd_y = (uint32_t)(PID_y.Kd * 1000);\n    \n    /* \u4fdd\u5b58X\u8f74PID\u53c2\u6570 */\n    buf[0] = (kp_x >> 24) & 0xFF;\n    buf[1] = (kp_x >> 16) & 0xFF;\n    buf[2] = (kp_x >> 8) & 0xFF;\n    buf[3] = kp_x & 0xFF;\n    at24c02_write(PID_X_KP_ADDR, buf, 4);\n    \n    buf[0] = (ki_x >> 24) & 0xFF;\n    buf[1] = (ki_x >> 16) & 0xFF;\n    buf[2] = (ki_x >> 8) & 0xFF;\n    buf[3] = ki_x & 0xFF;\n    at24c02_write(PID_X_KI_ADDR, buf, 4);\n    \n    buf[0] = (kd_x >> 24) & 0xFF;\n    buf[1] = (kd_x >> 16) & 0xFF;\n    buf[2] = (kd_x >> 8) & 0xFF;\n    buf[3] = kd_x & 0xFF;\n    at24c02_write(PID_X_KD_ADDR, buf, 4);\n    \n    /* \u4fdd\u5b58Y\u8f74PID\u53c2\u6570 */\n    buf[0] = (kp_y >> 24) & 0xFF;\n    buf[1] = (kp_y >> 16) & 0xFF;\n    buf[2] = (kp_y >> 8) & 0xFF;\n    buf[3] = kp_y & 0xFF;\n    at24c02_write(PID_Y_KP_ADDR, buf, 4);\n    \n    buf[0] = (ki_y >> 24) & 0xFF;\n    buf[1] = (ki_y >> 16) & 0xFF;\n    buf[2] = (ki_y >> 8) & 0xFF;\n    buf[3] = ki_y & 0xFF;\n    at24c02_write(PID_Y_KI_ADDR, buf, 4);\n    \n    buf[0] = (kd_y >> 24) & 0xFF;\n    buf[1] = (kd_y >> 16) & 0xFF;\n    buf[2] = (kd_y >> 8) & 0xFF;\n    buf[3] = kd_y & 0xFF;\n    at24c02_write(PID_Y_KD_ADDR, buf, 4);\n    \n    printf(\"[PID] Save to EEPROM: X(%.3f,%.3f,%.3f) Y(%.3f,%.3f,%.3f)\\r\\n\", \n           PID_x.Kp, PID_x.Ki, PID_x.Kd, PID_y.Kp, PID_y.Ki, PID_y.Kd);\n}"
