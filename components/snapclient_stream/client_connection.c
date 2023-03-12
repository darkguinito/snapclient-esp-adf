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


static int _get_socket_error_code_reason_bis(char *str, int sockfd)
{
    uint32_t optlen = sizeof(int);
    int result;
    int err;

    err = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &result, &optlen);
    if (err == -1) {
        ESP_LOGE(TAG, "%s, getsockopt failed: ret=%d", str, err);
        return -1;
    }
    if (result != 0) {
        ESP_LOGW(TAG, "%s error, error code: %d, reason: %s", str, err, strerror(result));
    }
    return result;
}

int connect_client(audio_element_handle_t self){
    snapclient = (snapclient_stream_t *)audio_element_getdata(self);
    if (snapclient->is_open) {
        ESP_LOGE(TAG, "Already opened");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Host is %s, port is %d\n", snapclient->host, snapclient->port);

    esp_transport_handle_t      t              = esp_transport_tcp_init();
    esp_transport_list_handle_t transport_list = esp_transport_list_init();

    esp_transport_list_add(transport_list, t, "http");

    AUDIO_NULL_CHECK(TAG, t, return ESP_FAIL);

    snapclient->sock = esp_transport_connect(t, snapclient->host, snapclient->port, CONNECT_TIMEOUT_CLIENT_MS);
    if (snapclient->sock < 0) {
        _get_socket_error_code_reason_bis("TCP create",  snapclient->sock);
        esp_transport_destroy(t);
        esp_transport_list_destroy(transport_list);
        return ESP_FAIL;
    }

    snapclient->is_open = true;
    snapclient->t = t;
    return 0;
}

void disconnect(){
    return;
}

void getMacAddress(){
    return ;
}

void* createMessage(const base_message_t base_message, char *in_buffer) {
    void* message = 0;
    int result = 0;
    switch (base_message.type)
    {
        case SNAPCAST_MESSAGE_CODEC_HEADER: {
            ESP_LOGI(TAG, "Deserializeing codec message.\n");
            message = malloc(sizeof(codec_header_message_full_t));
			result  = codec_header_message_full_deserialize(message, in_buffer);
            print_codec_header_message(message);
            break;
        }
        case SNAPCAST_MESSAGE_WIRE_CHUNK: {
            ESP_LOGI(TAG, "Deserializeing WIRE chunk message.\n");
            message = malloc(sizeof(wire_chunk_message_full_t));
			result  = wire_chunk_message_full_deserialize(message, in_buffer);
            print_wire_chunk(message);
            break;
        }
        case SNAPCAST_MESSAGE_SERVER_SETTINGS:{
            ESP_LOGI(TAG, "Deserializeing server settings message.\n");
            message = malloc(sizeof(server_settings_message_full_t));
			result  = server_settings_message_full_deserialize(message, in_buffer);
            print_settings_message(message);
            break;
        }
        case SNAPCAST_MESSAGE_TIME: {
            ESP_LOGI(TAG, "Deserializeing time message.\n");
            message = malloc(sizeof(time_message_full_t));
			result  = time_message_full_deserialize(message, in_buffer);
            print_time_message(message);
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
    return message;
}

void* getNextMessage(audio_element_handle_t self, char *in_buffer)
{
    void* message;
    int r_size        = 0;
    int result        = 0;
    const size_t max_size = 1000000;
    base_message_t base_message = {0};

    memset(&base_message, 0, sizeof(base_message_t));
    r_size = audio_element_input(self, in_buffer, sizeof(base_message_t));
    if (r_size <= 0) {
        // ring buffer cannot provide enough data (weird...)
        ESP_LOGI(TAG, "Cannot retrieved %d bytes of data!!!", sizeof(base_message_t));
        return 0;
    }
    result = base_message_deserialize(&base_message, in_buffer);
    print_base_message(&base_message);
    tv_t t = {0,0};
    base_message.received = t;
    if (base_message.type > SNAPCAST_MESSAGE_LAST)
    {
        ESP_LOGE(TAG, "unknown message type received: %d, size: %d", base_message.type, sizeof(base_message_t));  
        return 0;
    }else if (base_message.size > max_size) {
        ESP_LOGE(TAG, "received message of type: %d to large: %d", base_message.type, sizeof(base_message_t)); 
        return 0;
    }

    message = createMessage(base_message, in_buffer);

    if (result != 0) ESP_LOGW(TAG, "Failed to deserialize message of type: %d", base_message.type); 

    return message;
}
