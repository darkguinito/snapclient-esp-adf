#include "wire_chunk.h"

#include <buffer.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "SNAPCLIENT_WIRE_CHUNK";

int wire_chunk_message_deserialize(wire_chunk_message_t *msg, const char *data, uint32_t size) {
    read_buffer_t buffer;
    int result = 0;

    buffer_read_init(&buffer, data, size);

    result |= buffer_read_int32(&buffer, &(msg->timestamp.sec));
    result |= buffer_read_int32(&buffer, &(msg->timestamp.usec));
    result |= buffer_read_uint32(&buffer, &(msg->size));

    // If there's been an error already (especially for the size bit) return early
    if (result) {
        return result;
    }

	msg->payload = (char*)&(buffer.buffer[buffer.index]);

    result |= sample_format_message_deserialize(&(msg->format), (char*)&(buffer.buffer[buffer.index]));
    if (result) {
        return result;
    }
    
	return result;
	/*
    // TODO maybe should check to see if need to free memory?
    msg->payload = malloc(msg->size * sizeof(char));
    // Failed to allocate the memory
    if (!msg->payload) {
        return 2;
    }

    result |= buffer_read_buffer(&buffer, msg->payload, msg->size);
    return result;
	*/
}


void wire_chunk_message_free(wire_chunk_message_t *msg) {
    if (msg->payload) {
        free(msg->payload);
        msg->payload = NULL;
    }
}


int wire_chunk_message_full_deserialize(wire_chunk_message_full_t *msg, const char *data) {
    memcpy(msg, data, sizeof(wire_chunk_message_t));
    return 0;
}

void print_wire_chunk(const wire_chunk_message_full_t* msg){
    print_base_message(&msg->base);
    ESP_LOGI(TAG, "wire_chunk: { timestamp: { sec: %d, usec: %d}, size: %d, payload: %d}\n", 
      msg->timestamp.sec,
      msg->timestamp.usec,
      msg->size,
      (uint32_t)msg->payload
    );
}