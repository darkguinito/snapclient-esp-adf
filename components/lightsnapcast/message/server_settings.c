#include "server_settings.h"

#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include <string.h>
#include "sample_format.h"

static const char *TAG = "SNAPCLIENT_SERVER_SETTINGS";

int server_settings_message_deserialize(server_settings_message_t *msg, const char *json_str) {
    int   status = 1;
    cJSON *value = NULL;
    cJSON *json  = cJSON_Parse(json_str);
   
    if (!json) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
            goto end;
        }
    }

    if (msg == NULL) {
        status = 2;
        goto end;
    }

    value = cJSON_GetObjectItemCaseSensitive(json, "bufferMs");
    if (cJSON_IsNumber(value)) {
        msg->buffer_ms = value->valueint;
    }

    value = cJSON_GetObjectItemCaseSensitive(json, "latency");
    if (cJSON_IsNumber(value)) {
        msg->latency = value->valueint;
    }

    value = cJSON_GetObjectItemCaseSensitive(json, "volume");
    if (cJSON_IsNumber(value)) {
        msg->volume = value->valueint;
    }

    value      = cJSON_GetObjectItemCaseSensitive(json, "muted");
    msg->muted = cJSON_IsTrue(value);
    status     = 0;
end:
    cJSON_Delete(json);
    return status;
}

int server_settings_message_full_deserialize(server_settings_message_full_t *msg, char *json_str) {
    size_t offset = 4;    
    server_settings_message_deserialize((server_settings_message_t *)&(msg->buffer_ms), json_str + offset);
    return 0;
}

void print_settings_message(const server_settings_message_full_t* msg){
    print_base_message(msg->base);
    ESP_LOGI(TAG, "server: { buffer_ms: %d, latency: %d, volume: %d, muted: %d }\n", 
      msg->buffer_ms,
      msg->latency,
      msg->volume,
      msg->muted
    );
}

void process_server(server_settings_message_full_t* server_settings_message, audio_element_handle_t self, audio_hal_handle_t s_volume_handle){

	audio_element_info_t snap_info = {0};
	sample_t sampleFormat = {0};
    ESP_LOGI(TAG, "SNAPCAST_MESSAGE_SERVER_SETTINGS (size=)");
    ESP_LOGI(TAG, "Buffer length:  %d", server_settings_message->buffer_ms);
    ESP_LOGI(TAG, "Ringbuffer size:%d", server_settings_message->buffer_ms * 48 * 4 );
    ESP_LOGI(TAG, "Latency:        %d", server_settings_message->latency);
    ESP_LOGI(TAG, "Mute:           %d", server_settings_message->muted);
    ESP_LOGI(TAG, "Setting volume: %d", server_settings_message->volume);

    // Not sure it is used somewhere ? 
    audio_element_getinfo(self, &snap_info);
//
    snap_info.sample_rates = sampleFormat.rate_;
    snap_info.bits         = sampleFormat.bits_;
    snap_info.channels     = sampleFormat.channels_;
//
    //// Not sure it is used somewhere ? 
    //// buffer_ms = server_settings_message->buffer_ms - server_settings_message->latency - player.latency;
    //// snap_info.duration = buffer_ms > 0 ? buffer_ms : 0;
    //
    snap_info.duration = server_settings_message->buffer_ms - server_settings_message->latency;
    audio_element_setinfo(self, &snap_info);

    // Volume setting using ADF HAL abstraction
    audio_hal_set_volume(s_volume_handle, server_settings_message->volume);
}