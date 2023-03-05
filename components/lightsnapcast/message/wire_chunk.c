#include "wire_chunk.h"

#include <buffer.h>
#include <stdlib.h>

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

	msg->payload = &(buffer.buffer[buffer.index]);
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