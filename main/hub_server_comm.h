#ifndef HUB_SERVER_COMM_H
#define HUB_SERVER_COMM_H

#define SERVER_IP "192.168.2.106" //CHANGE
#define SERVER_PORT 1800          //CHANGE

#define PREFIX_REGISTER 1
#define PREFIX_SENSOR_DATA 2

#include "lwip/sockets.h"
#include "esp_log.h"

extern char led_command_buffer[64];

bool connect_to_server_with_retries(int retries, int delay_ms, int *sock);

void read_led_command_from_server();

void send_to_server(int prefix, const char *data);

#endif