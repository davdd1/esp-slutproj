#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

#include "esp_err.h"

esp_err_t init_sensor();
float get_temp();
void sensor_task(void *pvParameter);

#endif