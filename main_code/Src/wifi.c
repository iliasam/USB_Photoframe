#include "main.h"
#include "wifi.h"
#include "key.h"
#include "wifi_strings.h"
#include "wifi_functions.h"
#include "usart.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern UART_HandleTypeDef huart3;

weather_type current_weather = {0,-100,-100};
weather_type weather_forecast[3] = {0,-100,-100,0,-100,-100,0,-100,-100};
weather_type balcon_weather = {0,-100,-100};

TimeType last_wifi_time = {12,0,0};



//update all data from internet
wifi_status_type update_wifi_data(void)
{
  static wifi_status_type wifi_status;
  
  uint8_t process_cnt = 0;
  wifi_status = test_esp_work();
  if (wifi_status == MODULE_BUSY) {reset_wifi(); return wifi_status;}
  if (wifi_status == MODULE_NOT_FOUND) return wifi_status;
  wifi_status = test_ap_connection();
  if (wifi_status == MODULE_BUSY) {reset_wifi(); return wifi_status;}
  if (wifi_status == NO_AP_CONNECTION) return wifi_status;
  
  //ready to connect to servers
  HAL_Delay(800);
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
  uint8_t rx_data[1500];
  uint8_t tx_data[200];
  uint16_t symb_cnt = 0;
  uint16_t total_rx_time_cnt = 0;
  int16_t pos = -1;
  
  uint8_t connected = connect_to_server("api.openweathermap.org");
  if (connected == 0) return WEATHER_NO_CONNECTION;
  
  memset(rx_data, 0, sizeof(rx_data));
  memset(tx_data, 0, sizeof(tx_data));
  
  strcpy((char*)tx_data, REQUEST_STRING_WHITH_KEY);
  strcat((char*)tx_data, "Host: api.openweathermap.org\r\n");
  strcat((char*)tx_data, "\r\n");
  symb_cnt = strlen((char*)tx_data);
  
  if (send_cipsend_command(symb_cnt) != 1)
  {
    wifi_close_connection();
    return WEATHER_FAIL;
  }
  DMA_UART3_Prepare_To_Receive(rx_data, sizeof(rx_data));
  HAL_UART_Transmit(&huart3, tx_data, symb_cnt, 200);//send GET reqest
  while (total_rx_time_cnt < 6000)
  {
    if (find_substring(rx_data,300, "200 OK",5) > -1)
    {
      pos = 1;//found
      HAL_Delay(1000);//ожидаем полной передачи данных
      break;
    }
    else if (find_substring(rx_data,300, "CLOSED",6) > -1)
    {
      pos = -2;
      break;
    }
    else if (find_substring(rx_data,300, "ERROR",5) > -1)
    {
      pos = -3;
      break;
    }

    total_rx_time_cnt+=50;
    HAL_Delay(50);//time step
  }
  Stop_UART3_Rx_DMA();
  
  if (find_substring(rx_data,sizeof(rx_data), "CLOSED",6) < 0)//closed not found
  {
    wifi_close_connection();
  }

  if (pos < 0) return WEATHER_FAIL;
  update_current_weather(&rx_data[0], sizeof(rx_data));
  
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
  
  get_current_time(data);
}

