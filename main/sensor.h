#ifndef SENSOR_H
#define SENSOR_H

#include "esp_err.h"

esp_err_t init_sensor();
float get_temp();
void sensor_task(void *pvParameter);

#endif