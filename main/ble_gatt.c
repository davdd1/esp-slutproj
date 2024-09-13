#include "ble_gatt.h"
#include "sensor.h"

#define HUB_READ_CHAR_UUID 0x1234
#define HUB_WRITE_CHAR_UUID 0x5678
#define HUB_SERVICE_UUID 0x0180

#define MAX_RETRY_COUNT 5
#define INITIAL_RETRY_DELAY_MS 100

static char *TAG = "BLE Server";

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
    }
    else
    {
        ESP_LOGI(TAG, "Allocated memory for hub device");
    }
}

int disc_chrs(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *characteristic, void *arg)
{
    if (has_found_all_chrs)
    {
        ESP_LOGI(TAG, "Characteristic discovery already completed.");
        return 0;
    }
    ESP_LOGI(TAG, "IN DISC CHRS FUNCTION");
    ESP_LOGI(TAG, "Characteristic UUID: %x", characteristic->uuid.u16.value);

    static bool has_read_chr = false;
    static bool has_write_chr = false;
    if (error->status != 0)
    {
        ESP_LOGE(TAG, "Characteristic discovery failed: %d", error->status);
        return 0;
    }

    if (characteristic->uuid.u16.value == HUB_WRITE_CHAR_UUID)
    {
        ESP_LOGI(TAG, "Writing characteristic...");
        has_write_chr = true;
        hub_device->write_char_handle = characteristic->val_handle;
        ESP_LOGI(TAG, "AFTER Writing");
    }
    else if (characteristic->uuid.u16.value == HUB_READ_CHAR_UUID)
    {
        ESP_LOGI(TAG, "Reading characteristic...");
        has_read_chr = true;
        hub_device->read_char_handle = characteristic->val_handle;
        ESP_LOGI(TAG, "AFTER READING");
    }

    if (has_read_chr && has_write_chr)
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

// write to hub
int send_data_to_hub(int sensor_temperature)
{
    ESP_LOGI(TAG, "IN SEND TO HUB FUNCTION");
    //  if WRITE_CHAR_HANDLE is not 0
    if (hub_device->write_char_handle != 0)
    {
        ESP_LOGI(TAG, "Temperature: %d °C", sensor_temperature);
        // print all device info field for debug
        ESP_LOGI(TAG, "Service UUID: %x", hub_device->service_uuid);
        ESP_LOGI(TAG, "Read Char Handle: %d", hub_device->read_char_handle);
        ESP_LOGI(TAG, "Write Char Handle: %d", hub_device->write_char_handle);
        ESP_LOGI(TAG, "Conn Handle: %d", hub_device->conn_handle);
        // write to hub
        int temp_data = 23;

        // Ensure the connection and handles are valid before attempting to write
        if (hub_device->conn_handle == 0)
        {
            ESP_LOGE(TAG, "Invalid connection handle");
            return -1;
        }

        if (hub_device->write_char_handle == 0)
        {
            ESP_LOGE(TAG, "Invalid write characteristic handle");
            return -1;
        }

        int retry_count = 0;
        int retry_delay = INITIAL_RETRY_DELAY_MS;
        int ret;

        do
        {
            vTaskDelay(pdMS_TO_TICKS(retry_delay));
            ret = ble_gattc_write_flat(hub_device->conn_handle, hub_device->write_char_handle,
                                       &temp_data, (uint16_t)sizeof(temp_data), write_cb, NULL);
            if (ret != 0)
            {
                ESP_LOGW(TAG, "Write failed, retrying... (attempt %d, error %d)", retry_count + 1, ret);
                retry_delay *= 2; // Exponential backoff
            }
            retry_count++;
        } while (ret != 0 && retry_count < MAX_RETRY_COUNT);

        if (ret != 0)
        {
            ESP_LOGE(TAG, "Write failed after %d attempts: %d", MAX_RETRY_COUNT, ret);
            return -1;
        }

    }
    return 0;
}