#include "hub_server_comm.h"
#include "ble_task.h"
#include "hub_ble_gatt.h"
#include "wifi_task.h"

static const char *TAG = "HUB-TCP";
char led_command_buffer[64] = "White"; // Buffer for the LED command, default value is "LED:0,0,0"

// Initialize a TCP connection to the server
static int connect_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
    };

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        ESP_LOGE(TAG, "Invalid server address");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to connect to server");
        close(sock);
        return -1;
    }

    return sock;
}

bool connect_to_server_with_retries(int retries, int delay_ms, int *sock) {
    for (int i = 0; i < retries; i++) {
        *sock = connect_to_server();
        if (*sock != -1) {
            return true;
        }
        ESP_LOGW(TAG, "Retrying connection... (%d/%d)", i + 1, retries);
        vTaskDelay(delay_ms / portTICK_PERIOD_MS);
    }
    ESP_LOGE(TAG, "Failed to connect to server after %d retries", retries);
    return false;
}

// Read data (LED command) from the GO server
void read_led_command_from_server() {
    int sock;
    if (ble_gap_conn_handle != BLE_HS_CONN_HANDLE_NONE && has_wifi_connected) {
        if (!connect_to_server_with_retries(5, 1000, &sock)) {
            ESP_LOGE(TAG, "Failed to connect to server");
            return;
        }
        
        char recv_buffer[64];
        int len = recv(sock, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (len > 0) {
            recv_buffer[len] = 0; // Null-terminate received data
            ESP_LOGI(TAG, "Received command from server: %s", recv_buffer);
            strncpy(led_command_buffer, recv_buffer, sizeof(led_command_buffer) - 1);
            led_command_buffer[sizeof(led_command_buffer) - 1] = '\0'; // Null-terminate the buffer
        } else if (len == 0) {
            //ESP_LOGE(TAG, "Server closed the connection");
        } else {
            ESP_LOGE(TAG, "Failed to receive data");
        }

        close(sock); // Close the socket after use
    }
}

// Send data to the server
void send_to_server(int prefix, const char *data) {
    int sock = connect_to_server();
    if (sock == -1) {
        ESP_LOGE(TAG, "Failed to connect to server");
        return;
    }

    char message[256];
    int msg_len = snprintf(message, sizeof(message), "%d%s", prefix, data);

    int bytes_sent = send(sock, message, msg_len, 0);
    if (bytes_sent < 0) {
        ESP_LOGE(TAG, "Failed to send data");
    } else {
        ESP_LOGI(TAG, "Data sent: %s", message);

        char recv_buffer[128];
        int len = recv(sock, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (len > 0) {
            recv_buffer[len] = 0; // Null-terminate received data
            ESP_LOGI(TAG, "Received from server: %s", recv_buffer);
        } else if (len == 0) {
            ESP_LOGE(TAG, "Server closed the connection");
        } else {
            ESP_LOGE(TAG, "Failed to receive data");
        }
    }

    close(sock); // Close the socket after use
}
