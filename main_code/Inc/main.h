#ifndef _MAIN_H
#define _MAIN_H

#include "emfat.h"
#include "wifi.h"

#define SECTOR_SIZE (uint16_t)512

#define IMAGE_SIZE   9114
#define EMPTY_SIZE   20000000

#define BMP_WIDTH    (uint16_t)600
#define BMP_HEIGHT   (uint16_t)800

#define BMP_FULL_SIZE     (int)(1078+(BMP_WIDTH*BMP_HEIGHT))
#define BMP_SIZE          (uint32_t)((uint32_t)BMP_WIDTH*(uint32_t)BMP_HEIGHT)
#define BMP_OFFSET   (uint16_t)54

#define BACK_COLOR_DAY   255 //white
//#define BACK_COLOR_NIGHT   0x82 //gray
//#define BACK_COLOR_NIGHT   0x52 //gray2
#define BACK_COLOR_NIGHT   0x49 //gray2
typedef struct
{
  uint8_t icon_code;
  int8_t temperature1;
  int8_t temperature2;
} weather_type;

void SystemClock_Config(void);

/* Private function prototypes -----------------------------------------------*/
void bmp_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata);
void image_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata);
void empty_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata);
void update_weather_information(void);

void MX_IWDG_Init(void);


#endif
