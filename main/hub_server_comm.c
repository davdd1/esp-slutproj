#include "hub_server_comm.h"


static const char* TAG = "HUB-TCP";

void send_to_server(int prefix, const char* data) {
    int sock = 0;
    struct sockaddr_in server_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket creation failed");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        ESP_LOGI(TAG, "Invalid address / not supported");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Connection to server failed");
        close(sock);
        return;
    }

    // update messsage with prefix
    char message[256];
    int msg_len = snprintf(message, sizeof(message), "%d%s", prefix, data);

    //send raw message with prefix
    send(sock, message, msg_len, 0);
    ESP_LOGI(TAG, "Data sent: %s", message);

    char recv_buffer[128];
    int len = recv(sock, recv_buffer, sizeof(recv_buffer) - 1, 0);
    if (len > 0) {
       recv_buffer[len] = 0; //null terminate received data
       ESP_LOGI(TAG, "Received from server: %s", recv_buffer); 
    }

    close(sock);
}
