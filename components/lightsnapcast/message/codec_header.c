#include "codec_header.h"

#include <stdlib.h>
#include <string.h>
#include <buffer.h>
#include "esp_log.h"

static const char *TAG = "SNAPCLIENT_CODEC_HEADER";

int codec_header_message_deserialize(codec_header_message_t *msg, const char *data, uint32_t size) {
    read_buffer_t buffer;
    size_t        s_string;
    int           result = 0;

    buffer_read_init(&buffer, data, size);

    result |= buffer_read_uint32(&buffer, &s_string);
    // Can't allocate the proper size string if we didn't read the size, so fail early
    if (result) return 1;

    msg->codec = malloc(s_string + 1); // +1 to add ending \0
    if (!msg->codec) return 2;

    result |= buffer_read_buffer(&buffer, msg->codec, s_string);
    // Make sure the codec is a proper C string by terminating it with a null character
    msg->codec[s_string] = '\0';

    result |= buffer_read_uint32(&buffer, &(msg->size));
    // Can't allocate the  proper size string if we didn't read the size, so fail early
    if (result) return 1;

    msg->payload = malloc(msg->size);
    if (!msg->payload) return 2;

    result |= buffer_read_buffer(&buffer, msg->payload, msg->size);
    return result;
}

void codec_header_message_free(codec_header_message_t *msg) {
    free(msg->codec);
    msg->codec = NULL;

    free(msg->payload);
    msg->payload = NULL;
}

void codec_header_message_full_free(codec_header_message_full_t *msg) {
    free(msg->codec);
    msg->codec = NULL;

    free(msg->payload);
    msg->payload = NULL;

    free(msg);
}

int codec_header_message_full_deserialize(codec_header_message_full_t *msg, const char *data) {
    memcpy(msg, data, sizeof(codec_header_message_t));
    return 0;
}


void print_codec_header_message(const codec_header_message_full_t* msg){
    print_base_message(&msg->base);
    ESP_LOGI(TAG, "codec: { size: %d, payload: %d, codec: %s }\n", 
        msg->size, 
        (int)msg->payload,
        msg->codec
    );
}