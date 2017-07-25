#ifndef __WIFI_H
#define __WIFI_H

#include <stdint.h>
#include "usart.h"

typedef struct
{
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
} TimeType;

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

wifi_status_type update_wifi_data(void);

wifi_status_type get_weather(void);
wifi_status_type get_weather_forecast(void);
wifi_status_type get_majordomo_temperature(void);
wifi_status_type get_majordomo_temperature2(void);

void get_current_time(uint8_t *str);

void update_current_weather(uint8_t *data, uint16_t length);
void update_forecast(uint8_t *data, uint16_t length);
void update_majordomo_temperature(uint8_t *data, uint16_t length, int8_t *temperature);

void enable_wifi(void);
void disable_wifi(void);
void reset_wifi(void);

#endif 
