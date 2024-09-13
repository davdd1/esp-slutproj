#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_task.h"
#include "driver/gpio.h"

void app_main(void)
{
    ble_wifi_init(); // Start program, initialize BLE and listen for WiFi credentials
}