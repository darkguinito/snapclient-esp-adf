#ifndef __SNAPCAST_CODEC_HEADER_H__
#define __SNAPCAST_CODEC_HEADER_H__

#include <stdint.h>
#include "message.h"
#include "audio_element.h"


typedef struct codec_header_message {
   uint32_t codec_size;
   char     *codec;
   uint32_t size;
   char     *payload;
} codec_header_message_t;

typedef struct __attribute__((__packed__)) codec_header_message_full {
   base_message_t*  base;
   uint32_t        codec_size;
   char            *codec;
   uint32_t        size;
   char            *payload;
} codec_header_message_full_t;

int  codec_header_message_full_deserialize(codec_header_message_full_t *msg, const char *data);
void print_codec_header_message(const codec_header_message_full_t* msg);
void process_codec(codec_header_message_full_t* codec_header_message, audio_element_handle_t self);

#endif  // __SNAPCAST_CODEC_HEADER_H__