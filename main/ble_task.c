#include "ble_task.h"
#include "ble_gap.h"
#include "ble_gatt.h"

static char *TAG = "BLE Server";
uint8_t ble_addr_type;

static uint8_t temp[] = {0x08, 0x09, 0x4a, 0x65, 0x73, 0x70, 0x48, 0x75, 0x62};

static int gattc_read(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg) {
    if (error->status == 0) {
        ESP_LOGI(TAG, "Read successful! Attribute handle: %d", attr->handle);
        ESP_LOG_BUFFER_HEX(TAG, attr->om->om_data, attr->om->om_len);
    } else {
        ESP_LOGE(TAG, "Read failed, error: %d", error->status);
    }
    return 0;
}

static int disc_chrs(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *characteristic, void *arg)
{
    if (error->status == 0) {
        ESP_LOGI(TAG, "Discovered characteristic UUID: %04x, handle: %d, properties: %02x",
                 characteristic->uuid.u16.value, characteristic->val_handle, characteristic->properties);
        
        if (characteristic->properties & BLE_GATT_CHR_PROP_READ) {
            int ret = ble_gattc_read(conn_handle, characteristic->val_handle, gattc_read, NULL);
            if (ret != 0) {
                ESP_LOGE(TAG, "Read request failed: %d", ret);
            }
        } else {
            ESP_LOGI(TAG, "Characteristic is not readable");
        }
    } else if (error->status == BLE_HS_EDONE) {
        ESP_LOGI(TAG, "Characteristic discovery completed");
    } else {
        ESP_LOGE(TAG, "Characteristic discovery failed: %d", error->status);
    }
    return 0;
}

static int disc_svcs(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg)
{
    if (error->status == 0) {
        ESP_LOGI(TAG, "Discovered service UUID: %04x, start handle: %d, end handle: %d",
                 service->uuid.u16.value, service->start_handle, service->end_handle);
        
        int ret = ble_gattc_disc_all_chrs(conn_handle, service->start_handle, service->end_handle, disc_chrs, NULL);
        if (ret != 0) {
            ESP_LOGE(TAG, "Characteristic discovery failed to start: %d", ret);
        }
    } else if (error->status == BLE_HS_EDONE) {
        ESP_LOGI(TAG, "Service discovery completed");
    } else {
        ESP_LOGE(TAG, "Service discovery failed: %d", error->status);
    }
    return 0;
}

int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_DISC:
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
        vTaskDelay(pdMS_TO_TICKS(100));
        break;

    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0)
        {
            ESP_LOGI(TAG, "Connection established!");
            ble_gattc_disc_all_svcs(event->connect.conn_handle, disc_svcs, NULL);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to connect: %d", event->connect.status);
            ble_scan();
        }
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