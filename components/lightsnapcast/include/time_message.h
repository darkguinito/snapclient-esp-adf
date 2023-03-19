#ifndef __SNAPCAST_TIME_H__
#define __SNAPCAST_TIME_H__

#include <stdint.h>

#include "message.h"

typedef struct time_message {
    tv_t           latency;
} time_message_t;

typedef struct __attribute__((__packed__)) time_message_full {
    base_message_t  base;
    tv_t            latency;
} time_message_full_t;

int time_message_deserialize     (time_message_t*      msg, const char* data);
int time_message_full_deserialize(time_message_full_t* msg, const char* data);
int time_message_full_serialize  (time_message_full_t* msg, char*       data);

void print_time_message     (const time_message_t*      msg);
void print_time_full_message(const time_message_full_t* msg);

#endif // __SNAPCAST_TIME_H__