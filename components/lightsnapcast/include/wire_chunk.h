#ifndef __SNAPCAST_WIRED_CHUNK_H__
#define __SNAPCAST_WIRED_CHUNK_H__

#include <stdint.h>

#include "message.h"
#include "sample_format.h"
#include "audio_element.h"

typedef struct wire_chunk_message {
    tv_t      timestamp;
    uint32_t  size;
    char*     payload;
    sample_t  format;
    uint32_t  idx;
} wire_chunk_message_t;

typedef struct __attribute__((__packed__)) wire_chunk_message_full {
    base_message_t*  base;
    tv_t            timestamp;
    uint32_t        size;
    char*           payload;
} wire_chunk_message_full_t;

int  wire_chunk_message_full_deserialize(wire_chunk_message_full_t *msg, const char *data);
void print_wire_chunk(const wire_chunk_message_full_t* msg);
void process_wire(wire_chunk_message_full_t* chunk, audio_element_handle_t self);

#endif // __SNAPCAST_WIRED_CHUNK_H__