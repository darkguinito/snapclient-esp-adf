#include "message.h"

#include <buffer.h>

const int BASE_MESSAGE_SIZE = 26;
const int TIME_MESSAGE_SIZE = 8;

int base_message_serialize(base_message_t *msg, char *data, uint32_t size) {
    write_buffer_t buffer;
    int result = 0;

    buffer_write_init(&buffer, data, size);

    result |= buffer_write_uint16(&buffer, msg->type);
    result |= buffer_write_uint16(&buffer, msg->id);
    result |= buffer_write_uint16(&buffer, msg->refersTo);
    result |= buffer_write_int32(&buffer, msg->sent.sec);
    result |= buffer_write_int32(&buffer, msg->sent.usec);
    result |= buffer_write_int32(&buffer, msg->received.sec);
    result |= buffer_write_int32(&buffer, msg->received.usec);
    result |= buffer_write_uint32(&buffer, msg->size);

    return result;
}

int base_message_deserialize(base_message_t *msg, const char *data, uint32_t size) {
    read_buffer_t buffer;
    int result = 0;

    buffer_read_init(&buffer, data, size);

    result |= buffer_read_uint16(&buffer, &(msg->type));
    result |= buffer_read_uint16(&buffer, &(msg->id));
    result |= buffer_read_uint16(&buffer, &(msg->refersTo));
    result |= buffer_read_int32 (&buffer, &(msg->sent.sec));
    result |= buffer_read_int32 (&buffer, &(msg->sent.usec));
    result |= buffer_read_int32 (&buffer, &(msg->received.sec));
    result |= buffer_read_int32 (&buffer, &(msg->received.usec));
    result |= buffer_read_uint32(&buffer, &(msg->size));

    return result;
}