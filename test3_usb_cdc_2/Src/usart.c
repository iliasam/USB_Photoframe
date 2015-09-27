
/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "gpio.h"
#include "usbd_def.h"

#define DMA_RX_BUFFER_LENGTH 64
uint8_t dma_rx_buffer[DMA_RX_BUFFER_LENGTH];




LINE_CODING linecoding_def =
  {
    115200, // baud rate
    0x00,   // stop bits-1
    0x00,   // parity - none
    0x08    // nb. of bits 8
  };



UART_HandleTypeDef huart3;
 DMA_HandleTypeDef hdma_uart_rx;

//move received bytes from dma buffer to buf
uint16_t take_received_bytes(uint8_t* buf)
{
  static uint16_t prev_count = DMA_RX_BUFFER_LENGTH;
  uint16_t cur_pos = hdma_uart_rx.Instance->CNDTR;
  uint16_t received_bytes_count = 0;
  if (prev_count >= cur_pos)//count decriment
  {
    uint16_t offset = (uint16_t)DMA_RX_BUFFER_LENGTH - prev_count;
    received_bytes_count = prev_count - cur_pos;
    prev_count = cur_pos;
    memcpy(buf, &dma_rx_buffer[offset], received_bytes_count);
    
  }
  else
  {
    //zero cross
    uint16_t offset = (uint16_t)DMA_RX_BUFFER_LENGTH - prev_count;
    received_bytes_count = prev_count;
    memcpy(buf, &dma_rx_buffer[offset], received_bytes_count);
    buf+= received_bytes_count;
    received_bytes_count = (uint16_t)DMA_RX_BUFFER_LENGTH - cur_pos;
    memcpy(buf, &dma_rx_buffer, received_bytes_count);
    received_bytes_count = (uint16_t)DMA_RX_BUFFER_LENGTH + prev_count - cur_pos;//total
    prev_count = cur_pos;
  }
  
  return received_bytes_count;
}


void UART_Send_Data(uint8_t* Buf, uint16_t Len)
{
  uint16_t i;
  for (i=0;i<Len;i++)
  {
    huart3.Instance->DR = *(Buf + i);
    
    while (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_TXE) == RESET){}//TX empty
  }
}

void Init_DMA_UART3_RX(void)
{
 
  /* DMA controller clock enable */
  __DMA1_CLK_ENABLE();
   
  /* Peripheral DMA init*/
  hdma_uart_rx.Instance = DMA1_Channel3;
  
  hdma_uart_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_uart_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_uart_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_uart_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_uart_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_uart_rx.Init.Mode = DMA_CIRCULAR;
  hdma_uart_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  HAL_DMA_Init(&hdma_uart_rx);
  
  HAL_DMA_Start(&hdma_uart_rx, (uint32_t)&(huart3.Instance->DR),(uint32_t)dma_rx_buffer,DMA_RX_BUFFER_LENGTH);
    
  /* Enable the UART DMA Rx request */
  SET_BIT(huart3.Instance->CR3, USART_CR3_DMAR);
    
   // __HAL_DMA_ENABLE()
}


/* USART3 init function */
void MX_USART3_UART_Init(void)
{

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart3);

}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(huart->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */
    /* Peripheral clock enable */
    __USART3_CLK_ENABLE();
  
    /**USART3 GPIO Configuration    
    PC10     ------> USART3_TX
    PC11     ------> USART3_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    __HAL_AFIO_REMAP_USART3_PARTIAL();

  /* USER CODE BEGIN USART3_MspInit 1 */

  /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{

  if(huart->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __USART3_CLK_DISABLE();
  
    /**USART3 GPIO Configuration    
    PC10     ------> USART3_TX
    PC11     ------> USART3_RX 
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11);

  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
  }
} 




/**
  * @brief  VCP_COMConfig
  *         Configure the COM Port with default values or values received from host.
  * @param  Conf: can be DEFAULT_CONFIG to set the default configuration or OTHER_CONFIG
  *         to set a configuration received from the host.
  * @retval None.
  */
uint16_t VCP_COMConfig(uint8_t Conf, LINE_CODING linecoding)
{
  if (Conf == DEFAULT_CONFIG)  
  {
    /*
    - BaudRate = 115200 baud  
    - Word Length = 8 Bits
    - One Stop Bit
    - Parity Odd
    - Hardware flow control disabled
    - Receive and transmit enabled
    */
    
   MX_USART3_UART_Init();
    
    
    /* Enable the USART Receive interrupt */
    //USART_ITConfig(EVAL_COM1, USART_IT_RXNE, ENABLE);
  }
  else
  {
    /* set the Stop bit*/
    switch (linecoding.format)
    {
    case 0:
      huart3.Init.StopBits = UART_STOPBITS_1;
      break;
    case 1:
      huart3.Init.StopBits = UART_STOPBITS_1;
      break;
    case 2:
      huart3.Init.StopBits = UART_STOPBITS_2;
      break;
    default :
      VCP_COMConfig(DEFAULT_CONFIG,linecoding_def);
      return (USBD_FAIL);
    }
    
    /* set the parity bit*/
    switch (linecoding.paritytype)
    {
    case 0:
      huart3.Init.Parity = UART_PARITY_NONE;
      break;
    case 1:
      huart3.Init.Parity = UART_PARITY_EVEN;
      break;
    case 2:
      huart3.Init.Parity = UART_PARITY_ODD;
      break;
    default :
      VCP_COMConfig(DEFAULT_CONFIG,linecoding_def);
      return (USBD_FAIL);
    }
    
    /*set the data type : only 8bits and 9bits is supported */
    switch (linecoding.datatype)
    {
    case 0x07:
      /* With this configuration a parity (Even or Odd) should be set */
      huart3.Init.WordLength = UART_WORDLENGTH_8B;
      break;
    case 0x08:
      if (huart3.Init.Parity == UART_PARITY_NONE)
      {
        huart3.Init.WordLength = UART_WORDLENGTH_8B;
      }
      else 
      {
        huart3.Init.WordLength = UART_WORDLENGTH_9B;
      }
      
      break;
    default :
      VCP_COMConfig(DEFAULT_CONFIG,linecoding_def);
      return (USBD_FAIL);
    }
    
    huart3.Init.BaudRate = linecoding.bitrate;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    
    /* Configure and enable the USART */
    HAL_UART_Init(&huart3);
  }
  return USBD_OK;
}





/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
