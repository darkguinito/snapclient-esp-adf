#include "wire_chunk.h"

#include <buffer.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "SNAPCLIENT_WIRE_CHUNK";

void wire_chunk_message_free(wire_chunk_message_t *msg) {
    if (msg->payload != NULL) {
        free(msg->payload);
        msg->payload = NULL;
    }
}

int wire_chunk_message_full_deserialize(wire_chunk_message_full_t *msg, const char *data) {
    memcpy(&(msg->timestamp), data, msg->size + sizeof(tv_t) + sizeof(uint32_t));
    msg->payload = data + sizeof(tv_t) + sizeof(uint32_t);
    return 0;
}

void print_wire_chunk(const wire_chunk_message_full_t* msg){
    print_base_message(msg->base);
    ESP_LOGI(TAG, "wire_chunk: { timestamp: { sec: %d, usec: %d}, size: %d, payload: %p}\n", 
      msg->timestamp.sec,
      msg->timestamp.usec,
      msg->size,
      msg->payload
    );
}


void process_wire(wire_chunk_message_full_t* chunk, audio_element_handle_t self){
    size_t w_size = audio_element_output(self, chunk->payload, chunk->size);
    if (w_size > 0) {
        audio_element_update_byte_pos(self, chunk->size);
    } else {
        ESP_LOGI(TAG, "Not Inserted any data stream");
    }
}