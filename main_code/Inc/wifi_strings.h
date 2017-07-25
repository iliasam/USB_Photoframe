#ifndef _WIFI_STRINGS_H
#define _WIFI_STRINGS_H

#include <stdint.h>
#include "wifi.h"


long find_numeric_value(uint8_t *data);
uint8_t isdigit_l(uint8_t _C);
int16_t find_substring(uint8_t *array, uint16_t arr_length, char *str, uint16_t str_length);

TimeType get_time_from_string(uint8_t *str, uint8_t length);
long find_numeric_value2(uint8_t *data, uint8_t* length);

#endif