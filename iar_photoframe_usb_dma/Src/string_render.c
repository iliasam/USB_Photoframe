#include "string_render.h"
#include "main.h"
#include <mcufont.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

extern weather_type current_weather;
extern weather_type weather_forecast[3];
extern weather_type balcon_weather;

#define BUFFER_SIZE (uint16_t)(BUFFER_HEIGHT*BMP_WIDTH)

const struct mf_font_s *font;
options_t options;
state_t state;
int font_height = 48;

unsigned char text_render_array[BUFFER_SIZE];//buffer for text

unsigned char tmp_cnt = 0;

extern uint16_t update_counter;//test 

static const char default_text[] = "MY test >ii< >IIi<\n";

char err_msg[25];

char tmp_msg[25] = "test";


test_struct text_array[] = {
		    {"—ейчас\n",44,25,1},
		    {"ѕрогноз\n",44,240,1},
                    {"Ѕалкон\n",44,588,1},
                    {(char*)&err_msg,250,25,1},
		    {NULL},{NULL},{NULL},{NULL},{NULL},{NULL},{NULL},{(char*)&tmp_msg,400,520,0},//last for test
                    {NULL},{NULL},{NULL},{NULL},{NULL},{NULL}
};


uint16_t buffer_start_line = 0;
uint16_t buffer_stop_line = 0;

//update strings according currend situation
void update_text(void)
{
   draw_temperatute(22,430, weather_forecast[0].temperature1,&text_array[4]);//1 day
   draw_temperatute(223,430, weather_forecast[1].temperature1,&text_array[5]);//2 day
   draw_temperatute(430,430, weather_forecast[2].temperature1,&text_array[6]);//3 day
   draw_temperatute(57,136, current_weather.temperature1,&text_array[7]);//now
   draw_temperatute(57,684, balcon_weather.temperature1,&text_array[8]);//balcon - temp_out
   draw_temperatute(400,684, balcon_weather.temperature2,&text_array[9]);//balcon2
   
   //draw_temperatute(400,570, tmp_cnt,&text_array[10]);//test   
   //tmp_cnt++;//test
}

void update_err_message(wifi_status_type status)
{
  switch (status)
  {
  case MODULE_NOT_FOUND: {strcpy((char*)&err_msg,"NO WiFi");break;}
  case MODULE_BUSY: {strcpy((char*)&err_msg,"WiFi BUSY");break;}
  case NO_AP_CONNECTION: {strcpy((char*)&err_msg,"WiFi no AP");break;}
  case WEATHER_FAIL: {strcpy((char*)&err_msg,"Weather Fail");break;}
  case WEATHER_NO_CONNECTION: {strcpy((char*)&err_msg,"Weather no conn");break;}
  case FORECAST_FAIL: {strcpy((char*)&err_msg,"Foresast Fail");break;}
  case FORECAST_NO_CONNECTION: {strcpy((char*)&err_msg,"Foresast no conn");break;}
  case MAJORDOMO_FAIL: {strcpy((char*)&err_msg,"Home fail");break;}
  case MAJORDOMO_NO_CONNECTION: {strcpy((char*)&err_msg,"Home no conn");break;}
  default: {break;}
  }
  if (status < GOOD_RESULT) {text_array[3].visible = 1;} else {text_array[3].visible = 0;}//only errors visible
  
  //sprintf((char*)&tmp_msg,"%d\n",update_counter);//test
}

//update temperature in given position
void draw_temperatute(uint16_t x,uint16_t y, int8_t temperature, test_struct *text)
{
  if (text->text == NULL)
  {
    text->text = malloc(8);
  }
  text->visible = 1;
  text->x = x;
  text->y = y;

  if (temperature >= 0){sprintf((char*)text->text,"+%d%cC\n",temperature,176);}//176 - deg symbol code  
  else
  {
    if (temperature < -50){sprintf((char*)text->text,"NO\n");}//error
    else sprintf((char*)text->text,"%d%cC\n",temperature,176);//minus
  }
}

//render functions
//*********************************************************************************
//*********************************************************************************

