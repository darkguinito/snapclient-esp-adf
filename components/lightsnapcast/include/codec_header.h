#ifndef __SNAPCAST_CODEC_HEADER_H__
#define __SNAPCAST_CODEC_HEADER_H__

#include <stdint.h>

typedef struct codec_header_message {
   char     *codec;
   uint32_t size;
   char     *payload;
} codec_header_message_t;

int  codec_header_message_deserialize(codec_header_message_t *msg, const char *data, uint32_t size);
void codec_header_message_free       (codec_header_message_t *msg);

#endif  // __SNAPCAST_CODEC_HEADER_H__