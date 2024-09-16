#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include <string.h>
#include <stdbool.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"

extern bool is_ssid_set;
extern bool is_pass_set;
extern bool has_wifi_init;
extern bool has_wifi_connected;
extern wifi_config_t wifi_config;

void wifi_init();
void set_wifi_ssid(const char* ssid);
void set_wifi_password(const char* password);
const char* get_wifi_ssid();
const char* get_wifi_password();

#endif