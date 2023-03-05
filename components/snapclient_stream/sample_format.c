#include "sample_format.h"

#include <buffer.h>
#include <stdio.h>
#include <stdlib.h>

int sample_format_message_deserialize(sample_t *msg, const char *data, uint32_t size) {
    read_buffer_t buffer;
    size_t        s_string;
    int           result = 0;

    buffer_read_init(&buffer, data, size);

    result |= buffer_read_uint16(&buffer, &(msg->sample_size_));
    if (result) return 1;

    result |= buffer_read_uint16(&buffer, &(msg->frame_size_));
    if (result) return 1;

    result |= buffer_read_uint32(&buffer, &(msg->rate_));
    if (result) return 1;

    result |= buffer_read_uint16(&buffer, &(msg->bits_));
    if (result) return 1;

    result |= buffer_read_uint16(&buffer, &(msg->channels_));
    if (result) return 1;

    return result;
}

// void setFormat(const char* format)
// {
// }

// void setFormat(uint32_t rate, uint16_t bits, uint16_t channels)
// {

// }