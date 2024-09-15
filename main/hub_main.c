#include "hub_ble_gap.h"
#include "hub_ble_gatt.h"
#include "ble_task.h"
#include "hub_queue.h"
#include "driver/gpio.h"

void app_main(void)
{
    // Initialize the queue for sending data
    init_queue(); 

    // Start the queue processing task
    xTaskCreate(process_queue_task, "process_queue_task", 4096, NULL, 2, NULL);

     // Initialize BLE and WiFi
    ble_wifi_init(); 
}