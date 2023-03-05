#ifndef __SNAPCAST_WIRED_CHUNK_H__
#define __SNAPCAST_WIRED_CHUNK_H__

#include <stdint.h>

#include "message.h"

typedef struct wire_chunk_message {
    tv_t     timestamp;
    uint32_t size;
    char     *payload;
} wire_chunk_message_t;

int  wire_chunk_message_deserialize(wire_chunk_message_t *msg, const char *data, uint32_t size);
void wire_chunk_message_free       (wire_chunk_message_t *msg);

#endif // __SNAPCAST_WIRED_CHUNK_H__