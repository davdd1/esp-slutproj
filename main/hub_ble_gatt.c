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
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = hub_handle_led_command},
         {0} // End of characteristics array
     }},
     // Service for wifi credentials
    {.type = BLE_GATT_SVC_TYPE_PRIMARY, 
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

int hub_handle_led_command(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
    {
        int rc = os_mbuf_append(ctxt->om, led_command_buffer, strlen(led_command_buffer));
        ESP_LOGW(TAG, "LED command read by client: %s", led_command_buffer);
        if (rc != 0)
        {
            ESP_LOGE("LED Command", "Failed to send LED command");
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }

        read_led_command_from_server();

        return 0;
    } else {
        ESP_LOGI("LED Command", "Connection invalid!");
        return -1;
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
    return 0;
}

int hub_handle_temperature_data(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->om->om_len <= 0)
    {
        ESP_LOGE(TAG, "Empty sensor data received");
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
        char temp[ctxt->om->om_len + 1];
        memcpy(temp, ctxt->om->om_data, ctxt->om->om_len);
        temp[ctxt->om->om_len] = '\0'; // Lägg till null-terminator
        enqueue_temperature(temp); // Add to queue
    }

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