//request for 3 day weather forecast
wifi_status_type get_weather_forecast(void)
{
  uint8_t rx_data[2400];
  uint8_t tx_data[200];
  uint16_t symb_cnt = 0;
  uint16_t total_rx_time_cnt = 0;
  int16_t pos = -1;
  
  uint8_t connected = connect_to_server("api.openweathermap.org");
  if (connected == 0) return FORECAST_NO_CONNECTION;
  
  memset(rx_data, 0, sizeof(rx_data));
  memset(tx_data, 0, sizeof(tx_data));
  
  //strcpy((char*)tx_data,"GET /data/2.5/forecast/daily?id=542374,ru&units=metric&cnt=4&appid=c3fd8f49d2a982fad19e064101e8f3bd HTTP/1.0\r\n");
  strcpy((char*)tx_data,"GET /data/2.5/forecast/daily?id=542374&units=metric&cnt=4&appid=c3fd8f49d2a982fad19e064101e8f3bd HTTP/1.0\r\n");
  strcat((char*)tx_data, "Host: api.openweathermap.org\r\n");
  strcat((char*)tx_data, "\r\n");
  symb_cnt = strlen((char*)tx_data);
  
  if (send_cipsend_command(symb_cnt) != 1)
  {
    wifi_close_connection();
    return WEATHER_FAIL;
  }

  DMA_UART3_Prepare_To_Receive(rx_data, sizeof(rx_data));
  HAL_UART_Transmit(&huart3, tx_data, symb_cnt, 200);//send GET reqest
  while (total_rx_time_cnt < 5000)
  {
    if (find_substring(rx_data,300, "200 OK",5) > -1)
    {
      pos = 1;//found
      HAL_Delay(2000);//ожидаем полной передачи данных
      break;
    }
    else if (find_substring(rx_data,300, "CLOSED",6) > -1)
    {
      pos = -2;
      break;
    }
    else if (find_substring(rx_data,300, "ERROR",5) > -1)
    {
      pos = -3;
      break;
    }

    total_rx_time_cnt+=50;
    HAL_Delay(50);//time step
  }
  Stop_UART3_Rx_DMA();
  
  if (find_substring(rx_data,sizeof(rx_data), "CLOSED",6) < 0)//closed not found
  {
    wifi_close_connection();
  }  
  
  if (pos < 0) return FORECAST_FAIL;
  
  update_forecast(&rx_data[0], sizeof(rx_data));
  return FORECAST_RECEIVED;
}

/*
//request majordomo server to send weather
wifi_status_type get_majordomo_temperature(void)
{
  uint8_t rx_data[500];
  int16_t pos = 0;
  
  uint8_t connected = connect_to_server("192.168.1.39");
  if (connected == 0) return MAJORDOMO_NO_CONNECTION;
  
  memset(&rx_data[0], 0, sizeof(rx_data));
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
  wifi_close_connection();
  update_majordomo_temperature(&rx_data[pos+30], 400,&balcon_weather.temperature1);
  
  return MAJORDOMO_RECEIVED;
}
*/


//request majordomo server to send weather
wifi_status_type get_majordomo_temperature(void)
{
  uint8_t rx_data[1500];
  uint8_t tx_data[200];
  uint16_t symb_cnt = 0;
  uint16_t total_rx_time_cnt = 0;
  int16_t pos = -1;
  
  uint8_t connected = connect_to_server("192.168.1.39");
  if (connected == 0) return MAJORDOMO_NO_CONNECTION;
  
  memset(rx_data, 0, sizeof(rx_data));
  memset(tx_data, 0, sizeof(tx_data));
  
  strcpy((char*)tx_data,"GET /objects/?object=Balcon&op=get&p=TempOut HTTP/1.0\r\n");
  strcat((char*)tx_data, "Host: 192.168.1.39\r\n");
  strcat((char*)tx_data, "\r\n");
  symb_cnt = strlen((char*)tx_data);
  
  if (send_cipsend_command(symb_cnt) != 1)
  {
    wifi_close_connection();
    return MAJORDOMO_FAIL;
  }
  DMA_UART3_Prepare_To_Receive(rx_data, sizeof(rx_data));
  HAL_UART_Transmit(&huart3, tx_data, symb_cnt, 200);//send GET reqest
  while (total_rx_time_cnt < 6000)
  {
    if (find_substring(rx_data,300, "200 OK",5) > -1)
    {
      pos = 1;//found
      HAL_Delay(1000);//ожидаем полной передачи данных
      break;
    }
    else if (find_substring(rx_data,300, "CLOSED",6) > -1)
    {
      pos = -2;
      break;
    }
    else if (find_substring(rx_data,300, "ERROR",5) > -1)
    {
      pos = -3;
      break;
    }
    total_rx_time_cnt+=50;
    HAL_Delay(50);//time step
  }
  Stop_UART3_Rx_DMA();
  
  if (find_substring(rx_data,sizeof(rx_data), "CLOSED",6) < 0)//closed not found
  {
    wifi_close_connection();
  }

  if (pos < 0) return MAJORDOMO_FAIL;
  update_majordomo_temperature(&rx_data[0], sizeof(rx_data),&balcon_weather.temperature1);
  
  return MAJORDOMO_RECEIVED;
}

