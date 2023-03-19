#include "time_message.h"

#include <buffer.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "SNAPCLIENT_TIME_MESSAGE";

int time_message_deserialize(time_message_t *msg, const char *data) {
    memcpy(msg, data, sizeof(time_message_t));
    return 0;
}

int time_message_full_deserialize(time_message_full_t *msg, const char *data) {
    memcpy(msg, data, sizeof(time_message_full_t));
    return 0;
}

int time_message_full_serialize(time_message_full_t *msg, char *data) {
    memcpy(data, msg, sizeof(time_message_full_t));
    return 0;
}

void print_time_full_message(const time_message_full_t* msg){
    ESP_LOGI(TAG, "Pointor %p %p\n", msg, &msg->base); 
    print_base_message(&(msg->base));
    ESP_LOGI(TAG, "timing: { sec: %d, usec: %d }\n", 
      msg->latency.sec,
      msg->latency.usec
    );
}

void print_time_message(const time_message_t* msg){
    ESP_LOGI(TAG, "timing: { sec: %d, usec: %d }\n", 
      msg->latency.sec,
      msg->latency.usec
    );
}