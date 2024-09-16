#include "ble_task.h"
#include "hub_ble_gatt.h"
#include "wifi_task.h"
#include "hub_server_comm.h"
#include "hub_queue.h"


static char *TAG = "BLE HUB";

const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY, // Type of service
     .uuid = BLE_UUID16_DECLARE(0x0180),
     .characteristics = (struct ble_gatt_chr_def[]){
         {// Char for Device id (prefix 1)
          .uuid = BLE_UUID16_DECLARE(0x1234),
          .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
          .access_cb = hub_handle_device_id},
         {// Char for Temp data (pref 2)
          .uuid = BLE_UUID16_DECLARE(0x5678),
          .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
          .access_cb = hub_handle_temperature_data},
         {// Char for LED command
          .uuid = BLE_UUID16_DECLARE(0x9ABC),
          .flags = BLE_GATT_CHR_F_NOTIFY,
          .access_cb = notify_led_command,
          .descriptors = (struct ble_gatt_dsc_def[]){
              {.uuid = BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_CFG_UUID16), // CCCD UUID
               .att_flags = BLE_ATT_F_READ | BLE_ATT_F_WRITE, // Read and write permissions
               .access_cb = dsc_test
              },
              {0} // End of descriptors array
          }
         },
         {0} // End of characteristics array
     }},
    {
     .type = BLE_GATT_SVC_TYPE_PRIMARY, 
     .uuid = BLE_UUID16_DECLARE(0x0185), 
     .characteristics = (struct ble_gatt_chr_def[]){                            
        {
         .uuid = BLE_UUID16_DECLARE(0x4321), 
         .flags = BLE_GATT_CHR_F_WRITE, 
         .access_cb = hub_handle_wifi_ssid_write}, 
        {
         .uuid = BLE_UUID16_DECLARE(0x8765), 
         .flags = BLE_GATT_CHR_F_WRITE, 
         .access_cb = hub_handle_wifi_password_write}, 
         {0} // End of characteristics array  
    }},
    {0} // End of services array
};

void dsc_test() {
    ESP_LOGI(TAG, "Descriptor accessed");
}

void notify_led_command(const char* command) {
    // Check if connection valid
    if (ble_gap_conn_handle == BLE_HS_CONN_HANDLE_NONE)
    {
        ESP_LOGE(TAG, "No connection to notify");
        return;
    }

    struct os_mbuf* om;
    uint16_t def_handle;
    uint16_t val_handle;
    const ble_uuid_t *svc_uuid = BLE_UUID16_DECLARE(0x0180);  // Your service UUID
    const ble_uuid_t *chr_uuid = BLE_UUID16_DECLARE(0x9ABC);  // Your LED command characteristic UUID

    //Get the handle of the LED command characteristic
    int rc = ble_gatts_find_chr(svc_uuid, chr_uuid, &def_handle, &val_handle);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Failed to find LED command characteristic: %d", rc);
        return;
    }

    // Create the notification data (ex: "LED:255,0,0")
    om = ble_hs_mbuf_from_flat(command, strlen(command));
    if (om == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mbuf for notification");
        return;
    }

    // Notify the connected client
    int ret = ble_gatts_notify_custom(ble_gap_conn_handle, val_handle, om);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "Failed to notify LED command: %d", ret);
    } else {
        ESP_LOGI(TAG, "Notified LED command: %s", command);
    }
}

int hub_handle_device_id(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    ESP_LOGI("BLE_GATT", "Device ID characteristic accessed");
    if (ctxt->om->om_len <= 0)
    {
        ESP_LOGI(TAG, "Empty Device ID received");
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
        if (ctxt->om->om_len == 6)
        {
            uint8_t device_id[6];
            memcpy(device_id, ctxt->om->om_data, 6);
            ESP_LOGI("Device ID", "Received MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                     device_id[0], device_id[1], device_id[2], device_id[3], device_id[4], device_id[5]);
            enqueue_mac_address(device_id);
        }
        else
        {
            ESP_LOGE("Device ID", "Invalid MAC address length received: %d", ctxt->om->om_len);
        }
    }
    memset(ctxt->om->om_data, 0, ctxt->om->om_len); // Clear the buffer
    return 0;
}

int hub_handle_temperature_data(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{

    ESP_LOGI("BLE_GATT", "Temperature characteristic accessed");
    if (ctxt->om->om_len <= 0)
    {
        ESP_LOGE(TAG, "Empty sensor data received");
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
    { // clear the buffer
        char temp[ctxt->om->om_len + 1];
        memcpy(temp, ctxt->om->om_data, ctxt->om->om_len);
        temp[ctxt->om->om_len] = '\0'; // Lägg till null-terminator
        ESP_LOGI("Temperature", "Received: %s °C", temp);
        enqueue_temperature(temp); // Add to queue
    }

    memset(ctxt->om->om_data, 0, ctxt->om->om_len);
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
