#ifndef STM32F10X_DMA_DIHALT_H_
#define STM32F10X_DMA_DIHALT_H_
#include "stm32f10x.h"

#define DMA_ENABLE 			((uint16_t)(1<<0))
#define DMA_DISABLE 			((uint16_t)0)

#define TRANSFER_COMPL_INT_ENABLE 	((uint16_t)(1<<1))
#define TRANSFER_COMPL_INT_DISABLE 	((uint16_t)0)

#define HALF_COMPL_INT_ENABLE 		((uint16_t)1<<2)
#define HALF_COMPL_INT_DISABLE 		((uint16_t)0)

#define TRANSFER_ERROR_INT_ENABLE 	((uint16_t)(1<<3))
#define TRANSFER_ERROR_INT_DISABLE 	((uint16_t)0)

#define READ_FROM_MEMORY 		((uint16_t)(1<<4))
#define READ_FROM_PERIFERIAL 		((uint16_t)0)

#define CIRCULAR_MODE_ENABLE 		((uint16_t)1<<5)
#define CIRCULAR_MODE_DISABLE 		((uint16_t)0)


#define PERIPHERAL_INC_MODE_ENABLE 	((uint16_t)(1<<6))
#define PERIPHERAL_INC_MODE_DISABLE     ((uint16_t)0)

#define MEMORY_INC_MODE_ENABLE 		((uint16_t)(1<<7))
#define MEMORY_INC_MODE_DISABLE 	((uint16_t)0)

#define PERIPHERAL_SIZE_8 		((uint16_t)0)
#define PERIPHERAL_SIZE_16 		((uint16_t)0x0100)
#define PERIPHERAL_SIZE_32 		((uint16_t)0x0200)

#define MEMORY_SIZE_8 			((uint16_t)0)
#define MEMORY_SIZE_16 			((uint16_t)0x0400)
#define MEMORY_SIZE_32 			((uint16_t)0x0800)

#define CHANNEL_PRIOTITY_LOW 		((uint16_t)0)
#define CHANNEL_PRIOTITY_MEDIUM		((uint16_t)(1<<12))
#define CHANNEL_PRIOTITY_HIGH  		((uint16_t)(2<<12))
#define DCHANNEL_PRIOTITY_VERYHIGH 	((uint16_t)(3<<12))

#define MEMORY_2_MEMORY_MODE_ENABLE (1<<14)
#define MEMORY_2_MEMORY_MODE_DISABLE 0

#define DMA_Disable(DMAChannel)(DMAChannel)->CCR &= (uint16_t)(~DMA_CCR1_EN)
#define DMA_Enable(DMAChannel)(DMAChannel)->CCR |= (uint16_t)DMA_CCR1_EN
#define DMA_Set_Size(DMAChannel,bufSize)(DMAChannel)->CNDTR = bufSize
#define DMA_Set_Mem_Addr(DMAChannel,memAddr)(DMAChannel)->CMAR = memAddr

void DMA_Init(DMA_Channel_TypeDef* Channel, uint32_t Perif, uint32_t Mem, uint16_t Size, uint16_t Conf);
uint32_t DMA_GetCurrentDataCounter(DMA_Channel_TypeDef* Channel);
void DMA_DeInit(DMA_Channel_TypeDef* Channel);

#endif /* STM32F10X_DMA_DIHALT_H_ */
