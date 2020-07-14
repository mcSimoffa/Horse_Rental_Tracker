#include "telemetry.h"
#include "main.h"
#include "SenseButton.h"
#include "Cfunction.h"
#include "FreeRTOS.h"
#include "task.h"
#include "time.h"

#define VALIDATE_WEIGHT_PULSE   50
sLogElem arrayLog[LOG_SIZE];
uint8_t headArrayLog;
  
/*----------------------------------------------------------*/
uint16_t middleValue (uint16_t _value, uint16_t smaArray[])
{
  uint16_t _sum = 0;
  for (uint8_t i=1;i<MIDDLE_MODUL;i++)
  {
    smaArray[i-1] = smaArray[i];
    _sum += smaArray[i];
  }
  smaArray[MIDDLE_MODUL-1] =  _value;
   _sum += _value;
  return (_sum/MIDDLE_MODUL);
}

uint8_t ArrayLog_GetHead()
{return (headArrayLog);}

sLogElem * ArrayLog_GetRecord (uint8_t recordnum)
{return ( &arrayLog[recordnum]);}
  
/*----------------------------------------------------------*/
void vTelemetry (void *pvParameters)
{
  uint8_t RiderState=0;  //,бит 0 наличия всадника сейчас (1 со,  0 без ) бит 1 - предыдущее состояние. Сдвинут
  TickType_t xLastWakeTime = xTaskGetTickCount();
  uint32_t  prevMinute=RTC_GetCounter()/60;
  uint16_t heavyCounter = 0;
  uint16_t freeCounter = 0;
  uint16_t smaArrayA[MIDDLE_MODUL], smaArrayB[MIDDLE_MODUL];
  uint16_t PauseTick;
  uint32_t lastHeavyTime;
  
  //--- главный цикл потока
  CsensorParamDef SensorA, SensorB;
  SensorA.GPIOx=SENSOR_A_PORT;
  SensorA.SensorPin=SENSOR_PIN_A;
  SensorA.StoragePin=STORAGE_PIN_A;
  SensorB.GPIOx=SENSOR_B_PORT;
  SensorB.SensorPin=SENSOR_PIN_B;
  SensorB.StoragePin=STORAGE_PIN_B;
  headArrayLog=0;
  while (flagSysParamRead ==0)
    vTaskDelay(1);
  PauseTick = SysParam.FreeSensorTime*10;
  do
  {
    touch_Discahrge(&SensorA);  //разрядка емкости датчика
    touch_Discahrge(&SensorB);
    vTaskDelay(1);
    taskENTER_CRITICAL();       //измерение
     touch_processing (&SensorA);
     touch_processing (&SensorB);
    taskEXIT_CRITICAL();
    if ((SensorA.cap < SysParam.SensorA_without - SysParam.Sensordelta) || (SensorB.cap < SysParam.SensorB_without - SysParam.Sensordelta))
    {
      heavyCounter++;
      freeCounter = 0;
      lastHeavyTime = RTC_GetCounter();
    }
    else
      freeCounter++;
    
    if (freeCounter > PauseTick) //седло пустое больше порогового числа тиков ? Считаем что всадник встал
    {
      if (RiderState == 1)
      {
        RiderState = 0x00;
        arrayLog[headArrayLog].duration = lastHeavyTime - arrayLog[headArrayLog].unixTime;
        if (++headArrayLog >= LOG_SIZE)
        headArrayLog -= LOG_SIZE;
      }
      heavyCounter = 0;
      freeCounter = 0;
      PauseTick = SysParam.FreeSensorTime*10;
    }
    
    if (heavyCounter > VALIDATE_WEIGHT_PULSE )    //всадник насидел больше порогового числа тиков? Считаем что он уселся
    {
      if (RiderState == 0)
      {
        RiderState = 0x01;
        arrayLog[headArrayLog].unixTime = RTC_GetCounter();
      }
     heavyCounter = 0;
    }

       
 //анализ обрыва и замыкания проводов датчика   
    smaValueA = middleValue (SensorA.cap, smaArrayA);
    smaValueB = middleValue (SensorB.cap, smaArrayB);
    if ((smaValueA > SENSOR_A_MAX_VALUE) || (smaValueB> SENSOR_B_MAX_VALUE))
      deviceFlags |= BROKEN_SENSOR;
    else
      deviceFlags &= ~BROKEN_SENSOR;
    
    /*char scapA[8];
    char CrLf[]="\r\n";
    itoa (SensorA.cap, (char *)&scapA);
    debugprint(scapA);
    debugprint(CrLf);*/
    vTaskDelayUntil (&xLastWakeTime,25);    //100ms cycle repeat
  }
   while (1);
  
}
//(GPIO_READ_OUTPUT(GPIOC,1<<LED_13_PIN)==0) ?  GPIO_SET(GPIOC,1<<LED_13_PIN):GPIO_RESET(GPIOC,1<<LED_13_PIN);
//vTaskDelay(250);CR_DBP_BB
//itoa (SensorA.cap, (char *)&scapA);
    //debugprint(scapA);
    //debugprint(CrLf);
    // vTaskDelay(100);

void RTC_SetCounter(uint32_t CounterValue)
{ 
 PWR->CR   |= PWR_CR_DBP;
 while ((RTC->CRL & RTC_CRL_RTOFF) == 0)   //wait for last task
      asm("NOP");
  RTC->CRL  |= RTC_CRL_CNF;//Enter configuration mode
  RTC->CNTH = CounterValue >> 16;
  RTC->CNTL = (CounterValue & (uint32_t)0x0000FFFF);
  RTC->CRL &= (uint16_t)~((uint16_t)RTC_CRL_CNF);//exit configuration mode
  while ((RTC->CRL & RTC_CRL_RTOFF) == 0)   //wait for last task
      asm("NOP");
  PWR->CR &= ~PWR_CR_DBP;
  return;
}

uint32_t RTC_GetCounter(void)
{
  uint16_t tmp = 0;
  RTC->CRL &= ~RTC_CRL_SECF; // clear SECF;
  do
  {
    tmp = RTC->CNTH;
    tmp <<=16;
    tmp |= RTC->CNTL;
  } while(RTC->CRL & RTC_CRL_SECF);
  tmp = RTC->CNTL;
  return (((uint32_t)RTC->CNTH << 16 ) | tmp) ;
}

  