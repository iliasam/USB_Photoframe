#include "main.h"
#include "wifi.h"
#include "usart.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern UART_HandleTypeDef huart3;

weather_type current_weather = {0,-100,-100};
weather_type weather_forecast[3] = {0,-100,-100,0,-100,-100,0,-100,-100};
weather_type balcon_weather = {0,-100,-100};

//update all data from internet
wifi_status_type update_wifi_data(void)
{
  wifi_status_type wifi_status;
  
  uint8_t process_cnt = 0;
  wifi_status = test_esp_work();
  if (wifi_status == MODULE_BUSY) {reset_wifi(); return wifi_status;}
  if (wifi_status == MODULE_NOT_FOUND) return wifi_status;
  wifi_status = test_ap_connection();
  if (wifi_status == MODULE_BUSY) {reset_wifi(); return wifi_status;}
  if (wifi_status == NO_AP_CONNECTION) return wifi_status;
  
  //ready to connect to servers
  HAL_Delay(100);
  wifi_status = auto_update_data(get_weather, 1);
  if (wifi_status > GOOD_RESULT) process_cnt++;

  HAL_Delay(100);
  wifi_status = auto_update_data(get_weather_forecast, 2);
  if (wifi_status > GOOD_RESULT) process_cnt++;
  
  HAL_Delay(100);
  wifi_status = auto_update_data(get_majordomo_temperature, 3);
  if (wifi_status > GOOD_RESULT) process_cnt++;
  
  HAL_Delay(100);
  wifi_status = auto_update_data(get_majordomo_temperature2, 4);
  if (wifi_status > GOOD_RESULT) process_cnt++;

  if (process_cnt == 4) 
  {
    wifi_status = ALL_DONE; 
    auto_update_data(NULL, 0);//command to reset cycle info
  }
  return wifi_status;
}

//this function automaticly calls given function "callback" if it was not called in current cycle earlier
//max num is 7
wifi_status_type auto_update_data(get_wifi_data_callback_t callback, uint8_t num)
{
  static uint8_t process_status = 0;
  wifi_status_type result;
  if (num == 0) {process_status = 0; return ALL_DONE;}//command to reset cycle info - prepare to new cycle
  uint8_t cur_pos = (1<<(num));
  
  if ((process_status & cur_pos) == 0)//function of this number was not called in current cycle earlier 
  {
    result = callback();
    if (result > GOOD_RESULT) process_status|= cur_pos;//this funcltion must not be called in this cycle later
  }
  else
  {
    result = FUNC_DONE;
  }
  
  return result;
}

//request curent weather
wifi_status_type get_weather(void)
{
  uint8_t tx_data[] ="AT+CIPSTART=\"TCP\",\"api.openweathermap.org\",80\r\n";
  uint8_t rx_data[1000];
  int16_t pos = 0;
  
  memset(&rx_data[0], 0, sizeof(rx_data));  
  
  DMA_UART3_Prepare_To_Receive(&rx_data[0], sizeof(rx_data));
  HAL_UART_Transmit(&huart3, &tx_data[0], sizeof(tx_data), 100);
  DMA_UART3_Wait_And_Stop(500);
  
  pos = find_substring(&rx_data[0],60, "CONNECT",7);
  if (pos == -1) return WEATHER_NO_CONNECTION;
  
  //get connection
  uint8_t tx_data2[] = "GET /data/2.5/weather?id=542374&units=metric HTTP/1.0\r\nHost: api.openweathermap.org\r\n\r\n";
  uint8_t tx_data3[] = "AT+CIPSEND=87\r\n";
  HAL_UART_Transmit(&huart3, &tx_data3[0], sizeof(tx_data3), 100);
  HAL_Delay(50);
  DMA_UART3_Prepare_To_Receive(&rx_data[0], sizeof(rx_data));
  HAL_UART_Transmit(&huart3, &tx_data2[0], sizeof(tx_data2), 200);
  DMA_UART3_Wait_And_Stop(1000);
  
  pos = find_substring(&rx_data[0],150, "200 OK",6);
  if (pos == -1) 
  {
    wifi_close_connection();
    return WEATHER_FAIL;
  }
    
  update_current_weather(&rx_data[100], 700);
  
  return WEATHER_RECEIVED;
}



//process recieved data of current weather
void update_current_weather(uint8_t *data, uint16_t length)
{
  int pos;
  
  pos = find_substring(data,length, "icon",4);
  if (pos == -1) return;
  current_weather.icon_code  = (uint8_t)find_numeric_value(&data[pos]);
  
  pos = find_substring(data,length, "temp",4);
  if (pos == -1) return;
  current_weather.temperature1  = (uint8_t)find_numeric_value(&data[pos]);
  current_weather.temperature2 = 0;//not used
}

