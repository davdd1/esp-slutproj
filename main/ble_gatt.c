#include "ble_gatt.h"
#include "sensor.h"

static char *TAG = "BLE Server";

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
    return 0;
}

int device_notify(uint16_t conn_handle, uint16_t attr_handle)
{
    int8_t notification_data = 1; // temp for testing
    struct os_mbuf *om = ble_hs_mbuf_from_flat(&notification_data, sizeof(notification_data));
    ble_gatts_notify_custom(conn_handle, attr_handle, om);
    return 0;
}
