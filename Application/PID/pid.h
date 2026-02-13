#ifndef __PID_H
#define __PID_H
#include "sys.h"

typedef struct {
    float Kp, Ki, Kd;
    float p_out, i_out, d_out;
    int error, last_error, last_error2, last_error3, last_error4, last_error5;
    int output;

} PID_TypeDef;

int pid(int present, uint16_t target, PID_TypeDef* PID);  // 三个参数：当前位置，目标位置，PID结构体
int better_pid(int present, uint16_t target, PID_TypeDef* PID);
void pid_init(float Kp, float Ki, float Kd, PID_TypeDef* PID);  // 4个参数：Kp,Ki,Kd,PID结构体
void pid_set_x(uint32_t kp, uint32_t ki, uint32_t kd);
void pid_set_y(uint32_t kp, uint32_t ki, uint32_t kd);

#endif