

/* Includes ------------------------------------------------------------------*/
#include "usart.h"

#include "gpio.h"


UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_uart_rx;

void Init_DMA_UART3_RX(void)
{
  /* DMA controller clock enable */
  __DMA1_CLK_ENABLE();
  
  //HAL_DMA_DeInit(&hdma_uart_rx);
   
  /* Peripheral DMA init*/
  hdma_uart_rx.Instance = DMA1_Channel3;
  
  hdma_uart_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_uart_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_uart_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_uart_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_uart_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_uart_rx.Init.Mode = DMA_NORMAL;
  hdma_uart_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  HAL_DMA_Init(&hdma_uart_rx);
  
  SET_BIT(huart3.Instance->CR3, USART_CR3_DMAR); /* Enable the UART DMA Rx request */
}

//important: after caling this function HAL lock DMA channel
//DMA_UART3_Wait_And_Stop must be called to unlock
void DMA_UART3_Prepare_To_Receive(uint8_t *dma_rx_buffer, uint16_t size)
{
  //CLEAR_BIT(huart3.Instance->SR, USART_SR_RXNE);//clear rx not empty
  //uint8_t tmp_data = huart3.Instance->DR;
  HAL_DMA_Start(&hdma_uart_rx, (uint32_t)&(huart3.Instance->DR),(uint32_t)dma_rx_buffer,(uint32_t)size);//dma enabled here
  CLEAR_BIT(huart3.Instance->SR, USART_SR_RXNE);//clear rx not empty
  uint8_t tmp_data = huart3.Instance->DR;
}

void DMA_UART3_Wait_And_Stop(uint16_t Timeout)
{
  HAL_Delay(Timeout);
  HAL_DMA_Abort(&hdma_uart_rx);
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
  
  Init_DMA_UART3_RX();
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
//#warning "Compiler generates FPU instructions for a device without an FPU (check __FPU_PRESENT)"

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