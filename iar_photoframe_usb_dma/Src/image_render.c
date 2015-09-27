#include "image_render.h"
#include "icon_names.h"
#include "line_icon.h"
#include "main.h"
#include <stdint.h>

extern weather_type current_weather;
extern weather_type weather_forecast[3];
extern weather_type balcon_weather;

//width and height calculated in update_picture_sizes()
picture_struct picture_array[] = {
                    {(unsigned char*)&line_icon,14,77,1,0,0},//line //0
                    {(unsigned char*)&line_icon,14,292,1,0,0},//line //1
                    {(unsigned char*)&line_icon,14,640,1,0,0},//line //2
                    {(unsigned char*)&error_icon,40,306,1,0,0},//1day //3
                    {(unsigned char*)&error_icon,244,306,1,0,0},//2day //4
                    {(unsigned char*)&error_icon,448,306,1,0,0},//3day //5
                    {(unsigned char*)&error_icon,448,104,1,0,0},//now //6
		    {NULL}
};

//update images according currend situation
void update_images(void)
{
  change_icon_by_code(weather_forecast[0].icon_code, &picture_array[3]);
  change_icon_by_code(weather_forecast[1].icon_code, &picture_array[4]);
  change_icon_by_code(weather_forecast[2].icon_code, &picture_array[5]);
  change_icon_by_code(current_weather.icon_code, &picture_array[6]);
  
  update_picture_sizes();
}

void change_icon_by_code(uint8_t code, picture_struct *image)
{
  switch (code)
  {
    case 1: {image->image = (unsigned char*)&sun_icon; break;}
    case 2: {image->image = (unsigned char*)&few_clouds_icon; break;}
    case 3: {image->image = (unsigned char*)&scattered_clouds_icon; break;}
    case 4: {image->image = (unsigned char*)&broken_clouds_icon; break;}
    case 9: {image->image = (unsigned char*)&shower_rain_icon; break;}
    case 10: {image->image = (unsigned char*)&rain_icon; break;}
    case 11: {image->image = (unsigned char*)&thunderstorm_icon; break;}
    case 13: {image->image = (unsigned char*)&snow_icon; break;}
    case 50: {image->image = (unsigned char*)&few_clouds_icon; break;}//must be mist
    default: {image->image = (unsigned char*)&error_icon; break;}
  }
}


uint8_t get_picture_data(unsigned char *picture, uint16_t x, uint16_t y, uint16_t width)
{
  uint16_t pic_x = x/8;
  uint8_t pic_shift = 7 - x%8;
  
  uint16_t pic_pos = y*(width/8) + pic_x + 4;//4 - image size shift
  
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
//dest - pointer to place data
//sector - sector number (from begin of file)
//0 sector - shorter then other sectors - it contain BMP_OFFSET bytes of header
void picture_prosess_sector(uint8_t *dest,uint32_t sector, int len)
{
  uint16_t first_line = 0;//number of line in main bmp image
  uint16_t last_line = 0;//number of line in main bmp image
  uint16_t line_offset = 0;//offset in main bmp image
  uint8_t i;
  uint16_t first_line_inv = 0;//инверси€
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
  
  first_line = BMP_HEIGHT - last_line_inv-1;//перерасчет инверсии - (0 - сама€ перва€ лини€ изображени€)
  last_line = BMP_HEIGHT - first_line_inv-1;//перерасчет инверсии - (0 - сама€ перва€ лини€ изображени€)
  
  for (i = 0; picture_array[i].image != NULL; i++)//Check every picture
  {
    picture_x_end = picture_array[i].x + picture_array[i].width - 1;//Horizontal end of picture
    picture_y_end = picture_array[i].y + picture_array[i].height - 1;//Horizontal end of picture ??
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
      
      for (dest_cnt=0;dest_cnt<len;dest_cnt++)//prosess all pixels in sector
      {
        if ((virtual_x >= picture_array[i].x) && (virtual_x <= picture_x_end) && (virtual_y >= picture_array[i].y) && (virtual_y <= picture_y_end))
        {
          *dest = get_picture_data(picture_array[i].image,(virtual_x - picture_array[i].x) ,(virtual_y - picture_array[i].y),picture_array[i].width);
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



