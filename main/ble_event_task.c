#include "ble_event_task.h"
#include "ble_gatt_client.h"
#include "ble_gap_init.h"
#include "ds18b20_sensor.h"

static char *TAG = "BLE_EVENT_HANDLER";
uint8_t ble_addr_type;

static uint8_t temp_mac[] = {0x09, 0x09, 0x44, 0x61, 0x76, 0x65, 0x20, 0x42, 0x6C, 0x65};

TaskHandle_t device_task_handle = NULL;
TaskHandle_t sensor_task_handle = NULL;

void start_tasks()
{
    if (device_task_handle == NULL)
    {
        xTaskCreate(device_task, "device_task", 2048, NULL, 4, &device_task_handle);
        ESP_LOGI("DEVICE", "Device task created");
    }
    if (sensor_task_handle == NULL)
    {
        xTaskCreate(sensor_task, "sensor_task", 3072, NULL, 5, &sensor_task_handle);
        ESP_LOGI("SENSOR", "Sensor task created");
    }
}

void stop_tasks()
{
    if (device_task_handle != NULL)
    {
        device_task_handle = NULL;
        ESP_LOGW("DEVICE", "Device task stopped");
    }
    if (sensor_task_handle != NULL)
    {
        vTaskDelete(sensor_task_handle);
        sensor_task_handle = NULL;
        ESP_LOGW("SENSOR", "Sensor task stopped");
    }
}

int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_DISC:
        // show all devices
        ESP_LOG_BUFFER_HEX(TAG, event->disc.data, event->disc.length_data);

        // Set connection/scan parameters
        struct ble_gap_conn_params conn_params;
        memset(&conn_params, 0, sizeof(conn_params));
        conn_params.scan_itvl = 0x0010;
        conn_params.scan_window = 0x0010;
        conn_params.itvl_min = 0x0018;
        conn_params.itvl_max = 0x0028;
        conn_params.latency = 0;
        conn_params.supervision_timeout = 0x0100;

        // if its a device we are looking for ie temp_mac
        if (memcmp(event->disc.data, temp_mac, 9) == 0)
        {
            int rer = ble_gap_disc_cancel();
            if (rer != 0)
            {
                ESP_LOGI(TAG, "%x", rer);
            }

            ESP_LOG_BUFFER_HEX(TAG, event->disc.data, event->disc.length_data);
            ESP_LOGI(TAG, "Device found! MAC below:");
            ESP_LOG_BUFFER_HEX(TAG, event->disc.addr.val, 6);
            ESP_LOGI(TAG, "Connecting...");

            int ret = ble_gap_connect(ble_addr_type, &event->disc.addr, BLE_HS_FOREVER, &conn_params, ble_gap_event, NULL);

            if (ret != 0)
            {
                ESP_LOGE(TAG, "Connection failed!: %d", ret);
            }
        }
        break;

    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0)
        {
            ESP_LOGI(TAG, "Connection established!");
            if (hub_device != NULL)
            {
                hub_device->conn_handle = event->connect.conn_handle;
            }

            start_tasks();  // start the tasks

            if (has_found_svc) // if service is already found, skip
            {
                ESP_LOGI(TAG, "Service already found, skipping...");
                return 0;
            }
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
        stop_tasks();
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
    disc_params.itvl = 0x0050;         // interval between
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