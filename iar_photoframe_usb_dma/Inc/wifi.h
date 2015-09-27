#ifndef __WIFI_H
#define __WIFI_H

#include <stdint.h>
#include "usart.h"
#include <stdint.h>

typedef enum
{
  MODULE_NOT_FOUND = 0,//"bad" results first
  MODULE_BUSY,
  MODULE_FOUND,
  NO_AP_CONNECTION,
  WEATHER_FAIL,
  WEATHER_NO_CONNECTION,
  FORECAST_FAIL,
  FORECAST_NO_CONNECTION,
  MAJORDOMO_FAIL,
  MAJORDOMO_NO_CONNECTION,
  GOOD_RESULT,//this only for comparation !!!
  AP_CONNECTED,
  WEATHER_RECEIVED,
  FORECAST_RECEIVED,
  MAJORDOMO_RECEIVED,
  FUNC_DONE,
  ALL_DONE
} wifi_status_type;

typedef wifi_status_type (*get_wifi_data_callback_t) (void);


wifi_status_type auto_update_data(get_wifi_data_callback_t callback, uint8_t num);

wifi_status_type test_esp_work(void);
wifi_status_type test_ap_connection(void);
void wifi_close_connection(void);

wifi_status_type update_wifi_data(void);

wifi_status_type get_weather(void);
wifi_status_type get_weather_forecast(void);
wifi_status_type get_majordomo_temperature(void);
wifi_status_type get_majordomo_temperature2(void);

void update_current_weather(uint8_t *data, uint16_t length);
void update_forecast(uint8_t *data, uint16_t length);
void update_majordomo_temperature(uint8_t *data, uint16_t length, int8_t *temperature);

int16_t find_substring(uint8_t *array, uint16_t arr_length, char *str, uint16_t str_length);
long find_numeric_value(uint8_t *data);
uint8_t isdigit_l(uint8_t _C);

void enable_wifi(void);
void disable_wifi(void);
void reset_wifi(void);

#endif 
