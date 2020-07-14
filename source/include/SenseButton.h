#ifndef _STM32F10x_CSENSOR_H_
#define _STM32F10x_CSENSOR_H_
#define MAX_CAP		1000

#include "stm32f10x.h"
#include "stm32F10x_gpioMY.h"

typedef struct 
{
  GPIO_TypeDef *GPIOx;
  uint8_t SensorPin;
  uint8_t StoragePin;
  uint16_t cap;
} CsensorParamDef;

void touch_Discahrge (CsensorParamDef *Param);
uint8_t touch_processing (CsensorParamDef *Param);
#endif //_STM32F10x_CSENSOR_H_