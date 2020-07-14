#include "stm32F10x_gpioMY.h"
#include "Cfunction.h"
#include <stdlib.h>
#include "main.h"

void removeSpaces(char *str1)  
{
char *str2; 
str2=str1;  
do
  if (*str1 != ' ')  *(str2++) = *str1;
while (*(str1++) !=0);
}


//------------------------------------------------------------
char *itoa(int16_t n, char *s)
{
  char c;   //for reverse
  int16_t sign;
  char *pBeginStr=s;
  if ((sign = n) < 0)
    n = -n; 
  *(s++)='\0';
  do 
  { 
    *s++ = n % 10 + '0'; 
  } while ((n /= 10) > 0);
  if (sign < 0)     *s++ = '-';
  char *p=s;    //end of string pointer before reverse
  p--;
  char *retVal=--s;
  s=pBeginStr;
  do
  {
    c = *s;
    *s++ = *p;
    *p-- = c;
  } while (p-s>0);
  return (retVal);
}

/*------------------------------------------------------------
*pStorage - pointer on Area for CRC calculation
Size - lenth area in Bytes
return CRC
*/
uint32_t CRCcalc( void *pStorage, uint16_t Size)
{
uint32_t *ptr = (uint32_t *)pStorage;
if ((Size & 0xFFFC)!=Size) return (0);
RCC->AHBENR |= RCC_AHBENR_CRCEN;  // CRC on
CRC->CR = 1; // reset CRC
for (uint16_t i=0;i<Size;i+=4)
  CRC->DR = *ptr++;
return (CRC->DR);
}

//------------------------------------------------------------
void debugprint (const char *toSWO)
{
 while ( *toSWO!=0)
  ITM_SendChar((uint32_t)*(toSWO++));
 return; 
}

char *stradd (char * destptr, const char * srcptr )
{
  do
    *destptr++ = *srcptr;
  while (*(srcptr++)!=0x00);
  return (--destptr);
}

void strrepl(char *src, char fnd, char rep)
{
  do
  {
    if (*src == fnd)
      *src = rep;
  } while (*(src++) != 0x00);
  
}

void HardFault_Handler()
{
  while (1)
    GPIO_RESET(GPIOC,1<<LED_13_PIN);
}


