#ifndef __SNAPCLIENT_PCM_DEVICE_H__
#define __SNAPCLIENT_PCM_DEVICE_H__

#include <stddef.h>

typedef struct pcm_device_s {
    int   idx;
    char* name;
    char* description;
}pcm_device_t;


#endif // __SNAPCLIENT_PCM_DEVICE_H__