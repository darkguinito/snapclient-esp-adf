#ifndef __SNAPCLIENT_CLIENT_CONNECTION_H__
#define __SNAPCLIENT_CLIENT_CONNECTION_H__

#include "client_settings.h"
#include "audio_element.h"
#include "esp_transport.h"
#include "client_settings.h"
#include "audio_hal.h"

#include "snapclient_stream.h"
#include "time_message.h"



typedef struct snapclient_stream {
    esp_transport_handle_t            t;
    audio_stream_type_t               type;
    int                               sock;
    int                               port;
    char                              *host;
    bool                              is_open;
    int                               timeout_ms;
    snapclient_stream_event_handle_cb hook;
    void                              *ctx;
	
	bool                              received_header;
	struct timeval                    last_sync;
	int                               id_counter;
	// base_message_t                    base_message;
	// codec_header_message_t            codec_header_message;
	// wire_chunk_message_t              wire_chunk_message;
	// server_settings_message_t         server_settings_message;
	time_message_t                    time_message;

	player_t                          player;
} snapclient_stream_t;

static snapclient_stream_t *snapclient = NULL;

void getMacAddress();

void* createMessage(const base_message_t base_message, char *in_buffer);
void* getNextMessage(audio_element_handle_t self, char *in_buffer);


#endif  //__SNAPCLIENT_CLIENT_CONNECTION_H__