//request majordomo server to send weather
wifi_status_type get_majordomo_temperature2(void)
{
  uint8_t rx_data[500];
  int16_t pos = 0;

  uint8_t connected = connect_to_server("192.168.1.39");
  if (connected == 0) return MAJORDOMO_NO_CONNECTION;
  
  memset(&rx_data[0], 0, sizeof(rx_data));
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
    if (find_substring(rx_data,sizeof(rx_data), "CLOSED",6) < 0)//closed not found
    {
      wifi_close_connection();
    }
    return MAJORDOMO_FAIL;
  }
  if (find_substring(rx_data,sizeof(rx_data), "CLOSED",6) < 0)//closed not found
  {
    wifi_close_connection();
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
  
  get_current_time(data);
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
  
  get_current_time(data);
  
  return;
}

//analyse received string to get time
void get_current_time(uint8_t *str)
{
  volatile static uint8_t time_str[50];//for debug
  int16_t pos_start = -1;
  int16_t length = 0;
  
  pos_start = find_substring(str,300, "Date",4);
  
  if (pos_start > 0)
  {
    length = find_substring(&str[pos_start],60, "\r\n",2);
    if ((length > sizeof(time_str)) || (length < 0)) return;
    memcpy((uint8_t*)time_str, &str[pos_start], length);
    
    last_wifi_time = get_time_from_string((uint8_t*)time_str, (uint8_t)length);
  }
}

//**********************************************************************************

wifi_status_type test_ap_connection(void)
{
  uint8_t tx_data[] ="AT+PING=\"192.168.1.1\"\r\n";
  uint8_t rx_data[50];
  int16_t pos = 0;
  
  memset(&rx_data[0], 0, sizeof(rx_data));
 
  DMA_UART3_Prepare_To_Receive(&rx_data[0], sizeof(rx_data));
  HAL_UART_Transmit(&huart3, &tx_data[0], sizeof(tx_data), 100);
  DMA_UART3_Wait_And_Stop(1000);
  
  pos = find_substring(&rx_data[0], sizeof(rx_data), "busy",2);
  if (pos != -1) 
  {
    DMA_UART3_Wait_And_Stop(0);//stop DMA
    return MODULE_BUSY;
  }
  
  pos = find_substring(&rx_data[0], sizeof(rx_data), "timeout",7);
  if (pos != -1) return NO_AP_CONNECTION;
  
  pos = find_substring(&rx_data[8],(sizeof(rx_data)-8), "+",1);//answer always have "AT+" part, but good ping give +XX answer
  if (pos != -1) 
  {
    DMA_UART3_Wait_And_Stop(0);//stop DMA
    return AP_CONNECTED;
  }
  
  return NO_AP_CONNECTION;
}


//проверить наличие модуля
//cheeck esp8266 presence
wifi_status_type test_esp_work(void)
{
  uint8_t tx_data[] = "AT\r\n";
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

//reset esp8266
void reset_wifi(void)
{
   disable_wifi();
   HAL_Delay(100);
   enable_wifi();
   HAL_Delay(3000);
}

void enable_wifi(void)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);//led on
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);//reset = 1 - main mode
}

void disable_wifi(void)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);//led on
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);//reset = 0 - reset mode
}
