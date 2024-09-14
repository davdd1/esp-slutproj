#include "hub_ble_gap.h"

static char* TAG = "BLE Server";
uint8_t ble_addr_type;

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

    // GAP - Device Connectivity def
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_app_advertise();
}

void host_task(void *params)
{
    nimble_port_run();
}