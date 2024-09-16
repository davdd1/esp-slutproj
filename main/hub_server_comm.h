#ifndef HUB_SERVER_COMM_H
#define HUB_SERVER_COMM_H

#define SERVER_IP "192.168.0.14" //ändra
#define SERVER_PORT 8080

#define PREFIX_REGISTER 1
#define PREFIX_SENSOR_DATA 2
#define PREFIX_COMMAND 3

#include "lwip/sockets.h"
#include "esp_log.h"

void read_led_command_from_server();

void send_to_server(int prefix, const char *data);

void close_connection();

#endif