/******************************************************************************
* процедура измерения ёмкости кнопки, сенсора, датчика итд.
******************************************************************************/
#include "SenseButton.h"

void touch_Discahrge (CsensorParamDef *Param)
{
  //оба входа Open Drain. Соединение на землю
  CLL_GPIO_SetOnePinMode(Param->GPIOx,Param->SensorPin,GPIO_MODE_OUTPUT50_OPEN_DRAIN);
  CLL_GPIO_SetOnePinMode(Param->GPIOx,Param->StoragePin,GPIO_MODE_OUTPUT50_OPEN_DRAIN);
  GPIO_RESET(Param->GPIOx,((1<<Param->SensorPin)|(1<<Param->StoragePin)));
}
uint8_t touch_processing (CsensorParamDef *Param)
{
  GPIO_SET(Param->GPIOx,1<<Param->StoragePin);      //накопительный конденсатор отключаем 
  GPIO_SET(Param->GPIOx,1<<Param->SensorPin);       //подготовка выхода сенсора в "1" для быстрых переключений
  Param->cap=0;
  do
  {
    GPIO_SET(Param->GPIOx,1<<Param->StoragePin);      //накопительный конденсатор отключаем 
    CLL_GPIO_SetOnePinMode(Param->GPIOx,Param->SensorPin,GPIO_MODE_OUTPUT50_PUSH_PULL); //сенсор заряжаем
    CLL_GPIO_SetOnePinMode(Param->GPIOx,Param->SensorPin,GPIO_MODE_INPUT_FLOATING);     //сенсор отключаем
    GPIO_RESET(Param->GPIOx,1<<Param->StoragePin);      //переносим заряд
    if (++Param->cap >= MAX_CAP)
      return (0);
  } while (GPIO_READ_INPUT(Param->GPIOx,1<<Param->SensorPin)==0);
  return (1);
}
        
//CLL_GPIO_SetOnePinMode(GPIOA,10,GPIO_MODE_OUTPUT2_OPEN_DRAIN);