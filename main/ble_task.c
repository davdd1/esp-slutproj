#include "ble_task.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "wifi_task.h"

static char *TAG = "BLE Server";

int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "BLE GAP EVENT CONNECTION %s", event->connect.status == 0 ? "OK" : "FAILED");

        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
        // Advertise again
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "DEVICE DISCONNECTED. RESTARTING ADVERTISING");
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "GRATTIS GUBBEN! NY SUB: Attr handle: %d", event->subscribe.attr_handle);
        if (event->subscribe.cur_notify)
        {
            ESP_LOGI(TAG, "NOTIFICATION ENABLED");
            device_notify(event->subscribe.conn_handle, event->subscribe.attr_handle);
            break;
        }
        break;
    default:
        break;
    }
    return 0;
}

void ble_wifi_init()
{
    esp_err_t ret = nvs_flash_init();
    // Intialize NVS
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);                            // Check if NVS intialization is successful
    nimble_port_init();                              // Initialize the host stack
    ble_svc_gap_device_name_set("Daves BLE-Server"); // Initialize NimBLE configuration - server name
    ble_svc_gap_init();                              // Initialize NimBLE configuration - gap service
    ble_svc_gatt_init();                             // Initialize NimBLE configuration - gatt service
    ble_gatts_count_cfg(gatt_svcs);                  // Initialize NimBLE configuration - config gatt services
    ble_gatts_add_svcs(gatt_svcs);                   // Initialize NimBLE configuration - queues gatt services.
    ble_hs_cfg.sync_cb = ble_app_on_sync;            // Initialize application
    nimble_port_freertos_init(host_task);            // Run the thread
    while (1)
    {
        if (is_ssid_set && is_pass_set)
        {
            wifi_init();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // delay 1 sec
    }
}