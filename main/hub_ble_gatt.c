#include "hub_ble_gatt.h"
#include "wifi_task.h"

static char *TAG = "BLE Server";

const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,  // Type of service
        .uuid = BLE_UUID16_DECLARE(0x0180),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0x1234),
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = hub_handle_temperature_read
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x5678),
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_PROP_WRITE,
                .access_cb = hub_handle_sensor_write
            },
            {0}  // End of characteristics array
        }
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x0185),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0x1234),
                .flags = BLE_GATT_CHR_F_WRITE,
                .access_cb = hub_handle_wifi_ssid_write
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x5678),
                .flags = BLE_GATT_CHR_F_WRITE,
                .access_cb = hub_handle_wifi_password_write
            },
            {0}  // End of characteristics array
        }
    },
    {0}  // End of services array
};



int hub_handle_temperature_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    ESP_LOGI("BLE_GATT", "Read operation on attribute handle: %d", attr_handle);
    char *value_string = "Sensor Temperature: ";
    os_mbuf_append(ctxt->om, value_string, strlen(value_string));
    return 0;
}

int hub_handle_sensor_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    ESP_LOGI("BLE_GATT", "Write operation on attribute handle: %d", attr_handle);
    
    if(ctxt->om->om_len == 0)
    {
        ESP_LOGE(TAG, "Empty data received");
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    ESP_LOGI(TAG, "Data length: %d", ctxt->om->om_len);
    ESP_LOGI(TAG, "Data: %s", (char *)ctxt->om->om_data);

    char temp[ctxt->om->om_len + 1];
    memcpy(temp, ctxt->om->om_data, ctxt->om->om_len);
    temp[ctxt->om->om_len] = '\0'; // Lägg till null-terminator
    
    //back to float
    float temperature = atof(temp);

    ESP_LOGI(TAG, "Received Temperature: %.2f °C", temperature);
    return 0;
}

int hub_handle_wifi_ssid_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char temp[ctxt->om->om_len + 1];
    memcpy(temp, ctxt->om->om_data, ctxt->om->om_len);
    temp[ctxt->om->om_len] = '\0'; // Lägg till null-terminator

    set_wifi_ssid(temp);
    ESP_LOGI(TAG, "SSID: %s", get_wifi_ssid());
    return 1;
}

int hub_handle_wifi_password_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char temp[ctxt->om->om_len + 1];
    memcpy(temp, ctxt->om->om_data, ctxt->om->om_len);
    temp[ctxt->om->om_len] = '\0'; // Lägg till null-terminator

    set_wifi_password(temp);
    ESP_LOGI(TAG, "Password: %s", get_wifi_password());
    return 1;
}
