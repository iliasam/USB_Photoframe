#ifndef _IMAGE_RENDER
#define _IMAGE_RENDER
#include <stdint.h>

typedef struct {
	unsigned char *image;
	uint16_t x;
	uint16_t y;
	uint8_t visible;
	uint16_t width;
	uint16_t height;
} picture_struct;

uint8_t get_picture_data(unsigned char *picture, uint16_t x, uint16_t y, uint16_t width);
void draw_line(uint8_t *dest,uint8_t picture_code,uint16_t picture_offset,uint16_t line_offset, uint16_t len);
void picture_prosess_sector(uint8_t *dest,uint32_t sector, int len);
void update_picture_sizes(void);
void change_icon_by_code(uint8_t code, picture_struct *image);
void update_images(void);

#endif
