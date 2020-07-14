#ifndef _TELEMETRY_H_
#define _TELEMETRY_H_
#include "stm32f10x.h"

#define MIDDLE_MODUL    5
#define LOG_SIZE        10

#define SENSOR_A_MIN_VALUE      55
#define SENSOR_B_MIN_VALUE      55

#define SENSOR_A_MAX_VALUE      125
#define SENSOR_B_MAX_VALUE      125


typedef struct 
{
  uint32_t  unixTime;
  uint16_t  duration;
} sLogElem;


void vTelemetry (void *pvParameters);
uint32_t RTC_GetCounter(void);
void RTC_SetCounter(uint32_t CounterValue);
uint8_t ArrayLog_GetHead();
sLogElem * ArrayLog_GetRecord (uint8_t recordnum);

#endif//_TELEMETRY_H_