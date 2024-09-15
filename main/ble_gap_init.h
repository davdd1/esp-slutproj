#ifndef BLE_GAP_INIT_H
#define BLE_GAP_INIT_H

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
#include "sdkconfig.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"

void ble_app_on_sync(void);
void host_task(void *params);

#endif