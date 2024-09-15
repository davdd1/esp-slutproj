#include "ble_event_task.h"
#include "ble_gatt_client.h"
#include "ble_gap_init.h"
#include "ds18b20_sensor.h"

static char *TAG = "BLE Server";
uint8_t ble_addr_type;

static uint8_t temp[] = {0x09, 0x09, 0x44, 0x61, 0x76, 0x65, 0x20, 0x42, 0x6C, 0x65};

TaskHandle_t sensor_task_handle = NULL;

void start_sensor_task() {
    if (sensor_task_handle == NULL) {
        xTaskCreate(sensor_task, "sensor_task", 2048, NULL, 5, &sensor_task_handle);
        ESP_LOGI("SENSOR", "Sensor task created");
    }
}

void stop_sensor_task() {
    if (sensor_task_handle != NULL) {
        vTaskDelete(sensor_task_handle);
        sensor_task_handle = NULL;
        ESP_LOGW("SENSOR", "Sensor task stopped");
    }
}

// static int gattc_read(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg) {
//     if (error->status == 0) {
//         ESP_LOGI(TAG, "Read successful! Attribute handle: %d", attr->handle);
//     } else {
//         ESP_LOGE(TAG, "Read failed, error: %d", error->status);
//     }
//     return 0;
// }

// static int gattc_write(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg) {
//     if (error->status == 0) {
//         ESP_LOGI(TAG, "Write successful! Attribute handle: %d", attr->handle);
//     } else {
//         ESP_LOGE(TAG, "Write failed, error: %d", error->status);
//     }
//     return 0;
// }

int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_DISC:
        // show all devices
        ESP_LOG_BUFFER_HEX(TAG, event->disc.data, event->disc.length_data);

        // if its a device we are looking for ie temp
        if (memcmp(event->disc.data, temp, 9) == 0)
        {
            int rer = ble_gap_disc_cancel();
            if(rer != 0) {
                ESP_LOGI(TAG, "%x", rer);
            }

            ESP_LOG_BUFFER_HEX(TAG, event->disc.data, event->disc.length_data);
            ESP_LOGI(TAG, "Device found! MAC below:");
            ESP_LOG_BUFFER_HEX(TAG, event->disc.addr.val, 6);
            ESP_LOGI(TAG, "Connecting...");

            int ret = ble_gap_connect(ble_addr_type, &event->disc.addr, BLE_HS_FOREVER, NULL, ble_gap_event, NULL);

            if (ret != 0)
            {
                ESP_LOGE(TAG, "Connection failed!: %d", ret);
            }
        }
        //vTaskDelay(pdMS_TO_TICKS(100));
        break;

    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0)
        {
            ESP_LOGI(TAG, "Connection established!");
            if (hub_device != NULL) {
                 hub_device->conn_handle = event->connect.conn_handle;
            }
            
            start_sensor_task();

            if (has_found_svc)                                  // if service is already found, skip
            {
                ESP_LOGI(TAG, "Service already found, skipping...");
                return 0;
            }
            ESP_LOGI(TAG, "Discovering services KANSKE FELLLLL..."); // log for debugging
            ble_gattc_disc_all_svcs(event->connect.conn_handle, disc_svcs, NULL);
            
        }
        else
        {
            ESP_LOGE(TAG, "Failed to connect: %d", event->connect.status);
            ble_scan();
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGE(TAG, "Disconnected from hub!");
        stop_sensor_task();
        if (hub_device != NULL)
        {
            hub_device->conn_handle = BLE_HS_CONN_HANDLE_NONE;
        }
        ESP_LOGE(TAG, "Reconnecting...");
        ble_scan();
        break;
    default:
        break;
    }
    return 0;
}

void ble_scan(void)
{
    struct ble_gap_disc_params disc_params;
    memset(&disc_params, 0, sizeof(disc_params));
    disc_params.passive = 1;           // passive scanning
    disc_params.filter_duplicates = 1; // filters duplicates
    disc_params.itvl = 0x0010;         // interval between (100ms)
    disc_params.window = 0x0010;       // how long

    int ret = ble_gap_disc(ble_addr_type, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "Discovery failed.");
    }
}

void ble_init()
{
    esp_err_t ret = nvs_flash_init();
    // Intialize NVS
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);                 // Check if NVS intialization is successful
    nimble_port_init();                   // Initialize the host stack
    ble_hs_cfg.sync_cb = ble_app_on_sync; // Initialize application
    nimble_port_freertos_init(host_task); // Run the thread
}