#ifndef _SNAPCAST_SERVER_SETTINGS_H_
#define _SNAPCAST_SERVER_SETTINGS_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct server_settings_message {
    int32_t  buffer_ms;
    int32_t  latency;
    uint32_t volume;
    bool     muted;
} server_settings_message_t;

int server_settings_message_deserialize(server_settings_message_t *msg, const char *json_str);

#endif _SNAPCAST_SERVER_SETTINGS_H_
