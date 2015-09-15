#include "image_render.h"
#include "test7_image.h"
#include "test6_image.h"
#include "main.h"
#include <stdint.h>

//first 4 bytes of picture is it's size

//width and height calculated in update_picture_sizes()
//pointer,X,Y,visible,width,height
picture_struct picture_array[] = {
		{&test7_image,10,500,1,0,0},//sun
		{&test6_image,250,536,1,0,0},//lines
		{NULL}
};

//get information about single pixel of image (X,Y)
uint8_t get_picture_data(unsigned char *picture, uint16_t x, uint16_t y)
{
	uint16_t pic_x = x/8;
	uint8_t pic_shift = 7 - x%8;

	uint16_t pic_pos = y*(128/8) + pic_x + 4;//4 - image size shift

	if ((picture[pic_pos] & (1<<pic_shift)) != 0)
	{
		return 0;//black
	}
	else
	{
		return BACK_COLOR;
	}
}


//used for show pictures
//dest - pointer to plaqce data
//sector - sector number (from begin of file)
//0 sector - shorter of other sectors - it contain BMP_OFFSET bytes of header
void picture_prosess_sector(uint8_t *dest,uint32_t sector, int len)
{
	uint16_t first_line = 0;//number of line in main bmp image
	uint16_t last_line = 0;//number of line in main bmp image
	uint16_t line_offset = 0;//offset in main bmp image
	uint8_t i;
	uint16_t first_line_inv = 0;//inverted
	uint16_t last_line_inv = 0;

	uint16_t dest_cnt = 0;
	uint16_t virtual_x = 0;
	uint16_t virtual_y = 0;

	uint16_t picture_x_end = 0;
	uint16_t picture_y_end = 0;

	uint8_t *tmp_pointer = dest;

	if (sector != 0)
	{
		first_line_inv = (uint16_t)((sector*(uint32_t)SECTOR_SIZE - (uint32_t)BMP_OFFSET)/(uint32_t)BMP_WIDTH);//number of read line of image (0 - last line of image)
		last_line_inv = (uint16_t)(((sector+1)*(uint32_t)SECTOR_SIZE - (uint32_t)BMP_OFFSET)/(uint32_t)BMP_WIDTH);//number of read line of image (0 - last line of image)
		line_offset = (uint16_t)((sector*(uint32_t)SECTOR_SIZE - (uint32_t)BMP_OFFSET) % (uint32_t)BMP_WIDTH);
	}
	else {len = len - BMP_OFFSET;}//0 sector

	first_line = BMP_HEIGHT - last_line_inv-1;//перерасчет инверсии - (0 - самая первая линия изображения) //calculate inversion
	last_line = BMP_HEIGHT - first_line_inv-1;//перерасчет инверсии - (0 - самая первая линия изображения) //calculate inversion

	for (i = 0; picture_array[i].image != NULL; i++)//Check every picture
	{
		picture_x_end = picture_array[i].x + picture_array[i].width - 1;//Horizontal end of picture
		picture_y_end = picture_array[i].y + picture_array[i].height - 1;//Horizontal end of picture
		dest = tmp_pointer;

		if ((last_line < picture_array[i].y) || (first_line > picture_array[i].y + picture_array[i].height-1))//image lines out of picture
		{
		}
		else
		{
			//picture must be shown
			//нужно показать картинку
			virtual_x = line_offset;//image coordinates (not picture coordinates)
			virtual_y = last_line;

			for (dest_cnt=0;dest_cnt<len;dest_cnt++)//process all pixels in sector
			{
				if ((virtual_x >= picture_array[i].x) && (virtual_x <= picture_x_end) && (virtual_y >= picture_array[i].y) && (virtual_y <= picture_y_end))
				{
					*dest = get_picture_data(picture_array[i].image,(virtual_x - picture_array[i].x) ,(virtual_y - picture_array[i].y));
				}
				dest++;//next sector byte

				virtual_x++;
				if (virtual_x >= BMP_WIDTH){virtual_x = 0;virtual_y--;}//next image line
			}

		}
	}
}


//read header of every picture and fill picture_array
void update_picture_sizes(void)
{
	uint8_t i;
	for (i = 0; picture_array[i].image != NULL; i++)//Check every picture
	{
		picture_array[i].width = picture_array[i].image[0] + picture_array[i].image[1]*256;
		picture_array[i].height = picture_array[i].image[2] + picture_array[i].image[3]*256;
	}
}



