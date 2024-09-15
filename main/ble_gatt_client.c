#include "ble_gatt_client.h"
#include "ds18b20_sensor.h"
#include "esp_mac.h"

static char *TAG = "BLE_GATT_CLIENT";

ble_device_info_t *hub_device;
bool has_found_all_chrs = false;
bool has_found_svc = false;

// init device info, mallloc
void init_ble_device()
{
    hub_device = malloc(sizeof(ble_device_info_t));
    if (hub_device == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for hub device");
        return;
    }
    else
    {
        ESP_LOGI(TAG, "Allocated memory for hub device");
        hub_device->conn_handle = BLE_HS_CONN_HANDLE_NONE;
    }
}

void device_task(void *pvParameters)
{
    while (!has_found_all_chrs) {
        ESP_LOGI("DEVICE_TASK", "Wait for service discovery to complete");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Service disc complete. Device task started");

    while (1)
    {
        // Check if the device is connected
        if (hub_device->conn_handle != BLE_HS_CONN_HANDLE_NONE)
        {
            ESP_LOGI(TAG, "Device connected. Sending device ID to hub...");
            int ret = send_data_handler(PREFIX_REGISTER, 0); // Send device ID
            if (ret == 0)
            {
                ESP_LOGI(TAG, "Device ID successfully sent to hub");
                break;
            }
            else
            {
                ESP_LOGE(TAG, "Failed to send device ID to hub");
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 10 seconds
    }
    ESP_LOGI(TAG, "Device task completed");
    vTaskDelete(NULL);
}

int disc_chrs(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *characteristic, void *arg)
{
    if (has_found_all_chrs)
    {
        ESP_LOGI(TAG, "Characteristic discovery already completed.");
        return 0;
    }
    ESP_LOGI(TAG, "IN DISC CHRS FUNCTION");
    if (characteristic == NULL)
    {
        ESP_LOGE(TAG, "Characteristic is NULL");
        return -1; // error
    }
    ESP_LOGI(TAG, "Characteristic UUID: %x", characteristic->uuid.u16.value);

    static bool has_device_id_chr = false;
    static bool has_temp_data_chr = false;
    static bool has_commands_chr = false;

    if (error->status != 0)
    {
        ESP_LOGE(TAG, "Characteristic discovery failed: %d", error->status);
        return 0;
    }

    if (characteristic->uuid.u16.value == HUB_DEVICE_ID_CHAR_UUID)
    {
        ESP_LOGI(TAG, "IN DEVICE ID CHAR");
        has_device_id_chr = true;
        hub_device->device_id_char_handle = characteristic->val_handle;
        ESP_LOGI(TAG, "AFTER DEVICE ID CHAR");
    }
    else if (characteristic->uuid.u16.value == HUB_TEMP_DATA_CHAR_UUID)
    {
        ESP_LOGI(TAG, "IN TEMP DATA CHAR");
        has_temp_data_chr = true;
        hub_device->temp_data_char_handle = characteristic->val_handle;
        ESP_LOGI(TAG, "AFTER TEMP DATA CHAR");
    }
    // else if (characteristic->uuid.u16.value == HUB_COMMANDS_CHAR_UUID)
    // {
    //     ESP_LOGI(TAG, "IN COMMANDS CHAR");
    //     has_commands_chr = true;
    //     hub_device->commands_char_handle = characteristic->val_handle;
    //     ESP_LOGI(TAG, "AFTER COMMANDS CHAR");
    // }

    ESP_LOGW(TAG, "has_temp_data_chr: %d, has_device_id_chr: %d, has_commands_chr: %d", has_temp_data_chr, has_device_id_chr, has_commands_chr);
    if (has_temp_data_chr && has_device_id_chr)
    {
        ESP_LOGI(TAG, "Characteristic discovery completed");
        has_found_all_chrs = true;
    }

    return 0;
}

int disc_svcs(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg)
{
    if (has_found_svc)
    {
        ESP_LOGI(TAG, "Service discovery already completed");
        return 0;
    }
    if (error->status != 0)
    {
        ESP_LOGE(TAG, "Service discovery failed: %d", error->status);
        return error->status;
    }

    ESP_LOGI(TAG, "Found Service UUID: %x", service->uuid.u16.value);

    // if service is the one we are looking for, connect
    if (service->uuid.u16.value == HUB_SERVICE_UUID && !has_found_all_chrs)
    {
        has_found_svc = true;
        ESP_LOGI(TAG, "Service found, discovering characteristics...");
        hub_device->service_uuid = service->uuid.u16.value;
        ESP_LOGI(TAG, "Trying to connect to service: %x...", service->uuid.u16.value);
        int ret = ble_gattc_disc_all_chrs(conn_handle, service->start_handle, service->end_handle, disc_chrs, NULL);
        if (ret != 0)
        {
            ESP_LOGE(TAG, "Characteristic discovery failed to start: %d", ret);
            return -1;
        }
    }

    if (has_found_svc)
    {
        ESP_LOGI(TAG, "Service discovery completed");
        return 0;
    }

    return 0;
}

void write_cb(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
    if (error->status == 0)
    {
        ESP_LOGI(TAG, "Write successful! Attribute handle: %d", attr->handle);
    }
    else
    {
        ESP_LOGE(TAG, "Write failed, error: %d", error->status);
    }
}

int send_device_id_to_hub()
{
    if (hub_device->device_id_char_handle != 0)
    {
        // buffer for mac
        uint8_t mac[6];
        // get mac
        esp_err_t ret_mac = esp_read_mac(mac, ESP_MAC_BT);
        if (ret_mac != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to read MAC address: %d", ret_mac);
            return -1;
        }

        ESP_LOGI(TAG, "Sending Bluetooth MAC: %02x:%02x:%02x:%02x:%02x:%02x", 
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        // send raw mac to hub
        int ret = ble_gattc_write_flat(hub_device->conn_handle, hub_device->device_id_char_handle,
                                       mac, sizeof(mac), write_cb, NULL);

        if (ret != 0)
        {
            ESP_LOGE(TAG, "Write failed: %d", ret);
            return -1;
        }
    }
    return 0;
}

int send_temp_data_to_hub(float sensor_temperature)
{
    if (hub_device->temp_data_char_handle != 0)
    {
        char temp_buffer[10];
        snprintf(temp_buffer, sizeof(temp_buffer), "%.2f", sensor_temperature);

        int ret = ble_gattc_write_flat(hub_device->conn_handle, hub_device->temp_data_char_handle,
                                       temp_buffer, strlen(temp_buffer), write_cb, NULL);
        if (ret != 0)
        {
            ESP_LOGE(TAG, "Write failed: %d", ret);
            return -1;
        }
        return 0;
    }
    ESP_LOGE(TAG, "Invalid Temperature data characteristic");
    return -1;
}

// write to hub
int send_data_handler(int prefix, float sensor_temperature)
{
    ESP_LOGI(TAG, "IN SEND TO HUB FUNCTION");

    ESP_LOGI(TAG, "Temperature: %.2f °C", sensor_temperature);
    // print all device info field for debug
    ESP_LOGI(TAG, "Service UUID: %x", hub_device->service_uuid);
    ESP_LOGI(TAG, "Temp data char handle: %d", hub_device->temp_data_char_handle);
    ESP_LOGI(TAG, "Device id char handle: %d", hub_device->device_id_char_handle);
    ESP_LOGI(TAG, "Conn Handle: %d", hub_device->conn_handle);

    // write to hub

    // Ensure the connection and handles are valid before attempting to write
    if (hub_device->conn_handle == BLE_HS_CONN_HANDLE_NONE)
    {
        ESP_LOGE(TAG, "Invalid connection handle");
        return -1;
    }

    switch (prefix)
    {
    case PREFIX_REGISTER:
        // send device id
        return send_device_id_to_hub();
    case PREFIX_SENSOR_DATA:
        // send temperature data
        return send_temp_data_to_hub(sensor_temperature);
    // maybe command later
    default:
        ESP_LOGE(TAG, "Invalid prefix: %d", prefix);
        return -1;
    }
    return 0;
}