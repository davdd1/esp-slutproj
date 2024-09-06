#include "ble_gatt.h"

static char *TAG = "BLE Server";

const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,  // Type of service
        .uuid = BLE_UUID16_DECLARE(0x0180),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0xFEF4),
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = device_read
            },
            {
                .uuid = BLE_UUID16_DECLARE(0xDEAD),
                .flags = BLE_GATT_CHR_F_WRITE,
                .access_cb = device_write
            },
            {
                .uuid = BLE_UUID16_DECLARE(0xAEED),
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .access_cb = device_notify
            },
            {0}  // End of characteristics array
        },
    },
    {0}  // End of services array
};


int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char temp[ctxt->om->om_len + 1];
    memcpy(temp, ctxt->om->om_data, ctxt->om->om_len);
    temp[ctxt->om->om_len] = '\0'; // Lägg till null-terminator

    char *hello = "Hello";
    if (strcmp(temp, hello) == 0)
    {
        ESP_LOGI(TAG, "Hello there!");
        return 1;
    }
    else
    {
        ESP_LOGE(TAG, "Goodbye.");
    }
    return 0;
}

int device_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *value_string = "Sensor Value: ";
    os_mbuf_append(ctxt->om, value_string, strlen(value_string));
    return 0;
}

int device_notify(uint16_t conn_handle, uint16_t attr_handle)
{
    int8_t notification_data = 1; // temp for testing
    struct os_mbuf *om = ble_hs_mbuf_from_flat(&notification_data, sizeof(notification_data));
    ble_gatts_notify_custom(conn_handle, attr_handle, om);
    return 0;
}