//used to show text
//dest - pointer to plaqce data
//sector - sector number (from begin of file)
//0 sector - shorter of other sectors - it contain BMP_OFFSET bytes of header
void string_prosess_sector(uint8_t *dest,uint32_t sector, int len)
{
  uint16_t first_line_inv = 0;//inversion (buffer is not inverted)
  uint16_t last_line_inv = 0;
  uint16_t first_line = 0;
  uint16_t last_line = 0;
  uint16_t line_offset = 0;//offset between begin of current image line and begin of current sector in bytes
  uint16_t buffer_offset = 0;
  uint16_t len2 = 0;
  
  if (sector != 0)
  {
    
    first_line_inv = (uint16_t)((sector*(uint32_t)SECTOR_SIZE - (uint32_t)BMP_OFFSET)/(uint32_t)BMP_WIDTH);//number of read line of image (0 - last line of image)
    last_line_inv = (uint16_t)(((sector+1)*(uint32_t)SECTOR_SIZE - (uint32_t)BMP_OFFSET)/(uint32_t)BMP_WIDTH);//number of read line of image (0 - last line of image)
    line_offset = (uint16_t)((sector*(uint32_t)SECTOR_SIZE - (uint32_t)BMP_OFFSET) % (uint32_t)BMP_WIDTH);
  }
  else {len = len - BMP_OFFSET;}//0 sector
  
  first_line = BMP_HEIGHT - last_line_inv-1;//calculate inversion - 0 - first line of image
  last_line = BMP_HEIGHT - first_line_inv-1;//calculate inversion - 0 - first line of image
  
  if ((first_line > buffer_stop_line) || (first_line <= buffer_start_line))//if current line was not send to buffer
  {
    draw_strings(first_line);
  }
  else if ((last_line > buffer_stop_line) || (last_line <= buffer_start_line))//if current line was not send to buffer
  {
    draw_strings(last_line-1);
  }
  
  //вычисление начала нужных данных в буфере и их копирование
  //calculate begin of needed data in buffer and copy data to dest
  
  //если весь сектор укладываетс€ в линию
  //if whole sector belong to one line
  if (line_offset <= (BMP_WIDTH - (uint16_t)len))
  {
    buffer_offset = (first_line - buffer_start_line)*BMP_WIDTH + line_offset;
    
    if (buffer_offset > (uint32_t)BUFFER_SIZE){memset(dest, 3, len);}//error
    else {memcpy(dest, &state.buffer[buffer_offset], len);}
  }
  else
  {
    //number off bytes in first part
    //число байтов в первой части
    uint16_t data_length = BMP_WIDTH - line_offset;
    buffer_offset = (last_line - buffer_start_line)*BMP_WIDTH + line_offset;
    
    if (data_length < len) {len2 = data_length;} else {len2 = len;}
    
    if (buffer_offset > (uint32_t)BUFFER_SIZE){memset(dest, 3, len);return;}//error
    else {memcpy(dest, &state.buffer[buffer_offset], len2);}
    
    dest+=data_length;
    
    //number off bytes in second part
    //число байтов во второй части
    data_length = len - data_length;
    buffer_offset = (last_line-1 - buffer_start_line)*BMP_WIDTH;
    
    len2 = data_length;
    if (buffer_offset > (uint32_t)BUFFER_SIZE){memset(dest, 3, len2);return;}//error
    else {memcpy(dest, &state.buffer[buffer_offset], len2);}
  }
}

//если видима€ область одной из строк попадает в отображаемую область буфера,
//то она отрисовываетс€
//то есть данна€ функци€ не всегда отрисовывает строки
//cur_line - начало буфера

//if visible part of one text strings belong to visible part of buffer
//then this string rendering
//cur_line - will be begin of the buffer
void draw_strings(uint16_t cur_line)
{
  uint8_t i;
  
  static uint8_t buffer_clean = 0;
  
  if (buffer_clean != 1) {clean_buffer();buffer_clean = 1;}//очистить буфер, если он заполнен
  for (i = 0; text_array[i].text != NULL; i++)//Check every string
  {
    if ((text_array[i].y >= (cur_line- font_height)) && (text_array[i].y < (cur_line + font_height)) && (text_array[i].visible != 0))//если строка попадает в область буфера
    {
      state.options->margin = text_array[i].x;
      state.options->anchor = text_array[i].x;
      state.y = (text_array[i].y - cur_line);
      options.text = text_array[i].text;
      
      mf_wordwrap(font, options.width, options.text, line_callback, &state);
      buffer_clean = 0;// в буфере по€вились данные //new data appear in buffer
    }
  }
  //пересчитываютс€ параметры отображаемой области буфера
  //recalculate parameters of visible part of buffer
  buffer_start_line = cur_line;
  buffer_stop_line = cur_line+BUFFER_HEIGHT-1;
}



/* Callback to write to a memory buffer. */
void pixel_callback(int16_t x, int16_t y, uint8_t count, uint8_t alpha,
                           void *state)
{
  state_t *s = (state_t*)state;
  uint32_t pos;
  int16_t value;
  
  if (y < 0 || y >= s->height) return;
  if (x < 0 || x + count >= s->width) return;
  
  while (count--)
  {
    pos = (uint32_t)s->width * y + x;
    value = s->buffer[pos];
    value -= alpha;
    if (value < 0) value = 0;
    s->buffer[pos] = value;
    
    x++;
  }
}

/* Callback to render characters. */
uint8_t character_callback(int16_t x, int16_t y, mf_char character,
                                  void *state)
{
    state_t *s = (state_t*)state;
    return mf_render_character(s->font, x, y, character, pixel_callback, state);
    //see: scaled_render_character
}

/* Callback to render lines. */
bool line_callback(const char *line, uint16_t count, void *state)
{
    state_t *s = (state_t*)state;


    mf_render_aligned(s->font, s->options->anchor, s->y,
                      s->options->alignment, line, count,
                      character_callback, state);

    s->y += s->font->line_height;
    return true;
}

void clean_buffer(void)
{
  memset(state.buffer, BACK_COLOR, BUFFER_SIZE);
}

void init_string_render(void)
{
  options.fontname = mf_get_font_list()->font->short_name;
  options.text = default_text;
  options.width = BMP_WIDTH;
  options.scale = 1;
  options.alignment = MF_ALIGN_LEFT;
  font = mf_find_font(options.fontname);
  
  options.margin = 50;
  options.anchor = options.margin;
  
  state.options = &options;
  state.width = options.width;
  state.height = font_height;
  state.y = (uint16_t)(0);
  state.font = font;
  state.buffer = (uint8_t*)&text_render_array;
  
  // mf_wordwrap(font, options.width, options.text, line_callback, &state);
  
  clean_buffer();
  draw_strings(0);
}

