#include "ble_gatt.h"
#include "sensor.h"
#include "ds18b20.h"
#include "onewire_bus.h"
#include "onewire_bus_impl_rmt.h"
#include "onewire_bus_interface.h"
#include "onewire_crc.h"
#include "onewire_cmd.h"
#include "onewire_device.h"
#include "onewire_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define GPIO_PIN 2

const char* tag = "SENSOR";

float temperature;
bool is_sensor_initialized = false;  // Flag to check if sensor is initialized
onewire_bus_handle_t bus = NULL;     // Global bus handle
ds18b20_device_handle_t ds18b20 = NULL;  // Global sensor handle

// Initialize the sensor (only once)
esp_err_t init_sensor() {
    // Install 1-wire bus
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = GPIO_PIN,
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10,  // 1byte ROM command + 8byte ROM number + 1byte device command
    };
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus));

    // Create 1-wire device iterator for device search
    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t next_onewire_device;
    esp_err_t search_result = ESP_OK;

    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));
    
    // Search for DS18B20 device
    while ((search_result = onewire_device_iter_get_next(iter, &next_onewire_device)) == ESP_OK) {
        ds18b20_config_t ds_cfg = {};
        // Check if the device is a DS18B20
        if (ds18b20_new_device(&next_onewire_device, &ds_cfg, &ds18b20) == ESP_OK) {
            ESP_LOGI(tag, "DS18B20 sensor found and initialized");
            break;
        }
    }

    ESP_ERROR_CHECK(onewire_del_device_iter(iter));

    if (ds18b20 == NULL) {
        ESP_LOGE(tag, "Failed to initialize DS18B20 sensor");
        return ESP_FAIL;
    }

    is_sensor_initialized = true;  // Set the flag to true after initialization
    return ESP_OK;
}

// Get temperature from sensor
float get_temp() {
    if (!is_sensor_initialized) {
        // Initialize the sensor if it hasn't been initialized yet
        if (init_sensor() != ESP_OK) {
            ESP_LOGE(tag, "Sensor initialization failed");
            return -1.0;
        }
    }

    // Trigger temperature conversion
    ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(ds18b20));

    // Get the temperature value
    ESP_ERROR_CHECK(ds18b20_get_temperature(ds18b20, &temperature));

    return temperature;
}

// Task to get temperature from sensor to send to server
void sensor_task(void *pvParameter) {
    while (1) {
        temperature = get_temp();
        int ret = send_data_to_hub((int)temperature);  // Send the temperature value to the server
        if (ret != 0) {
            ESP_LOGE(tag, "Failed to send data to server");
        }
        // Log the temperature value in celcius
        ESP_LOGI(tag, "Temperature: %d °C", (int)temperature);
        vTaskDelay(8000 / portTICK_PERIOD_MS);
    }
}
