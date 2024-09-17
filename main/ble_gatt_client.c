#include "ble_gatt_client.h"
#include "ds18b20_sensor.h"
#include "client_led.h"
#include "esp_mac.h"

static char *TAG = "BLE_GATT_CLIENT";

ble_device_info_t *hub_device;
bool has_found_all_chrs = false;
bool has_found_svc = false;
static bool has_device_id_chr = false;
static bool has_temp_data_chr = false;
static bool has_led_chr = false;

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
        led_init();
    }
}

void led_task(void* pvParameters) {
    while (!has_found_all_chrs) {
        ESP_LOGI("LED_Task", "Wait for service discovery to complete");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Service discovery complete. LED task started");

    while (1) {
        ESP_LOGI("LED_Task", "Listening for LED commands");
        if(hub_device->led_char_handle != 0)  {
        int ret = read_led_command_from_hub();
        if (ret != 0) {
            ESP_LOGI("LED_Task", "Failed to read LED command: %d", ret);
        } else {
            ESP_LOGI("LED_Task", "Read LED command successfully");
        }
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI("LED_Task", "LED task complete");
    vTaskDelete(NULL);
}

void device_task(void *pvParameters)
{
    while (!has_found_all_chrs)
    {
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }

    while (1)
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
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 10 seconds
    }
    ESP_LOGI(TAG, "Device task completed");
    vTaskDelete(NULL);
}

int disc_chrs(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *characteristic, void* arg)
{
    if (has_found_all_chrs)
    {
        ESP_LOGI(TAG, "Characteristic discovery already completed.");
        return 0;
    }
    if (characteristic == NULL)
    {
        ESP_LOGE(TAG, "Characteristic is NULL");
        return -1; // error
    }

    if (error->status != 0)
    {
        ESP_LOGE(TAG, "Characteristic discovery failed: %d", error->status);
        return 0;
    }

    ESP_LOGI(TAG, "Characteristic UUID: %x", characteristic->uuid.u16.value);

    if (characteristic->uuid.u16.value == HUB_DEVICE_ID_CHAR_UUID)
    {
        has_device_id_chr = true;
        hub_device->device_id_char_handle = characteristic->val_handle;
    }
    else if (characteristic->uuid.u16.value == HUB_TEMP_DATA_CHAR_UUID)
    {
        has_temp_data_chr = true;
        hub_device->temp_data_char_handle = characteristic->val_handle;
    }
    else if (characteristic->uuid.u16.value == HUB_LED_CHAR_UUID)
    {
        has_led_chr = true;
        hub_device->led_char_handle = characteristic->val_handle;
    }

    // Check if all characteristics have been found and log
    ESP_LOGW(TAG, "has_temp_data_chr: %d, has_device_id_chr: %d, has_led_chr: %d", has_temp_data_chr, has_device_id_chr, has_led_chr);
    if (has_temp_data_chr && has_device_id_chr && has_led_chr)
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

// Callback function for reading LED command from hub and setting the LED color
void read_cb(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
    if (error->status == 0)
    {
        char command[64];
        memcpy(command, attr->om->om_data, attr->om->om_len);
        command[attr->om->om_len] = '\0';

        ESP_LOGI(TAG, "Received LED command: %s", command);

    // Check for specific color names
        if (strcasecmp(command, "Green") == 0)
        {
            set_led_color(0, 255, 0); // Set LED to Green (R=0, G=255, B=0)
            ESP_LOGI(TAG, "Set LED to Green");
        }
        else if (strcasecmp(command, "Red") == 0)
        {
            set_led_color(255, 0, 0); // Set LED to Red (R=255, G=0, B=0)
            ESP_LOGI(TAG, "Set LED to Red");
        }
        else if (strcasecmp(command, "Blue") == 0)
        {
            set_led_color(0, 0, 255); // Set LED to Blue (R=0, G=0, B=255)
            ESP_LOGI(TAG, "Set LED to Blue");
        }
        else if (strcasecmp(command, "White") == 0) 
        {
            set_led_color(255, 255, 255); //white
            ESP_LOGI(TAG, "Set LED to White");
        }
        else
        {
            ESP_LOGW(TAG, "Unknown color command: %s", command);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Read LED command failed, error: %d", error->status);
    }
}

int read_led_command_from_hub() {
    if (hub_device->led_char_handle != 0)
    {
        int rc = ble_gattc_read(hub_device->conn_handle, hub_device->led_char_handle, read_cb, NULL);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "Read failed: %d", rc);
            return -1;
        } ;
    } else {
        ESP_LOGE(TAG, "Invalid LED command characteristic");
        return -1;
    }
    return 0;
}

// send device id(mac addr) to hub
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

// check prefix and write to hub
int send_data_handler(int prefix, float sensor_temperature)
{

    ESP_LOGI(TAG, "Temperature: %.2f °C", sensor_temperature);
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
    default:
        ESP_LOGE(TAG, "Invalid prefix: %d", prefix);
        return -1;
    }
    return 0;
}