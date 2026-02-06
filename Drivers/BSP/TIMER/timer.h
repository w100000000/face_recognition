#ifndef __TIMER_H
#define __TIMER_H

#include "sys.h"

extern TIM_HandleTypeDef htim3;

void TIM3_Int_Init(uint16_t arr, uint16_t psc);
void TIM3_PWM_Init(uint16_t arr, uint16_t psc);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim);

#endif