#include "server_settings.h"

//#ifdef ESP_PLATFORM
// The ESP-IDF changes the include directory for cJSON
#include <cJSON.h>
//#else
//#include "json/cJSON.h"
//#endif
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"

static const char *TAG = "SNAPCLIENT_SERVER_SETTINGS";

int server_settings_message_deserialize(server_settings_message_t *msg, const char *json_str) {
    int   status = 1;
    cJSON *value = NULL;
    cJSON *json  = cJSON_Parse(json_str);

    if (!json) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr) {
            // TODO change to a macro that can be diabled
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


int server_settings_message_full_deserialize(server_settings_message_full_t *msg, const char *json_str) {
    base_message_deserialize(&msg->base, json_str);
    // memmove(json_str, json_str + 4, sizeof(codec_header_message_t) - sizeof(base_message_t) - 4);
    // in_buffer[message_size - 3] = '\0';
    server_settings_message_deserialize((server_settings_message_t *)&(msg->buffer_ms), json_str);
    return 0;
}

void print_settings_message(const server_settings_message_full_t* msg){
    print_base_message(&msg->base);
    ESP_LOGI(TAG, "server: { buffer_ms: %d, latency: %d, volume: %d, muted: %d }\n", 
      msg->buffer_ms,
      msg->latency,
      msg->volume,
      msg->muted
    );
}