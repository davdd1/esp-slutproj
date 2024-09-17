#ifndef HUB_QUEUE_H
#define HUB_QUEUE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "wifi_task.h"

#define QUEUE_LENGTH 30

typedef enum {
    ACTION_SEND_TEMPERATURE,
    ACTION_SEND_MAC_ADDRESS
} action_type_t;

typedef struct {
    action_type_t action_type; // type of data
    union {
        //temp and mac address
        char temperature[32];
        uint8_t mac_address[6];
    } data;
} queue_item_t;

void init_queue();
void process_queue_task(void* pvParameters);
void enqueue_temperature(const char* temperature);
void enqueue_mac_address(const uint8_t* mac_address);


#endif