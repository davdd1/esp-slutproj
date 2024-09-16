#include "ble_gap_init.h"
#include "ble_gatt_client.h"
#include "ble_event_task.h"
#include "ds18b20_sensor.h"
#include "client_led.h"

void app_main(void)
{
    init_ble_device();
    ble_init(); // Start program, initialize BLE and listen for WiFi credentials
}