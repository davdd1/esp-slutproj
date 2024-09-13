#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_task.h"
#include "sensor.h"

void app_main(void)
{
    ble_init(); // Start program, initialize BLE and listen for WiFi credentials
    init_ble_device();

}