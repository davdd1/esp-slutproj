#ifndef BLE_EVENT_TASK_H
#define BLE_EVENT_TASK_H

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
#include "host/ble_gap.h"
#include "host/ble_gatt.h"


extern uint8_t ble_addr_type;

void start_tasks();
void stop_tasks();

int ble_gap_event(struct ble_gap_event *event, void *arg);
void ble_scan(void);
void ble_init();

#endif