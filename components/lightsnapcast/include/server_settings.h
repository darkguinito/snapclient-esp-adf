#ifndef __SNAPCAST_SERVER_SETTINGS_H__
#define __SNAPCAST_SERVER_SETTINGS_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "message.h"
#include "audio_element.h"
#include "audio_hal.h"

typedef struct __attribute__((__packed__)) server_settings_message {
    int32_t  buffer_ms;
    int32_t  latency;
    uint32_t volume;
    bool     muted;
} server_settings_message_t;

typedef struct __attribute__((__packed__)) server_settings_message_full {
    base_message_t* base;
    int32_t        buffer_ms;
    int32_t        latency;
    uint32_t       volume;
    bool           muted;
} server_settings_message_full_t;

int server_settings_message_deserialize     (server_settings_message_t      *msg, const char *json_str);
int server_settings_message_full_deserialize(server_settings_message_full_t *msg, char *json_str);
void print_settings_message(const server_settings_message_full_t* msg);

void process_server(server_settings_message_full_t* server, audio_element_handle_t self, audio_hal_handle_t s_volume_handle);

#endif // __SNAPCAST_SERVER_SETTINGS_H__
