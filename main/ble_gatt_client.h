#ifndef BLE_GATT_CLIENT_H
#define BLE_GATT_CLIENT_H

#include <stdio.h>
#include <stdint.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "arpa/inet.h"

// Hub GATT Service UUIDs
#define HUB_SERVICE_UUID 0x0180 // The primary service UUID for the hub

// Characteristics UUIDs within HUB_SERVICE_UUID
#define HUB_DEVICE_ID_CHAR_UUID 0x1234 // Characteristic for Device ID (prefix 1)
#define HUB_TEMP_DATA_CHAR_UUID 0x5678 // Characteristic for Temperature data (prefix 2)
#define HUB_COMMANDS_CHAR_UUID 0x9ABC  // Characteristic for Commands/Configuration (prefix 3)


#define PREFIX_REGISTER 1
#define PREFIX_SENSOR_DATA 2
#define PREFIX_COMMAND 3

extern bool has_found_svc;

typedef struct {
    uint8_t addr[BLE_DEV_ADDR_LEN];
    uint16_t service_uuid;
    uint16_t device_id_char_handle;
    uint16_t temp_data_char_handle;
    uint16_t commands_char_handle;
    uint16_t conn_handle;
} ble_device_info_t;

extern ble_device_info_t *hub_device;

void init_ble_device();

void device_task(void* pvParameters);

int disc_chrs(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *characteristic, void *arg);

int disc_svcs(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg);

void write_cb(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg);

int send_device_id_to_hub();
int send_temp_data_to_hub(float sensor_temperature);

int send_data_handler(int prefix, float sensor_tempearture);

#endif

