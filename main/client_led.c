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

        xTaskCreate(led_task, "led_task", 2048, NULL, 6, NULL);
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

    ESP_LOGI(TAG, "Set LED color: R=%d, G=%d, B=%d", red, green, blue);
}

void turn_off_led()
{
    if (led_strip_clear(led_strip) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to clear LED strip");
    }
}