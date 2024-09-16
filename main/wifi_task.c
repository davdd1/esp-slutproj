#include "wifi_task.h"
#include <stdbool.h>

bool is_ssid_set = false;
bool is_pass_set = false;
bool has_wifi_init = false;
bool has_wifi_connected = false;

static const char* TAG = "WIFI";

wifi_config_t wifi_config = {
    .sta = {
        .ssid = {0},
        .password = {0},
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
    },
};


// Event handler for WiFi and IP events
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();  // Connect when WiFi starts
        ESP_LOGI(TAG, "WiFi started, attempting to connect...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        has_wifi_connected = false;  // Mark as disconnected
        ESP_LOGI(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();  // Reconnect automatically
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        has_wifi_connected = true;  // Mark as connected
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Subnet mask: " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
    }
}

void wifi_init()
{
    static esp_netif_t* netif = NULL;
    // if first time, initialize NVS
    if (!has_wifi_init)
    {
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        netif = esp_netif_create_default_wifi_sta();
        if (netif == NULL)
        {
            ESP_LOGE("WIFI", "Failed to create default wifi netif");
            return;
        }

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        //register event handler
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        has_wifi_init = true;
    }
    else
    {
        ESP_LOGI("WIFI", "Reconnecting with new config...");
        esp_wifi_disconnect();
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_connect());
        has_wifi_connected = false;
    }


    ESP_LOGI("WIFI", "SSID: %s", wifi_config.sta.ssid);
    ESP_LOGI("WIFI", "Password: %s", wifi_config.sta.password);

//     ESP_ERROR_CHECK(esp_wifi_connect());

//     esp_netif_ip_info_t ip_info;
//     int retries = 0;
//     const int max_retries = 10;

//     // wait for connection
//     while (retries < max_retries)
//     {
//         ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip_info));
//         if (ip_info.ip.addr != 0)
//         {
//             ESP_LOGI("WIFI", "IP Address: " IPSTR, IP2STR(&ip_info.ip));
//             ESP_LOGI("WIFI", "Subnet mask: " IPSTR, IP2STR(&ip_info.netmask));
//             ESP_LOGI("WIFI", "Gateway: " IPSTR, IP2STR(&ip_info.gw));
//             is_ssid_set = false;
//             is_pass_set = false;
//             //has_wifi_connected = true;
//             ESP_LOGE("WIFI", "WIFI CONNECTED: %d", has_wifi_connected);
//             break;
//         }
//         ESP_LOGI("WIFI", "Waiting for IP... %d", retries);
//         vTaskDelay(pdMS_TO_TICKS(1000)); // wait 1 sec
//         retries++;
//     }

//     if (retries >= max_retries)
//     {
//         ESP_LOGE("WIFI", "Failed to connect to AP after %d retries", max_retries);
//     }
//     // LOGGA
}

void set_wifi_ssid(const char *ssid)
{
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    is_ssid_set = true;
}

void set_wifi_password(const char *password)
{
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    is_pass_set = true;
}

const char *get_wifi_ssid()
{
    return (const char *)wifi_config.sta.ssid;
}

const char *get_wifi_password()
{
    return (const char *)wifi_config.sta.password;
}