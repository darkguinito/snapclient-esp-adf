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
#include "controller.h"

static const char *TAG = "SNAPCLIENT_CLIENT_CONNECTION";
base_message_t base_message_;
#define CONNECT_TIMEOUT_CLIENT_MS        500



void createMessage(base_message_t* message, char *in_buffer, audio_element_handle_t self, audio_hal_handle_t s_volume_handle, time_message_full_t* time_message) {
    int result = 0;

    switch (message->type)
    {
        case SNAPCAST_MESSAGE_CODEC_HEADER: {
        //    ESP_LOGI(TAG, "Deserializeing codec message.\n");
            codec_header_message_full_t codec = { message, 0, NULL, 0, NULL };
			result = codec_header_message_full_deserialize(&codec, in_buffer);
        //    print_codec_header_message(&codec);
            process_codec(&codec, self);
            codec_header_message_full_free(&codec);
            break;
        }
        case SNAPCAST_MESSAGE_WIRE_CHUNK: {
     //       ESP_LOGI(TAG, "Deserializeing WIRE chunk message.\n");
            wire_chunk_message_full_t chunk = { message, { 0, 0}, 0, NULL};
			result = wire_chunk_message_full_deserialize(&chunk, in_buffer);
           // print_wire_chunk(&chunk);
            process_wire(&chunk, self);
          //  codec_header_message_full_free(&wire_chunk_message_free);
            break;
        }
        case SNAPCAST_MESSAGE_SERVER_SETTINGS:{
        //    ESP_LOGI(TAG, "Deserializeing server settings message.\n");
            server_settings_message_full_t server = { message, 0, 0, 0, false};
			result = server_settings_message_full_deserialize(&server, in_buffer);
           // print_settings_message(&server);
            process_server(&server, self, s_volume_handle);
            break;
        }
        case SNAPCAST_MESSAGE_TIME: {
            ESP_LOGI(TAG, "Deserializeing time message.\n");
            
            memcpy(&time_message->base, message, sizeof(base_message_t));
            memcpy(&time_message->latency, in_buffer, sizeof(tv_t));
            //print_time_full_message(time_message);
            break;
        }
        case SNAPCAST_MESSAGE_HELLO: {
  //          ESP_LOGI(TAG, "Deserializeing hello message.\n");
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
    //ESP_LOGE(TAG, "End\n");
    return ;
}

// size_t getNextMessage(audio_element_handle_t self, char *in_buffer, audio_hal_handle_t s_volume_handle, bool *received)
// {
//     size_t r_size  = 0;
//     int result     = 0;
//     const size_t max_size = 1000000;

//     base_message_t base_message = {0};
//     r_size = audio_element_input(self, in_buffer, sizeof(base_message_t));

//     if (r_size < sizeof(base_message_t)) {
//         ESP_LOGE(TAG, "Retrieved only %d bytes of data instead of %d !!! ABORT", r_size, sizeof(base_message_t));
//         return -1;
//     }

//     result = base_message_deserialize(&base_message, in_buffer);

//     if (result != 0){
//         ESP_LOGW(TAG, "Failed to deserialize message of type: %d", base_message.type); 
//         return NULL;
//     } 

//     r_size += audio_element_input(self, in_buffer, base_message.size);
//     if (r_size < base_message.size)  // XXX
//     {
//         ESP_LOGE(TAG, "Retrieved only %d bytes of data instead of %d !!! ABORT", r_size, base_message.size);
//         return -1;
//     }

//     tv_t t = {0,0};
//     base_message.received = t;
//     if (base_message.type > SNAPCAST_MESSAGE_LAST)
//     {   
//         ESP_LOGE(TAG, "unknown message type received: %d, size: %d", base_message.type, sizeof(base_message_t));  
//         return 0;
//     }else if (base_message.size > max_size) {
//         ESP_LOGE(TAG, "received message of type: %d to large: %d, max: %d", base_message.type, base_message.size, max_size); 
//         return 0;
//     }

//     createMessage(&base_message, in_buffer, self, s_volume_handle);
//     *received = (base_message.type == SNAPCAST_MESSAGE_CODEC_HEADER);
//     return r_size;
// }
