#include "client_led.h"
#include "ble_gatt_client.h"
#include "esp_log.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "led_strip_types.h"
#include "driver/rmt.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "LED_HANDLER";
static led_strip_handle_t led_strip = NULL; // LED strip handle

void led_init()
{
    if (led_strip == NULL)
    {
        led_strip_config_t strip_config = {
            .strip_gpio_num = LED_STRIP_GPIO,
            .max_leds = 1,
            .led_pixel_format = LED_PIXEL_FORMAT_GRB,
            .led_model = LED_MODEL_WS2812,
            .flags.invert_out = false,
        };
        led_strip_rmt_config_t rmt_config = {
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .resolution_hz = 10 * 1000 * 1000, // 10 MHz
            .flags.with_dma = false,
        };

        // Skapa LED-strip objektet
        if (led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to create LED strip");
            return;
        }

        // Rensa LED-stripen
        if (led_strip_clear(led_strip) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to clear LED strip");
        }
    }
}
// Set the color of the LED strip
void set_led_color(int red, int green, int blue)
{
    if (led_strip == NULL)
    {
        ESP_LOGE(TAG, "LED strip not initialized");
        return;
    }

    // Set the color of the LED strip
    if (led_strip_set_pixel(led_strip, 0, red, green, blue) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set LED pixel color");
        return;
    }

    // Refresh the LED strip to show the color
    if (led_strip_refresh(led_strip) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to show LED color");
    }
}
// BLE notification handler for LED commands
static void ble_notification_handler(uint16_t conn_handle, struct ble_gatt_attr *attr, const struct ble_gatt_error *error, void *arg)
{
    if (error->status != 0)
    {
        ESP_LOGE(TAG, "BLE notification failed, error: %d", error->status);
        return;
    }

    // Parse the received LED command
    char recv_buffer[64];
    memcpy(recv_buffer, attr->om->om_data, attr->om->om_len);
    recv_buffer[attr->om->om_len] = '\0'; // Null-terminate the string

    ESP_LOGI(TAG, "Received LED command: %s", recv_buffer);

    // Handle the command, e.g., "LED:255,0,0"
    if (strncmp(recv_buffer, "LED:", 4) == 0)
    {
        int red, green, blue;
        sscanf(recv_buffer + 4, "%d,%d,%d", &red, &green, &blue);

        // Set the LED color on the ESP32 based on the received command
        set_led_color(red, green, blue);
    }
}

void turn_off_led()
{
    if (led_strip_clear(led_strip) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to clear LED strip");
    }
}