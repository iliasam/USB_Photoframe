#ifndef _MAIN_H
#define _MAIN_H

#include "emfat.h"

#define SECTOR_SIZE (uint16_t)512

#define EMPTY_SIZE   20000000

#define BMP_WIDTH    	(uint16_t)600//width of generated image
#define BMP_HEIGHT   	(uint16_t)800//height of generated image

#define BMP_FULL_SIZE   (int)(1078+(BMP_WIDTH*BMP_HEIGHT))//including header
#define BMP_SIZE        (uint32_t)((uint32_t)BMP_WIDTH*(uint32_t)BMP_HEIGHT)
#define BMP_OFFSET   	(uint16_t)54 //1078-512*2

#define BACK_COLOR   255 //white

void image_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata);
void empty_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata);

#endif
