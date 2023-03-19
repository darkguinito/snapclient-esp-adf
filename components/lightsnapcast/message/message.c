#include "message.h"

#include <string.h>
#include "esp_log.h"

const int BASE_MESSAGE_SIZE = 26;
const int TIME_MESSAGE_SIZE = 8;

static const char *TAG = "SNAPCLIENT_BASE_MESSAGE";

int base_message_serialize(base_message_t *msg, char *data) {
    memcpy(data, msg, sizeof(base_message_t));
    return 0;
}

int base_message_deserialize(base_message_t *msg, const char *data) {
    memcpy(msg, data, sizeof(base_message_t));
    return 0;
}


void print_base_message(const base_message_t* msg){
    ESP_LOGI(TAG, "msg: { type: %d, id: %d, refersTo: %d, sent: { sec: %d, usec: %d }, received: { sec: %d, usec: %d }, size: %d }\n", 
        msg->type, 
        msg->id,
        msg->refersTo,
        msg->sent.sec,
        msg->sent.usec,
        msg->received.sec,
        msg->received.usec,
        msg->size
    );
}