#include "time.h"

#include <buffer.h>

int time_message_serialize(time_message_t *msg, char *data, uint32_t size) {
    write_buffer_t buffer;
    int            result = 0;

    buffer_write_init(&buffer, data, size);

    result |= buffer_write_int32(&buffer, msg->latency.sec);
    result |= buffer_write_int32(&buffer, msg->latency.usec);

    return result;
}

int time_message_deserialize(time_message_t *msg, const char *data, uint32_t size) {
    read_buffer_t buffer;
    int           result = 0;

    buffer_read_init(&buffer, data, size);

    result |= buffer_read_int32(&buffer, &(msg->latency.sec));
    result |= buffer_read_int32(&buffer, &(msg->latency.usec));

    return result;
}
