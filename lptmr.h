#ifndef LPTMR_H
#define LPTMR_H

#include "MKL05Z4.h"

void LPTMR_Init(uint16_t count_val);
void LPTMR_SetTime(uint16_t count_val);

#endif