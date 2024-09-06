#include "ble_gap.h"
#include "ble_task.h"

static char* TAG = "BLE Server";

void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_scan();
}

void host_task(void *params)
{
    nimble_port_run();
}