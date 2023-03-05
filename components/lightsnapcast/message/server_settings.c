#include "server_settings.h"

//#ifdef ESP_PLATFORM
// The ESP-IDF changes the include directory for cJSON
#include <cJSON.h>
//#else
//#include "json/cJSON.h"
//#endif

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
