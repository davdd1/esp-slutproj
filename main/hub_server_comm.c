#include "hub_server_comm.h"
#include "ble_task.h"
#include "hub_ble_gatt.h"

static const char* TAG = "HUB-TCP";
static int sock = -1;  // Persistent socket for communication

// Initialize a TCP connection to the server
static bool connect_to_server() {
    struct sockaddr_in server_addr;

    if (sock != -1) {
        // Socket already exists, check if it's still valid
        return true;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket creation failed");
        return false;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        ESP_LOGE(TAG, "Invalid address / not supported");
        close(sock);
        sock = -1;
        return false;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Connection to server failed");
        close(sock);
        sock = -1;
        return false;
    }

    ESP_LOGI(TAG, "Connected to server: %s:%d", SERVER_IP, SERVER_PORT);
    return true;
}

// Read data (Led command) from the the GO server
void read_led_command_from_server() {
    if (!connect_to_server()) {
        ESP_LOGE(TAG, "Failed to connect to server");
        return;
    }

    char recv_buffer[64];
    int len = recv(sock, recv_buffer, sizeof(recv_buffer) - 1, 0);
    if (len > 0) {
        recv_buffer[len] = 0;  // Null-terminate received data
        ESP_LOGI(TAG, "Received command from server: %s", recv_buffer);
        
        //notify the ble client with the command
        notify_led_command(recv_buffer);
    } else if (len == 0) {
        ESP_LOGE(TAG, "Server closed the connection");
        close(sock);
        sock = -1;
    } else {
        ESP_LOGE(TAG, "Failed to receive data");
        close(sock);
        sock = -1;
    }
}

// Send data to the server
void send_to_server(int prefix, const char* data) {
    if (!connect_to_server()) {
        ESP_LOGE(TAG, "Failed to connect to server");
        return;
    }

    // Prepare the message with the prefix
    char message[256];
    int msg_len = snprintf(message, sizeof(message), "%d%s", prefix, data);

    // Send the message
    int bytes_sent = send(sock, message, msg_len, 0);
    if (bytes_sent < 0) {
        ESP_LOGE(TAG, "Failed to send data, attempting to reconnect...");
        close(sock);
        sock = -1;

        // Try to reconnect and resend the data
        if (!connect_to_server()) {
            ESP_LOGE(TAG, "Reconnection failed");
            return;
        }

        bytes_sent = send(sock, message, msg_len, 0);
        if (bytes_sent < 0) {
            ESP_LOGE(TAG, "Failed to send data after reconnection");
            close(sock);
            sock = -1;
            return;
        }
    }

    ESP_LOGI(TAG, "Data sent: %s", message);

    // Receive response from the server (optional)
    char recv_buffer[128];
    int len = recv(sock, recv_buffer, sizeof(recv_buffer) - 1, 0);
    if (len > 0) {
        recv_buffer[len] = 0;  // Null-terminate received data
        ESP_LOGI(TAG, "Received from server: %s", recv_buffer);
    } else if (len == 0) {
        ESP_LOGE(TAG, "Server closed the connection");
        close(sock);
        sock = -1;
    } else {
        ESP_LOGE(TAG, "Failed to receive data");
        close(sock);
        sock = -1;
    }
}

// Close the persistent connection when needed
void close_connection() {
    if (sock != -1) {
        close(sock);
        sock = -1;
        ESP_LOGI(TAG, "Connection closed");
    }
}
