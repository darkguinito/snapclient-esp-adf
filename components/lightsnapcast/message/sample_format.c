#include "sample_format.h"

// #include <buffer.h>
// #include <stdio.h>
// #include <stdlib.h>
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SNAPCLIENT_SAMPLE_FORMAT";

int sample_format_message_deserialize(sample_t *msg, const char *data) {
  memcpy(msg, data, sizeof(sample_t));
  return 0;
}

void print_sample_format(const sample_t* format){
    ESP_LOGI(TAG, "format: { audio_format: %d, channels_: %d, rate_: %d, byte_rate: %d, block_align: %d, bits_: %d}\n", 
      format->audio_format,
      format->channels_,
      format->rate_,
      format->byte_rate,
      format->block_align,
      format->bits_
    );
}