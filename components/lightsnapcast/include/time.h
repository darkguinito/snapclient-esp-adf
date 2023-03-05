#ifndef __SNAPCAST_TIME_H__
#define __SNAPCAST_TIME_H__

#include <stdint.h>

#include "message.h"

typedef struct time_message {
    tv_t latency;
} time_message_t;

int time_message_serialize  (time_message_t *msg,       char *data, uint32_t size);
int time_message_deserialize(time_message_t *msg, const char *data, uint32_t size);


#endif // __SNAPCAST_TIME_H__