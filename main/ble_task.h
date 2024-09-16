#ifndef BLE_TASK_H
#define BLE_TASK_H

#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

extern bool is_connected;
extern uint16_t ble_gap_conn_handle;

int ble_gap_event(struct ble_gap_event *event, void *arg);
void ble_wifi_init();

#endif