#include "codec_header.h"

#include <stdlib.h>
#include <string.h>
#include <buffer.h>
#include "esp_log.h"
#include "audio_type_def.h"
#include "sample_format.h"

static const char *TAG = "SNAPCLIENT_CODEC_HEADER";

void codec_header_message_full_free(codec_header_message_full_t *msg) {
}

int codec_header_message_full_deserialize(codec_header_message_full_t *msg, const char *data) {
    uint32_t offset = 0;

    memcpy(&(msg->codec_size), data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    msg->codec = data + offset;
    offset += msg->codec_size;

    memcpy(&(msg->size), data + offset, sizeof(msg->size));
    offset += sizeof(msg->size);

    msg->payload = data + offset;
    return 0;
}

void print_codec_header_message(const codec_header_message_full_t* msg){
    print_base_message(msg->base);
    ESP_LOGI(TAG, "codec: { code_size: %d, codec: %s, size: %d, payload: %p }\n", 
        msg->codec_size,
        msg->codec,
        msg->size, 
        msg->payload
    );
}


void process_codec(codec_header_message_full_t* codec_header_message, audio_element_handle_t self)
{
    esp_codec_type_t codec;
	sample_t sampleFormat = {0};
	audio_element_info_t snap_info = {0};

    if (strcmp(codec_header_message->codec, "opus") == 0) {
        codec = ESP_CODEC_TYPE_OPUS;
    } else if (strcmp(codec_header_message->codec, "flac") == 0) {
        codec = ESP_CODEC_TYPE_FLAC;
    } else if (strcmp(codec_header_message->codec, "pcm") == 0) {
        codec = ESP_CODEC_TYPE_PCM;
    } else if (strcmp(codec_header_message->codec, "ogg") == 0) {
        codec = ESP_CODEC_TYPE_OGG;
    } else {
        ESP_LOGI(TAG, "Codec : %s not supported", codec_header_message->codec);
        ESP_LOGI(TAG, "Change encoder codec to opus in /etc/snapserver.conf on server");
        return ;
    }

    audio_element_set_codec_fmt(self, codec);
    
    sample_format_message_deserialize(&sampleFormat, codec_header_message->payload + 16);
    print_sample_format(&sampleFormat);
    ESP_LOGI(TAG, "sampleformat: %d:%d:%d\n", sampleFormat.rate_, sampleFormat.bits_, sampleFormat.channels_);

    audio_element_getinfo(self, &snap_info);
    snap_info.sample_rates = sampleFormat.rate_;
    snap_info.bits         = sampleFormat.bits_;
    snap_info.channels     = sampleFormat.channels_;
    audio_element_setinfo(self, &snap_info);
    audio_element_report_info(self);
}