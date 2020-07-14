#ifndef _STM32F10x_USART1_H_
#define _STM32F10x_USART1_H_

#define GET_USART1_TC (USART1->SR & USART_SR_TC)
void OringBuf_Init();
void USART1_IRQHandler();
void SendStringUsart1WithDMA(const char *AdresOfString,uint16_t quantil);
uint16_t OringBuf_GetFullness();
void OringBuf_Clear();
uint16_t USART_GetArray(char *Dest, uint8_t ExpectedLen);
uint16_t USART_GetLine(char *Dest);

#endif //_STM32F10x_USART1_H_