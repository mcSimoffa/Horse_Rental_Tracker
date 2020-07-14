#define DEBUG_MODE
#include "stm32f10x_dma_dihalt.h"
#include "stm32F10x_usart1.h"

#define BUFFER_SIZE         512		//circle buffer size
#define ALLMOST_FULL        200
#define SUBSTRING_SEPARATOR	0x0A

char     Buffer[BUFFER_SIZE];
uint16_t	head;	
uint16_t tail;  
uint16_t	substringHead;


uint16_t OringBuf_GetFullness()
{
  if(head<tail) return (BUFFER_SIZE+head-tail);
  return (head-tail);	
}

void OringBuf_Init()
{
  head=tail=substringHead=0;
}

void OringBuf_Clear()
{
  tail=head;
  substringHead=head;
}

void USART1_IRQHandler()
{ 
  uint32_t SR_state;
  uint8_t incomingByte;
  SR_state=USART1->SR;
  
  //Receive Error recognized
  if ((SR_state & (USART_SR_FE | USART_SR_NE | USART_SR_ORE))!=0)
  {
    ITM_SendChar((uint32_t)"\0");
    incomingByte=(uint8_t) USART1->DR;
  }
  //new Byte received
  if ((SR_state & USART_SR_RXNE)!=0)
  {
    incomingByte=(uint8_t) USART1->DR;
#ifdef DEBUG_MODE
    ITM_SendChar((uint32_t)incomingByte);
#endif    
    Buffer[head]=incomingByte;
  if (OringBuf_GetFullness()<BUFFER_SIZE-1)
  {
    head++;
    if (head>=BUFFER_SIZE)
      head -= BUFFER_SIZE;
  }
  if (OringBuf_GetFullness()>=ALLMOST_FULL)
    substringHead=head;
  if (incomingByte == SUBSTRING_SEPARATOR)
    substringHead=head; 
  }
}

void SendStringUsart1WithDMA(const char *AdresOfString,uint16_t quantil)
{    
  DMA_Disable(DMA1_Channel4);
  DMA_Set_Mem_Addr(DMA1_Channel4,(uint32_t)AdresOfString);
  DMA_Set_Size(DMA1_Channel4,quantil);
  DMA1->IFCR = DMA_IFCR_CTCIF4;
  USART1->SR &= ~USART_SR_TC;
  DMA_Enable (DMA1_Channel4);
}

void GetParam(uint16_t *_tail, uint16_t *_head, uint16_t *subhead)
{
 *_tail=tail;
 *_head=head;
 *subhead=substringHead;
 return;
}
uint16_t USART_GetArray(char *Dest, uint8_t ExpectedLen)
{
  uint16_t CopyHead;
  uint16_t copy=0;
  uint16_t Safetail;
  if (OringBuf_GetFullness()>=ExpectedLen)
  {
    CopyHead=tail+ExpectedLen;
    if (CopyHead>=BUFFER_SIZE)
     CopyHead -= BUFFER_SIZE;
  }
  else 
    return (0);
  
  if (tail>CopyHead)
    {
      Safetail=tail;
      while (tail<BUFFER_SIZE) 
       {
         *Dest=Buffer[tail];
         tail++;
         Dest++;
       }
     copy=tail-Safetail;
     if (tail>=BUFFER_SIZE)
       tail -= BUFFER_SIZE;
    }
  
    if (tail<CopyHead)
    {
      Safetail=tail;
      while (tail<CopyHead) 
       {
         *Dest=Buffer[tail];
         tail++;
         Dest++;
       }
     copy += (tail-Safetail);
    }
  return (copy);
}

uint16_t USART_GetLine(char *Dest)
{
  uint16_t CopyHead;
  uint16_t copy=0;
  uint16_t Safetail;
  char content=0;
  uint16_t LastSublenth=(head<substringHead) ? (BUFFER_SIZE+head-substringHead):(head-substringHead);	
  
  
  if (OringBuf_GetFullness()>LastSublenth)
    CopyHead=substringHead;
   else  return (0); 
   
  if (CopyHead<tail)
    {
      Safetail=tail;
      while (tail<BUFFER_SIZE) 
       {
         content=Buffer[tail];
         *Dest=content;
         tail++;
         Dest++;
         if (content==SUBSTRING_SEPARATOR)       break;
       }
     copy=tail-Safetail;
     if (tail>=BUFFER_SIZE)
       tail -= BUFFER_SIZE;
     *Dest=0;	//Zero terminator
    }
  
    if ((tail<CopyHead) & (content!=SUBSTRING_SEPARATOR))
    {
      Safetail=tail;
      while (tail<CopyHead) 
       {
         content=Buffer[tail];
         *Dest=content;
         tail++;
         Dest++;
         if (content==SUBSTRING_SEPARATOR)         break;
       }
     copy += (tail-Safetail);
     *Dest=0;
    }
  return (copy);
}