//request for 3 day weather forecast
wifi_status_type get_weather_forecast(void)
{
  uint8_t tx_data[] ="AT+CIPSTART=\"TCP\",\"api.openweathermap.org\",80\r\n";
  uint8_t rx_data[2100];
  int16_t pos = 0;
  
  memset(&rx_data[0], 0, sizeof(rx_data));  
  DMA_UART3_Prepare_To_Receive(&rx_data[0], sizeof(rx_data));
  HAL_UART_Transmit(&huart3, &tx_data[0], sizeof(tx_data), 100);
  DMA_UART3_Wait_And_Stop(500);
  
  pos = find_substring(&rx_data[0],60, "CONNECT",7);
  if (pos == -1) return FORECAST_NO_CONNECTION;
  
  //get connection
  uint8_t tx_data2[] = "GET /data/2.5/forecast/daily?id=542374,ru&units=metric&cnt=4 HTTP/1.0\r\nHost: api.openweathermap.org\r\n\r\n";
  uint8_t tx_data3[] = "AT+CIPSEND=103\r\n";
  HAL_UART_Transmit(&huart3, &tx_data3[0], sizeof(tx_data3), 100);
  HAL_Delay(50);
  DMA_UART3_Prepare_To_Receive(&rx_data[0], sizeof(rx_data));
  HAL_UART_Transmit(&huart3, &tx_data2[0], sizeof(tx_data2), 200);
  DMA_UART3_Wait_And_Stop(1000);
  
  pos = find_substring(&rx_data[0],150, "200 OK",6);
  if (pos == -1) 
  {
    wifi_close_connection();
    return FORECAST_FAIL;
  }
  update_forecast(&rx_data[100], 1600);
  
  return FORECAST_RECEIVED;
}

//request majordomo server to send weather
wifi_status_type get_majordomo_temperature(void)
{
  uint8_t tx_data[] ="AT+CIPSTART=\"TCP\",\"192.168.1.39\",80\r\n";
  uint8_t rx_data[500];
  int16_t pos = 0;
  
  memset(&rx_data[0], 0, sizeof(rx_data));  
  DMA_UART3_Prepare_To_Receive(&rx_data[0], sizeof(rx_data));
  HAL_UART_Transmit(&huart3, &tx_data[0], sizeof(tx_data), 100);
  DMA_UART3_Wait_And_Stop(500);
  
  pos = find_substring(&rx_data[0],60, "CONNECT",7);
  if (pos == -1) return MAJORDOMO_NO_CONNECTION;
  
  //get connection
  uint8_t tx_data2[] = "GET /objects/?object=Balcon&op=get&p=TempOut HTTP/1.0\r\nHost: 192.168.1.39\r\n\r\n";
  uint8_t tx_data3[] = "AT+CIPSEND=77\r\n";
  HAL_UART_Transmit(&huart3, &tx_data3[0], sizeof(tx_data3), 100);
  HAL_Delay(50);
  DMA_UART3_Prepare_To_Receive(&rx_data[0], sizeof(rx_data));
  HAL_UART_Transmit(&huart3, &tx_data2[0], sizeof(tx_data2), 200);
  DMA_UART3_Wait_And_Stop(1500);
  
  pos = find_substring(&rx_data[0],150, "200 OK",6);
  if (pos == -1) 
  {
    wifi_close_connection();
    return MAJORDOMO_FAIL;
  }
  
  update_majordomo_temperature(&rx_data[pos+30], 400,&balcon_weather.temperature1);
  
  return MAJORDOMO_RECEIVED;
}

//request majordomo server to send weather
wifi_status_type get_majordomo_temperature2(void)
{
  uint8_t tx_data[] ="AT+CIPSTART=\"TCP\",\"192.168.1.39\",80\r\n";
  uint8_t rx_data[500];
  int16_t pos = 0;
  
  memset(&rx_data[0], 0, sizeof(rx_data));  
  DMA_UART3_Prepare_To_Receive(&rx_data[0], sizeof(rx_data));
  HAL_UART_Transmit(&huart3, &tx_data[0], sizeof(tx_data), 100);
  DMA_UART3_Wait_And_Stop(500);
  
  pos = find_substring(&rx_data[0],60, "CONNECT",7);
  if (pos == -1) return MAJORDOMO_NO_CONNECTION;
  
  //get connection
  uint8_t tx_data2[] = "GET /objects/?object=Balcon&op=get&p=Temp2 HTTP/1.0\r\nHost: 192.168.1.39\r\n\r\n";
  uint8_t tx_data3[] = "AT+CIPSEND=75\r\n";
  HAL_UART_Transmit(&huart3, &tx_data3[0], sizeof(tx_data3), 100);
  HAL_Delay(50);
  DMA_UART3_Prepare_To_Receive(&rx_data[0], sizeof(rx_data));
  HAL_UART_Transmit(&huart3, &tx_data2[0], sizeof(tx_data2), 200);
  DMA_UART3_Wait_And_Stop(1500);
  
  pos = find_substring(&rx_data[0],150, "200 OK",6);
  if (pos == -1) 
  {
    wifi_close_connection();
    return MAJORDOMO_FAIL;
  }
  
  update_majordomo_temperature(&rx_data[pos+30], 400, &balcon_weather.temperature2);
  
  return MAJORDOMO_RECEIVED;
}

