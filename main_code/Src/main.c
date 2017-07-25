//Basic code for "USB Photoframe"
//Emulates USB Flash drive whith image at it
//By LIASAM

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"
#include "wifi.h"

#include "main.h"
#include "emfat.h"
#include "string_render.h"
#include "image_render.h"
#include "bmp_header.h"


// экземпляр виртуальной файловой системы FAT32
emfat_t emfat;
const uint32_t bmp_size = BMP_FULL_SIZE;
const uint32_t bmp_width = BMP_WIDTH;
const uint32_t bmp_height = BMP_HEIGHT;

//uint8_t current_back_color = BACK_COLOR_NIGHT;
uint8_t current_back_color = BACK_COLOR_DAY;

IWDG_HandleTypeDef hiwdg;//used for watchdog

// Структура ФС
//Filesystem stucture
static emfat_entry_t entries[] =
{
	// name          dir    lvl offset  size             max_size        user  read               write
	{ "",            true,  0,  0,      0,               0,            0,     NULL,             NULL }, // root
	{ "image.bmp",  false, 1,  0,      BMP_FULL_SIZE,        BMP_FULL_SIZE,      0,     bmp_read_proc,    NULL },
	{ "empty.txt",  false, 1,  0,      EMPTY_SIZE,      EMPTY_SIZE,    0,     empty_read_proc,  NULL },//do not delete
	{ NULL }
};

wifi_status_type current_wifi_state;
uint16_t update_counter = 10000;//time between wifi requests
uint16_t emfat_request_time = 0;//time from last request for sector

extern TimeType last_wifi_time;



int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);//gpio0 = 1 - main mode
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);//reset = 1 - main mode
  
  MX_USART3_UART_Init();
  //MX_IWDG_Init();// не хватает времени
  
  reset_wifi();
  emfat_init(&emfat, "emfat", entries);
  init_string_render();
  update_text();
  update_images();

  MX_USB_DEVICE_Init();
  
  HAL_Delay(3000);

  /* Infinite loop */
  while (1)
  { 
    HAL_Delay(1000);
    update_weather_information();
    update_err_message(current_wifi_state);
    HAL_IWDG_Refresh(&hiwdg);//refresh watchdog
  }
}

void update_weather_information(void)
{
  if (update_counter > 600)//600 seconds
  {
    if (emfat_request_time > 300)//300 ms came from last request for image
    {
      current_wifi_state = update_wifi_data();//request data from servers
      update_err_message(current_wifi_state);
      
      //uptade image backcolor depending from received time
      if ((last_wifi_time.hour < 8) || (last_wifi_time.hour > 21))
        current_back_color = BACK_COLOR_NIGHT;
      else
        current_back_color = BACK_COLOR_DAY;
      
      if (1)
      {
        update_text();
        update_images();
      }
      if (current_wifi_state == ALL_DONE)
      {
        update_counter = 0;
        disable_wifi();//all data successfully updated - powerdown wifi
      }
      else
      {
        HAL_Delay(3000);
      }
    }
  }
  else
  {
    update_counter++;
    if (emfat_request_time > 10000) emfat_request_time = 10000;//protect from overflow
  }
  
  if (update_counter > (600-10)){ enable_wifi();}//power up wifi
}

//callback - called by emfat
//generates BMP file
void bmp_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata)
{
  int len = 0;
  emfat_request_time = 0;
  
  uint32_t image_sector;
  if (offset > BMP_FULL_SIZE) return;
  if (offset + size > BMP_FULL_SIZE)
    len = BMP_FULL_SIZE - offset;
  else
    len = size;
  
  image_sector = (offset/(uint32_t)SECTOR_SIZE )- (uint32_t)2;
  
  if (offset == 0)//header sector number 1
  {
    memcpy(dest, &bmp_header, 512);
    
    memcpy((dest+2), &bmp_size, 4);
    memcpy((dest+18), &bmp_width, 4);
    memcpy((dest+22), &bmp_height, 4);
    image_sector = 0;
  }
  else if (offset == 512){memcpy(dest, &bmp_header[512], 512);image_sector = 0;}//header sector number 2
  else if (offset == 1024)//header sector number 3 + data
  {
    memcpy(dest, &bmp_header[1024], BMP_OFFSET);
    string_prosess_sector(dest+BMP_OFFSET,image_sector,len);
    picture_prosess_sector(dest+BMP_OFFSET,image_sector,len);
    
    //memset(dest, 7, len);
  }
  else
  {
    //memset(dest, 7, len);
    string_prosess_sector(dest,image_sector,len);
    picture_prosess_sector(dest,image_sector,len);
  }
}

void empty_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata)
{
  int len = 0;
  if (offset > EMPTY_SIZE) return;
  if (offset + size > EMPTY_SIZE)
    len = EMPTY_SIZE - offset;
  else
    len = size;
  memset(dest, 65, len);//65 - "A"
  
  emfat_request_time = 0;
}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV5;
  RCC_OscInitStruct.Prediv1Source = RCC_PREDIV1_SOURCE_PLL2;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL2.PLL2State = RCC_PLL2_ON;
  RCC_OscInitStruct.PLL2.PLL2MUL = RCC_PLL2_MUL8;
  RCC_OscInitStruct.PLL2.HSEPrediv2Value = RCC_HSE_PREDIV2_DIV5;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBPLLCLK_DIV3;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

}



/* IWDG init function */
void MX_IWDG_Init(void)
{
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
  hiwdg.Init.Reload = 4095;
  HAL_IWDG_Init(&hiwdg);
  // 1/40khz*256*4095
  
  HAL_IWDG_Start(&hiwdg);
}



#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
