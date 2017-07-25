#ifndef _WIFI_FUNCTIONS_H
#define _WIFI_FUNCTIONS_H

#include <stdint.h>


typedef enum
{
  CONNECTION_NO = 0,
  CONNECTION_ALREADY,
  CONNECTION_ERROR,
  CONNECTION_GOOD  
} wifi_connection_status_type;


uint8_t wifi_close_connection(void);
uint8_t send_cipsend_command(uint16_t length);
uint8_t connect_to_server(char* server_name);
wifi_connection_status_type single_connect_to_server(char* server_name);


#endif