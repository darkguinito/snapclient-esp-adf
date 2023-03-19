#ifndef __SNAPCAST_MESSAGE_H__
#define __SNAPCAST_MESSAGE_H__

#include <stdint.h>
#include <stddef.h>

extern const int BASE_MESSAGE_SIZE;
extern const int TIME_MESSAGE_SIZE;

typedef enum {
    SNAPCAST_MESSAGE_BASE            = 0,
    SNAPCAST_MESSAGE_CODEC_HEADER    = 1,
    SNAPCAST_MESSAGE_WIRE_CHUNK      = 2,
    SNAPCAST_MESSAGE_SERVER_SETTINGS = 3,
    SNAPCAST_MESSAGE_TIME            = 4,
    SNAPCAST_MESSAGE_HELLO           = 5,
    // SNAPCAST_MESSAGE_STREAM_TAGS     = 6,
    SNAPCAST_MESSAGE_CLIENT_INFO     = 7,

    SNAPCAST_MESSAGE_FIRST           = SNAPCAST_MESSAGE_BASE,
    SNAPCAST_MESSAGE_LAST            = SNAPCAST_MESSAGE_CLIENT_INFO
} message_type_t;

typedef struct tv {
    int32_t sec;
    int32_t usec;
} tv_t;

typedef struct __attribute__((__packed__)) base_message {
    uint16_t type;
    uint16_t id;
    uint16_t refersTo;
    tv_t     sent;
    tv_t     received;
    size_t   size;
} base_message_t;


//const size_t max_size = 1000000;

int base_message_serialize  (base_message_t *msg,       char *data);
int base_message_deserialize(base_message_t *msg, const char *data);

void print_base_message(const base_message_t* msg);

#endif // __SNAPCAST_MESSAGE_H__