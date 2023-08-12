#ifndef __SNAPCLIENT_SETTINGS_H__
#define __SNAPCLIENT_SETTINGS_H__

#include <stddef.h>

#include "sample_format.h"


typedef enum{
    UNSPECIFIED = 0,
    EXCLUSIVE   = 1,
    SHARED      = 2
} sharing_mode_e;

typedef enum {
    HARDWARE = 0,
    SOFTWARE,
    SCRIPT,
    NONE
}  mode_e;

typedef struct mixer_s {
    mode_e  mode;
    char*   parameter;
} mixer_t;

typedef struct server_s{
    char*  p_host;
    size_t port;
} server_t;

typedef struct player_s{
    char*             player_name;
    char*             parameter;
    int               latency;
    sample_t          sample_format;
    sharing_mode_e    sharing_mode;
    mixer_t           mixer;

} player_t;

typedef struct logging_s {
    char *sink;
    char *filter;
}logging_t;

typedef struct client_settings {

    size_t instance;
    char*  host_id;

    server_t server;
    player_t player;
    logging_t logging;

}client_settings_t;

#endif  //__SNAPCLIENT_SETTINGS_H__