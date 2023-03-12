#include "sample_format.h"

#include <buffer.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"

static const char *TAG = "SNAPCLIENT_SAMPLE_FORMAT";

int sample_format_message_deserialize(sample_t *msg, const char *data) {
    read_buffer_t buffer;
    int           result = 0;

    buffer_read_init(&buffer, data, sizeof(sample_t));

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

void print_sample_format(const sample_t* format){
    ESP_LOGI(TAG, "format: { sample_size: %d, frame_size: %d, rate: %d, bits: %d, channels: %d }\n", 
      format->sample_size_,
      format->frame_size_,
      format->rate_,
      format->bits_,
      format->channels_
    );
}