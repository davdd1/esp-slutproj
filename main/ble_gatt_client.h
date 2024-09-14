#ifndef BLE_GATT_CLIENT_H
#define BLE_GATT_CLIENT_H

#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "arpa/inet.h"

extern bool has_found_svc;

typedef struct {
    uint8_t addr[BLE_DEV_ADDR_LEN];
    uint16_t service_uuid;
    uint16_t write_char_handle;
    uint16_t read_char_handle;
    uint16_t conn_handle;
} ble_device_info_t;

extern ble_device_info_t *hub_device;

void init_ble_device();

int disc_chrs(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *characteristic, void *arg);

int disc_svcs(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg);

int send_data_to_hub(float sensor_temperature);

#endif

