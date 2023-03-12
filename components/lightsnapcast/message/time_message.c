#include "time_message.h"

#include <buffer.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "SNAPCLIENT_TIME_MESSAGE";

int time_message_serialize(time_message_t *msg, char *data) {
    write_buffer_t buffer;
    int            result = 0;

    buffer_write_init(&buffer, data, sizeof(time_message_t));
    

    result |= buffer_write_int32(&buffer, msg->latency.sec);
    result |= buffer_write_int32(&buffer, msg->latency.usec);

    return result;
}

int time_message_deserialize(time_message_t *msg, const char *data) {
    read_buffer_t buffer;
    int           result = 0;

    buffer_read_init(&buffer, data, sizeof(time_message_t));

    result |= buffer_read_int32(&buffer, &(msg->latency.sec));
    result |= buffer_read_int32(&buffer, &(msg->latency.usec));

    return result;
}


int time_message_full_deserialize(time_message_full_t *msg, const char *data) {
    memcpy(msg, data, sizeof(time_message_full_t));
    return 0;
}

void print_time_message(const time_message_full_t* msg){
    print_base_message(&msg->base);
    ESP_LOGI(TAG, "timing: { sec: %d, usec: %d }\n", 
      msg->latency.sec,
      msg->latency.usec
    );
}