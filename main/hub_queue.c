#include "hub_queue.h"
#include "hub_server_comm.h"

const char* TAG = "HUB-QUEUE";

QueueHandle_t data_queue = NULL;

void init_queue(){
    data_queue = xQueueCreate(QUEUE_LENGTH, sizeof(queue_item_t)); //Queue for data, size defined in header
    if(data_queue == NULL){
        ESP_LOGE("QUEUE", "Failed to create queue");
    }
}
void process_queue_task(void* pvParameters) {
    queue_item_t item;
    while (1) {
    //check if wifi is init

    if (has_wifi_connected) {
        if (xQueueReceive(data_queue, &item, portMAX_DELAY) == pdPASS) {
            switch (item.action_type) {
                case ACTION_SEND_TEMPERATURE:
                    //Handle temp data
                    ESP_LOGI(TAG, "Sending temperature: %s", item.data.temperature);
                    send_to_server(PREFIX_SENSOR_DATA, item.data.temperature);
                    break;
                case ACTION_SEND_MAC_ADDRESS:
                    //Handle mac address
                    char mac_str[18];
                    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
                            item.data.mac_address[0], item.data.mac_address[1], item.data.mac_address[2], 
                            item.data.mac_address[3], item.data.mac_address[4], item.data.mac_address[5]);
                    ESP_LOGW(TAG, "Sending MAC address: %s", mac_str);
                    send_to_server(PREFIX_REGISTER, mac_str);
                    break;
                default:
                    ESP_LOGE(TAG, "Unknown action type: %d", item.action_type);
                    break;
            }
            //Maybe close connection here
            // close_connection();
    }
        } else {
            vTaskDelay(100 / portTICK_PERIOD_MS);  // Wait for a second before checking again
        }
    }
}
void enqueue_temperature(const char* temperature) {
    ESP_LOGW(TAG, "Enqueuing temperature data");
    queue_item_t item;
    item.action_type = ACTION_SEND_TEMPERATURE;
    
    // Copy temperature data to item (strncpy ensures no buffer overflow)
    strncpy(item.data.temperature, temperature, sizeof(item.data.temperature) - 1);
    item.data.temperature[sizeof(item.data.temperature) - 1] = '\0'; // Null-terminate

    if (xQueueSend(data_queue, &item, 100 / portTICK_PERIOD_MS) != pdPASS) {
        // Queue is full, check if we can discard the oldest item (only temperature)
        ESP_LOGW(TAG, "Queue is full, checking oldest item");

        queue_item_t oldest_item;
        if (xQueuePeek(data_queue, &oldest_item, 0) == pdPASS) {
            // Check if the oldest item is temperature data
            if (oldest_item.action_type == ACTION_SEND_TEMPERATURE) {
                // Discard the oldest temperature data
                ESP_LOGW(TAG, "Discarding oldest temperature data");
                xQueueReceive(data_queue, &oldest_item, 0); // Remove the oldest item

                // Now try sending the new item again
                if (xQueueSend(data_queue, &item, 100 / portTICK_PERIOD_MS) != pdPASS) {
                    ESP_LOGE(TAG, "Failed to enqueue temperature data after discarding oldest");
                } else {
                    ESP_LOGI(TAG, "Temperature data enqueued after discarding oldest");
                }
            } else {
                ESP_LOGW(TAG, "Oldest item is not temperature data, skipping enqueue");
            }
        }
    } else {
        ESP_LOGW(TAG, "Temperature data enqueued");
    } 
}

void enqueue_mac_address(const uint8_t* mac_address) {
    ESP_LOGW(TAG, "Enqueuing MAC address");
    ESP_LOGW(TAG, "WIFICONNECTED: %d", has_wifi_connected);
    queue_item_t item;
    item.action_type = ACTION_SEND_MAC_ADDRESS;
    
    memcpy(item.data.mac_address, mac_address, 6);

    // Try to enqueue the MAC address
    if (xQueueSend(data_queue, &item, 100 / portTICK_PERIOD_MS) != pdPASS) {
        ESP_LOGW(TAG, "Queue is full, checking oldest item");

        queue_item_t oldest_item;
        // Peek to see the oldest item in the queue
        if (xQueuePeek(data_queue, &oldest_item, 0) == pdPASS) {
            // Check if the oldest item is temperature data
            if (oldest_item.action_type == ACTION_SEND_TEMPERATURE) {
                // Discard the oldest temperature data
                ESP_LOGW(TAG, "Discarding oldest temperature data to enqueue MAC address");
                xQueueReceive(data_queue, &oldest_item, 0); // Remove the oldest temperature item

                // Now try to enqueue the MAC address again
                if (xQueueSend(data_queue, &item, 100 / portTICK_PERIOD_MS) != pdPASS) {
                    ESP_LOGE(TAG, "Failed to enqueue MAC address after discarding oldest");
                } else {
                    ESP_LOGI(TAG, "MAC address enqueued after discarding oldest temperature data");
                }
            } else {
                ESP_LOGE(TAG, "Queue is full, but oldest item is not temperature data. Could not enqueue MAC address.");
            }
        } else {
            ESP_LOGE(TAG, "Failed to peek into queue.");
        }
    } else {
        ESP_LOGI(TAG, "MAC address enqueued");
    }
}
