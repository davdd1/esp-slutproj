#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"s
#include "services/gatt/ble_svc_gatt.h"
#include "sdkconfig.h"
#include "driver/gpio.h"

uint8_t ble_addr_type;
static char *TAG = "BLE Server";
void ble_app_advertise(void);

static int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
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

static int device_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *value_string = "Sensor Value: ";
    os_mbuf_append(ctxt->om, value_string, strlen(value_string));
    return 0;
}

static int device_notify(uint16_t conn_handle, uint16_t attr_handle)
{
    int8_t notification_data = 1; // temp for testing
    struct os_mbuf *om = ble_hs_mbuf_from_flat(&notification_data, sizeof(notification_data));
    ble_gatts_notify_custom(conn_handle, attr_handle, om);
    return 0;
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "BLE GAP EVENT CONNECTION %s", event->connect.status == 0 ? "OK" : "FAILED");

        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
        // Advertise again
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "BLE GAP EVENT DISCONNECTED");
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "GRATTIS GUBBEN! NY SUB: Attr handle: %d", event->subscribe.attr_handle);
        if (event->subscribe.cur_notify)
        {
            ESP_LOGI(TAG, "NOTIFICATION ENABLED");
            device_notify(event->subscribe.conn_handle, event->subscribe.attr_handle);
            break;
        }
        break;
    default:
        break;
    }
    return 0;
}

// Define BLE Connection
void ble_app_advertise()
{

    // GAP - Device name def
    struct ble_hs_adv_fields adv_fields;
    const char *device_name;
    memset(&adv_fields, 0, sizeof(adv_fields));
    device_name = ble_svc_gap_device_name();
    adv_fields.name = (uint8_t *)device_name;
    adv_fields.name_len = strlen(device_name);
    adv_fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&adv_fields);

    // GAP - Device Connectivity
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY, // type of service
     .uuid = BLE_UUID16_DECLARE(0x180),
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(0xFEF4),
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = device_read},
         {.uuid = BLE_UUID16_DECLARE(0xDEAD),
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
         {.uuid = BLE_UUID16_DECLARE(0xAEED),
          .flags = BLE_GATT_CHR_F_NOTIFY,
          .access_cb = device_notify},
         {0}}},
    {0}};

void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_app_advertise();
}

void host_task(void *params)
{
    nimble_port_run();
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    // Intialize NVS
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);                            // Check if NVS intialization is successful
    nimble_port_init();                              // Initialize the host stack
    ble_svc_gap_device_name_set("Daves BLE-Server"); // Initialize NimBLE configuration - server name
    ble_svc_gap_init();                              // Initialize NimBLE configuration - gap service
    ble_svc_gatt_init();                             // Initialize NimBLE configuration - gatt service
    ble_gatts_count_cfg(gatt_svcs);                  // Initialize NimBLE configuration - config gatt services
    ble_gatts_add_svcs(gatt_svcs);                   // Initialize NimBLE configuration - queues gatt services.
    ble_hs_cfg.sync_cb = ble_app_on_sync;            // Initialize application
    nimble_port_freertos_init(host_task);            // Run the thread
}