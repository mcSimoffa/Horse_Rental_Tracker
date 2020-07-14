#include <string.h>
#include "OringBuf.h"
#include "SysTick.h"

#define BUFFER_SIZE         512		//circle buffer size
#define SUBSTRING_SEPARATOR	0x0A
#define TIME_OUT_RX         500    //0.5 sec Таймаут ожидания символа конца подстроки при приеме

char     Buffer[BUFFER_SIZE];
uint16_t	head;		//Голова (пустая еще ячейка куда запишется следующий байт)
uint16_t tail;           //Хвост (первая не прочитанная ячейка)
uint16_t	substringHead;  //Голова подстроки (первая ячейка новой построки)

sDelayData delayTimeOut;

//выдает длину между головой и указанным параметрок как хвостом
uint16_t OringBuf_GetFullness( uint16_t _tail)
{
  if(head<_tail) return (BUFFER_SIZE+head-_tail);
  return (head-_tail);	
}


void OringBuf_Init()
{
  head=tail=substringHead=0;
}

//запись байта в голову буффера. Вызывается в прерывании USART1
void OringBuf_Put(uint8_t dataByte)
{ 
  Buffer[head]=dataByte;
  //проверка переполнения буффера
  if (OringBuf_GetFullness(tail)<BUFFER_SIZE-1)
  {
    head++;
    if (head>=BUFFER_SIZE)
      head -= BUFFER_SIZE;
  }
  //встретился разделитель
  if (dataByte == SUBSTRING_SEPARATOR)
  {
    substringHead=head;    //начало подстроки
    NonblockDelay_Stop(&delayTimeOut);  //отменить таймаут незавершенной строки
  }
  else
    NonblockDelay_Start(&delayTimeOut,TIME_OUT_RX);//установить таймаут незавершенной строки
}

//пишет массив байт от хвоста до головы подстроки substringHead
//возвращает количество скопированных байт
uint16_t GetString(char *Dest)
{
  uint16_t SafesubstringHead;
  uint16_t copy=0;
  uint8_t Safetail;
  char content;
 
  do
  {
  __LDREXH(&SafesubstringHead);
    if (NonblockDelay_IsEnd(&delayTimeOut)==1)
      substringHead=head;   //выдать незафинализированную подстроку  ;
  } while(__STREXH(substringHead,&SafesubstringHead)==1);

  if (SafesubstringHead<tail)
    {
      Safetail=tail;
      while (tail<BUFFER_SIZE) 
       {
         content=Buffer[tail];
         *Dest=content;
         tail++;
         Dest++;
         if(content==SUBSTRING_SEPARATOR)        break;
       }
     copy=tail-Safetail;
     if (tail>=BUFFER_SIZE)
       tail -= BUFFER_SIZE;
     *Dest=0;	//Zero terminator
    }
  
    if (SafesubstringHead>tail) 
    {
      Safetail=tail;
      while (tail<SafesubstringHead) 
       {
         content=Buffer[tail];
         *Dest=content;
         tail++;
         Dest++;
         if(content==SUBSTRING_SEPARATOR)        break;
       }
     copy += (tail-Safetail);
     *Dest=0;	//Zero terminator
    }
          
    
  return (copy);
}
