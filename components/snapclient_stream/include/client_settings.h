#ifndef __SNAPCLIENT_SETTINGS_H__
#define __SNAPCLIENT_SETTINGS_H__

#include <stddef.h>

#include "sample_format.h"
#include "pcm_device.h"

enum sharing_mode {
    UNSPECIFIED = 0,
    EXCLUSIVE   = 1,
    SHARED      = 2
};

enum mode {
    HARDWARE = 0,
    SOFTWARE,
    SCRIPT,
    NONE
};

struct mixer_s {
    mode   mode;
    char*  parameter;
} mixer_defautl = {mode::SOFTWARE, NULL };

typedef struct mixer_s mixer_t;

struct server_s{
    char*  p_host;
    size_t port;
} server_default = { NULL, 1704 };

typedef struct server_s server_t;


struct player_s{
    char*             player_name;
    char*             parameter;
    int               latency;
    pcm_device_t      pcm_device;
    sample_t          sample_format;
    sharing_mode      sharing_mode;
    mixer_t           mixer;

} player_default = { NULL, NULL, 0, {0, NULL, NULL}, { 0, 0, 0, 0, 0 }, sharing_mode::UNSPECIFIED, mixer_defautl };

typedef struct player_s player_t;

struct logging_s {
    char *sink;
    char *filter;
}logging_default = { NULL, "*:info"};

typedef struct logging_s logging_t;

typedef struct client_settings {

    size_t instance;
    char*  host_id;

    server_t server;
    player_t player;
    logging_t logging;

}client_settings_t;

#endif  //__SNAPCLIENT_SETTINGS_H__