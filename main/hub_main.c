#include "hub_ble_gap.h"
#include "hub_ble_gatt.h"
#include "ble_task.h"
#include "hub_queue.h"
#include "hub_server_comm.h"

void app_main(void)
{
    // Initialize the queue for sending data
    init_queue();

    xTaskCreate(ble_wifi_init, "ble_wifi_init", 4096, NULL, 5, NULL);
    
    // Start the queue processing task
    xTaskCreate(process_queue_task, "process_queue_task", 4096, NULL, 2, NULL);

    while (1)
    {
        // Read LED command from the Go server every 10 seconds
        read_led_command_from_server();
        vTaskDelay(pdMS_TO_TICKS(10000)); // Wait for 10 seconds
    }
}