//process recieved data from majordomo
//save result to temperature
void update_majordomo_temperature(uint8_t *data, uint16_t length, int8_t *temperature)
{
  int pos;
  
  pos = find_substring(data,length, "utf-8",5);
  if (pos == -1) return;
  //balcon_weather.temperature1  = (uint8_t)find_numeric_value(&data[pos+5]);
  *temperature = (uint8_t)find_numeric_value(&data[pos+5]);
}

//process recieved data of 3 day weather forecast
void update_forecast(uint8_t *data, uint16_t length)
{
  int pos;
  uint8_t i;
  
  pos = find_substring(data,length, "city",4);
  if (pos == -1) return;
  
  //skip current day
  pos = pos + find_substring(&data[pos],length, "dt",2);
  if (pos == -1) return;
  pos = pos + find_substring(&data[pos],length, "day",3);
  
  for (i=0;i<3;i++)
  {
    pos = pos + find_substring(&data[pos],length, "dt",2);
    if (pos == -1) return;
    
    pos = pos + find_substring(&data[pos],length, "day",3);
    if (pos == -1) return;
    weather_forecast[i].temperature1  = (int8_t)find_numeric_value(&data[pos]);
    
    pos = pos + find_substring(&data[pos],length, "night",5);
    if (pos == -1) return;
    weather_forecast[i].temperature2  = (int8_t)find_numeric_value(&data[pos]);
    
    pos = pos + find_substring(&data[pos],length, "icon",4);
    if (pos == -1) return;
    weather_forecast[i].icon_code  = (uint8_t)find_numeric_value(&data[pos]);
  }
  return;
}




uint8_t isdigit_l(uint8_t _C)
{
  if ((_C >= '0' && _C <= '9') || (_C == '-')) return 1; 
  else return 0;
}

long find_numeric_value(uint8_t *data)
{
  uint8_t pos = 0;
  long result = 0;
  for (pos = 0;pos<15;pos++)
  {
    if (isdigit_l(data[pos])) break;
  }
  result = strtol((char*)&data[pos], 0, 10);
  
  return result;
}

//проверить наличие подключения
//cheeck connection to AP
wifi_status_type test_ap_connection(void)
{
  uint8_t tx_data[] ="AT+PING=\"192.168.1.1\"\r\n";
  uint8_t rx_data[50];
  int16_t pos = 0;
  
  memset(&rx_data[0], 0, sizeof(rx_data));
 
  DMA_UART3_Prepare_To_Receive(&rx_data[0], sizeof(rx_data));
  HAL_UART_Transmit(&huart3, &tx_data[0], sizeof(tx_data), 100);
  HAL_Delay(100);
  
  pos = find_substring(&rx_data[0], sizeof(rx_data), "busy",2);
  if (pos != -1) 
  {
    DMA_UART3_Wait_And_Stop(0);//stop DMA
    return MODULE_BUSY;
  }
  
  pos = find_substring(&rx_data[8],(sizeof(rx_data)-8), "+",1);//answer always have "AT+" part, but good ping give +XX answer
  if (pos != -1) 
  {
    DMA_UART3_Wait_And_Stop(0);//stop DMA
    return AP_CONNECTED;
  }
  DMA_UART3_Wait_And_Stop(1000);
  pos = find_substring(&rx_data[0], sizeof(rx_data), "timeout",7);
  if (pos != -1) return NO_AP_CONNECTION;
  
  return NO_AP_CONNECTION;
}


//проверить наличие модуля
//cheeck esp8266 presence
wifi_status_type test_esp_work(void)
{
  uint8_t tx_data[4] = "AT\r\n";
  uint8_t rx_data[10];
  int16_t pos = 0;
  
  memset(&rx_data[0], 0, sizeof(rx_data));
 
  DMA_UART3_Prepare_To_Receive(&rx_data[0], 10);
  HAL_UART_Transmit(&huart3, &tx_data[0], 4, 100);
  DMA_UART3_Wait_And_Stop(600);
  
  pos = find_substring(&rx_data[0], sizeof(rx_data), "busy",2);
  if (pos != -1) return MODULE_BUSY;
  
  pos = find_substring(&rx_data[0], sizeof(rx_data), "AT",2);
  if (pos != -1) return MODULE_FOUND;
  
  
  return MODULE_NOT_FOUND;
}

void wifi_close_connection(void)
{
  uint8_t tx_data[] ="AT+CIPCLOSE\r\n";
  HAL_UART_Transmit(&huart3, &tx_data[0], sizeof(tx_data), 100);
  HAL_Delay(300);
}

int16_t find_substring(uint8_t *array, uint16_t arr_length, char *str, uint16_t str_length)
{
  int16_t pos = -1;
  int i;
  for (i=0;i<(arr_length - str_length);i++)
  {
    if (memcmp(&array[i], str, str_length) == 0) {pos = i; break;}
  }
  return pos;
  
}

//reset esp8266
void reset_wifi(void)
{
   disable_wifi();
   HAL_Delay(100);
   enable_wifi();
}

void enable_wifi(void)
{
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);//reset = 1 - main mode
}

void disable_wifi(void)
{
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);//reset = 0 - reset mode
}

