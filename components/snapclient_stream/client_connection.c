#include "client_connection.h"

#include <string.h>
#include "audio_element.h"
#include "esp_log.h"
#include "esp_err.h"
#include "message.h"
#include "codec_header.h"
#include "wire_chunk.h"
#include "server_settings.h"
#include "time_message.h"

#include "esp_transport_tcp.h"

static const char *TAG = "SNAPCLIENT_CLIENT_CONNECTION";
base_message_t base_message_;
#define CONNECT_TIMEOUT_CLIENT_MS        500

void createMessage(base_message_t* message, char *in_buffer, audio_element_handle_t self, audio_hal_handle_t s_volume_handle, time_message_full_t* time_message) {
    int result = 0;

    switch (message->type)
    {
        case SNAPCAST_MESSAGE_CODEC_HEADER: {
            ESP_LOGD(TAG, "Deserializeing codec message.\n");
            codec_header_message_full_t codec = { message, 0, NULL, 0, NULL };
			result = codec_header_message_full_deserialize(&codec, in_buffer);
        //    print_codec_header_message(&codec);
            process_codec(&codec, self);
            break;
        }
        case SNAPCAST_MESSAGE_WIRE_CHUNK: {
            ESP_LOGD(TAG, "Deserializeing WIRE chunk message.\n");
            wire_chunk_message_full_t chunk = { message, { 0, 0}, 0, NULL};
			result = wire_chunk_message_full_deserialize(&chunk, in_buffer);
           // print_wire_chunk(&chunk);
            //struct timeval now;
            //gettimeofday(&now, NULL);
            //int diff = (int)now.tv_usec - chunk.timestamp.usec;
            chunk.timestamp.usec += time_message->latency.usec;
            //chunk.size += (diff / 40000);
            //int delta = (diff / 20000);
            //chunk.size += (diff / 20000);
            //ESP_LOGI(TAG, "Timestamp: %d, %d Now %d, %d, diff %d, size %d, delta %d, Latency %d, %d", chunk.timestamp.sec, chunk.timestamp.usec, (int)now.tv_sec, (int)now.tv_usec, diff, chunk.size, delta, time_message->latency.sec, time_message->latency.usec);
            process_wire(&chunk, self);
            break;
        }
        case SNAPCAST_MESSAGE_SERVER_SETTINGS:{
            ESP_LOGD(TAG, "Deserializeing server settings message.\n");
            server_settings_message_full_t server = { message, 0, 0, 0, false};
			result = server_settings_message_full_deserialize(&server, in_buffer);
           // print_settings_message(&server);
            process_server(&server, self, s_volume_handle);
            break;
        }
        case SNAPCAST_MESSAGE_TIME: {
            ESP_LOGD(TAG, "Deserializeing time message.\n");            
            memcpy(&time_message->base, message, sizeof(base_message_t));
            memcpy(&time_message->latency, in_buffer, sizeof(tv_t));
            struct timeval trx;
            trx.tv_sec  = time_message->base.sent.sec - time_message->latency.sec;
            trx.tv_usec = time_message->base.sent.usec - time_message->latency.usec;
            settimeofday(&trx, NULL);
            //print_time_full_message(time_message);
            break;
        }
        case SNAPCAST_MESSAGE_HELLO: {
            ESP_LOGI(TAG, "Deserializeing hello message.\n");
  //          message = malloc(sizeof(hello_message_full_t));
//			result  = codec_header_message_full_deserialize(message, in_buffer);
            break;
        }
        case SNAPCAST_MESSAGE_CLIENT_INFO: {
            ESP_LOGI(TAG, "Deserializeing client info message.\n");
            // message = malloc(sizeof(client_info_full_t));
			// result  = codec_header_message_full_deserialize(message, in_buffer);
            break;
        }
        default: {
            ESP_LOGI(TAG, "Deserializeing codec message.\n");
            break;
        }
    }

    if ( result != 0 ){
        ESP_LOGE(TAG, "Erreur deserializing\n");
    }
    return ;
}
