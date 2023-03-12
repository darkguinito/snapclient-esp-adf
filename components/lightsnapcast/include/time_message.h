#ifndef __SNAPCAST_TIME_H__
#define __SNAPCAST_TIME_H__

#include <stdint.h>

#include "message.h"

typedef struct time_message {
   // base_message_t base;
    tv_t           latency;
} time_message_t;


typedef struct time_message_full {
    base_message_t base;
    tv_t           latency;
} time_message_full_t;

int time_message_serialize  (time_message_t *msg,       char *data);
int time_message_deserialize(time_message_t *msg, const char *data);

int time_message_full_deserialize(time_message_full_t *msg, const char *data);

void print_time_message(const time_message_full_t* msg);

#endif // __SNAPCAST_TIME_H__