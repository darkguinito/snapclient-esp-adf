#ifndef __SNAPCLIENT_CONTROLLER_H__
#define __SNAPCLIENT_CONTROLLER_H__


#include "audio_hal.h"
#include "audio_element.h"
#include "message.h"

void controller_getNextMessage(audio_element_handle_t self, const char* base, message_type_t type, audio_hal_handle_t s_volume_handle);

char* get_message_type(message_type_t type);

#endif // __SNAPCLIENT_CONTROLLER_H__