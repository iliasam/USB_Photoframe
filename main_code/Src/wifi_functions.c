#include "wifi_functions.h"
#include "wifi_strings.h"
#include "usart.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart3;

uint8_t send_cipsend_command(uint16_t length)
{
  uint8_t tx_data[100];
  uint8_t rx_data[100];
  int16_t pos = 0;  
  uint8_t tx_bytes_cnt = 0;
  uint16_t total_rx_time_cnt = 0;
  
  //clean tx and tx buffers
  memset(tx_data, 0, sizeof(tx_data));
  memset(rx_data, 0, sizeof(rx_data)); 
  //tx command
  tx_bytes_cnt = sprintf((char*)tx_data, "AT+CIPSEND=%d\r\n", length);
  //rx answer
  DMA_UART3_Prepare_To_Receive(rx_data, sizeof(rx_data));
  HAL_UART_Transmit(&huart3, tx_data, tx_bytes_cnt, 100);
  while (total_rx_time_cnt < 2000)
  {
    if (find_substring(rx_data,sizeof(rx_data), "CLOSED",6)  > -1) 
    {
      Stop_UART3_Rx_DMA();
      return 0;//fail
    }
    else if (find_substring(rx_data,sizeof(rx_data), ">",1)  > -1) 
    {
      Stop_UART3_Rx_DMA();
      return 1;
    }
    
    total_rx_time_cnt+=50;
    HAL_Delay(50);//time step
  }
  
  pos = find_substring(rx_data,100, ">",1);
  if (pos == -1) 
  {
    return 0;
  }
  return 1;
}

uint8_t connect_to_server(char* server_name)
{
  uint8_t connection_result = 0;
  wifi_connection_status_type connection_status = CONNECTION_NO;
  
  connection_status = single_connect_to_server(server_name);
  
  if (connection_status == CONNECTION_GOOD)
  {
    return 1;//CONNECTION_GOOD
  }
  else
  {
    //connection fail
    if (wifi_close_connection() == 1)//try to close connection
    {
      //now really closed
      if (connection_status == CONNECTION_ALREADY)
      {
        connection_status = single_connect_to_server(server_name);//try to connect again
        if (connection_status == CONNECTION_GOOD)
        {
          connection_result = 1;//CONNECTION_GOOD
        }
        else
        {
          wifi_close_connection();
          connection_result = 0;//cannot connect again
        }
      }
      else
      {
        connection_result = 0;//connection fail from first time
      }
    }
    else
    {
      connection_result = 0;//cannot close connection - bad code
    }
  }
  asm("nop");
  return connection_result;
}

//единственна€ попытка подключитс€ к серверу
wifi_connection_status_type single_connect_to_server(char* server_name)
{
  uint16_t symb_cnt = 0;
  uint16_t total_rx_time_cnt = 0;
  wifi_connection_status_type connection_status = CONNECTION_NO;
  uint8_t tx_data[150];
  uint8_t rx_data[200];
  
  //clean tx and tx buffers
  memset(tx_data, 0, sizeof(tx_data)); 
  memset(rx_data, 0, sizeof(rx_data)); 
  
  strcpy((char*)tx_data,"AT+CIPSTART=\"TCP\",\"");
  strcat((char*)tx_data, server_name);
  strcat((char*)tx_data, "\",80\r\n");
  
  symb_cnt = strlen((char*)tx_data); 
  
  DMA_UART3_Prepare_To_Receive(&rx_data[0], sizeof(rx_data));
  HAL_UART_Transmit(&huart3, &tx_data[0], symb_cnt, 100);//AT+CIPSTART
  
  while (total_rx_time_cnt < 7000)
  {
    if (find_substring(&rx_data[0],sizeof(rx_data), "ALREADY",7) > -1) 
    {
      connection_status = CONNECTION_ALREADY;
      Stop_UART3_Rx_DMA();
      return connection_status;
    }
    if (find_substring(&rx_data[0],sizeof(rx_data), "DNS Fail",7) > -1) 
    {
      connection_status = CONNECTION_ALREADY;
      Stop_UART3_Rx_DMA();
      return connection_status;
    }
    else if (find_substring(&rx_data[0],sizeof(rx_data), "CONNECT",7) > -1)
    {
      connection_status = CONNECTION_GOOD;
      Stop_UART3_Rx_DMA();
      return connection_status;
    }
    else if (find_substring(&rx_data[0],sizeof(rx_data), "ERROR",5) != -1)
    {
      connection_status = CONNECTION_ERROR;
      Stop_UART3_Rx_DMA();
      return connection_status;
    }
    else if (find_substring(&rx_data[0],sizeof(rx_data), "CLOSE",5) != -1)
    {
      connection_status = CONNECTION_ERROR;
      Stop_UART3_Rx_DMA();
      return connection_status;
    }
    
    total_rx_time_cnt+=50;
    HAL_Delay(50);//time step
  }
  
  return CONNECTION_NO;//timeout
  
}



//return 1 - successfully closed
uint8_t wifi_close_connection(void)
{
  uint8_t rx_data[100];
  memset(rx_data, 0, sizeof(rx_data)); 
  
  uint8_t tx_data[] ="AT+CIPCLOSE\r\n";
  HAL_UART_Transmit(&huart3, &tx_data[0], sizeof(tx_data), 100);
  //HAL_Delay(2000);
  DMA_UART3_Wait_For_Answer(5000, 100, rx_data, sizeof(rx_data), 4);
  
  if (find_substring(&rx_data[0],sizeof(rx_data), "CLOSED",6) != 1) return 1;
  return 0;
}




