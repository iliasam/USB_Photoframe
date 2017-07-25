#include "wifi_strings.h"
#include "wifi.h"
#include <string.h>
#include <stdlib.h>

//**********************************************************************************
//working with strings

//Date: Wed, 08 Mar 2017 10:48:40 GMT
TimeType get_time_from_string(uint8_t *str, uint8_t length)
{
  TimeType result = {0,0,0};
  uint8_t last_num_length = 0;
  
  uint8_t space_cnt = 0;
  uint8_t i = 0;
  const uint8_t time_zone = 3;//>0
  
  //try to found 5'th space symbol
  while ((i < length) && (space_cnt < 5))
  {
    if (str[i] == ' ') space_cnt++;
    i++; 
  }
  if (i == length) return result;//not found
  
  result.hour = (uint8_t)find_numeric_value2(&str[i], &last_num_length);
  i+=last_num_length+1;//number+":"
  
  result.minute = (uint8_t)find_numeric_value2(&str[i], &last_num_length);
  i+=last_num_length+1;
  
  result.second = (uint8_t)find_numeric_value2(&str[i], &last_num_length);
  
  result.hour = result.hour + time_zone;
  if (result.hour > 23) result.hour = result.hour - 24;
  
  return result;
}

uint8_t isdigit_l(uint8_t _C)
{
  if ((_C >= '0' && _C <= '9') || (_C == '-')) return 1; 
  else return 0;
}

//ищет число в тексте, возвращает его значение
long find_numeric_value2(uint8_t *data, uint8_t* length)
{
  long result = 0;
  
  uint8_t i = 0;
  uint8_t start_pos  = 0;
  uint8_t stop_pos  = 0;
  const uint8_t str_length = 20;
  uint8_t number_length = 0;
  
  //find begin of number
  while (i < str_length)
  {
    if (isdigit_l(data[i])) break;
    i++; 
  }
  start_pos = i;  //i - is digit

  //find end of number
  while (i < str_length)
  {
    if (isdigit_l(data[i]) == 0) break; //not a digit
    i++; 
  }
  stop_pos = i;//i - not a digit
  
  number_length = stop_pos - start_pos;
  *length = number_length;//return length
  
  result = strtol((char*)&data[start_pos], 0, 10);//10 - base
  
  return result;
}

//желательно переделать на find_numeric_value2
long find_numeric_value(uint8_t *data)
{
  uint8_t pos = 0;
  long result = 0;
  for (pos = 0;pos<20;pos++)
  {
    if (isdigit_l(data[pos])) break;
  }
  result = strtol((char*)&data[pos], 0, 10);
  
  return result;
}

int16_t find_substring(uint8_t *array, uint16_t arr_length, char *str, uint16_t str_length)
{
  int16_t pos = -1;
  uint16_t i1=0;
  
  if (arr_length < str_length) return -1;//error
  uint16_t end_pos = arr_length - str_length;
  if (end_pos > arr_length) return -1;//error
  
  for (i1=0; i1 < end_pos; i1++)
  {
    if (memcmp((uint8_t*)&array[i1], (uint8_t*)str, str_length) == 0) {pos = (int16_t)i1; break;}
  }
  return pos;
}