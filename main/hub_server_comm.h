#ifndef HUB_SERVER_COMM_H
#define HUB_SERVER_COMM_H

#define SERVER_IP "192.168.2.106" //ändra till dator ip
#define SERVER_PORT 1800

#define PREFIX_REGISTER 1
#define PREFIX_SENSOR_DATA 2

#include "lwip/sockets.h"
#include "esp_log.h"

extern char led_command_buffer[64];
extern int tcp_connected;

bool connect_to_server_with_retries(int retries, int delay_ms, int *sock);

void read_led_command_from_server();

void send_to_server(int prefix, const char *data);

void close_connection();

#endif