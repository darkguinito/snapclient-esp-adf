#include "controller.h"
#include <string.h>

#include "message.h"
#include "codec_header.h"
#include "audio_type_def.h"
#include "audio_element.h"
#include "audio_hal.h"
#include "esp_log.h"

#include "codec_header.h"
#include "wire_chunk.h"
#include "server_settings.h"

static const char *TAG = "SNAPCLIENT_CONTROLLER";

char* get_message_type(message_type_t type){
	switch(type){
		case SNAPCAST_MESSAGE_CODEC_HEADER:
			return "SNAPCAST_MESSAGE_CODEC_HEADER";
		case SNAPCAST_MESSAGE_WIRE_CHUNK:
			return "SNAPCAST_MESSAGE_WIRE_CHUNK";
		case SNAPCAST_MESSAGE_SERVER_SETTINGS:
			return "SNAPCAST_MESSAGE_SERVER_SETTINGS";
		default:
			return "Unknow";
	}
}

void controller_getNextMessage(audio_element_handle_t self, const base_message_t* base, audio_hal_handle_t s_volume_handle) {
	esp_codec_type_t codec;
    int      w_size        = 0;
	sample_t sampleFormat = {0};
	char *   message_name = get_message_type(base->type);
	audio_element_info_t snap_info = {0};
	ESP_LOGI(TAG, "%s (size=%d)", message_name, base->size);
	switch (base->type) {
		case SNAPCAST_MESSAGE_CODEC_HEADER:
		{
			codec_header_message_full_t* codec_header_message = (codec_header_message_full_t*) base;

			ESP_LOGI(TAG, "Received codec header message\r\n");
			ESP_LOGI(TAG, "Codec: %s , Size: %d", codec_header_message->codec, codec_header_message->size);

			if (strcmp(codec_header_message->codec, "opus") == 0) {
				codec = ESP_CODEC_TYPE_OPUS;
			} else if (strcmp(codec_header_message->codec, "flac") == 0) {
				codec = ESP_CODEC_TYPE_FLAC;
			} else if (strcmp(codec_header_message->codec, "pcm") == 0) {
				codec = ESP_CODEC_TYPE_PCM;
			} else if (strcmp(codec_header_message->codec, "ogg") == 0) {
				codec = ESP_CODEC_TYPE_OGG;
			} else {
				ESP_LOGI(TAG, "Codec : %s not supported", codec_header_message->codec);
				ESP_LOGI(TAG, "Change encoder codec to opus in /etc/snapserver.conf on server");
				break;
			}

			audio_element_set_codec_fmt(self, codec);

			sample_format_message_deserialize(&sampleFormat, codec_header_message->payload);

			ESP_LOGI(TAG, "sampleformat: %d:%d:%d\n", sampleFormat.rate_, sampleFormat.bits_, sampleFormat.channels_);

			//snapclient->received_header = true;
			codec_header_message_full_free(codec_header_message);

			audio_element_getinfo(self, &snap_info);
			snap_info.sample_rates = sampleFormat.rate_;
			snap_info.bits         = sampleFormat.bits_;
			snap_info.channels     = sampleFormat.channels_;

			// Not sure it is used somewhere ? 
			// buffer_ms          = snapclient->server_settings_message.buffer_ms - snapclient->server_settings_message.latency - snapclient->player.latency;
			// snap_info.duration = buffer_ms > 0 ? buffer_ms : 0;

			// ESP_LOGE(TAG, "Latency: %d\n", snapclient->player.latency);
			// audio_element_setinfo(self, &snap_info);
			// audio_element_report_info(self);
			break;
		}
		case SNAPCAST_MESSAGE_WIRE_CHUNK:
		{
			wire_chunk_message_full_t* wire_chunk_message = (wire_chunk_message_full_t*) base;
			ESP_LOGI(TAG, "Received wire message\r\n");

			// write the received chunk in the output ring buffer
			w_size = audio_element_output(self, wire_chunk_message->payload, wire_chunk_message->size);
			if (w_size > 0) {
				//ESP_LOGI(TAG, "Inserted %d of data stream", w_size);
				audio_element_update_byte_pos(self, wire_chunk_message->size);
			} else {
				ESP_LOGI(TAG, "Not Inserted any data stream");
			}

			free(wire_chunk_message);
			break;
		}
		case SNAPCAST_MESSAGE_SERVER_SETTINGS:
		{
			server_settings_message_full_t* server_settings_message = (server_settings_message_full_t*) base;
			ESP_LOGI(TAG, "SNAPCAST_MESSAGE_SERVER_SETTINGS (size=)");
			
			// The first 4 bytes in the buffer are the size of the string.
			// We don't need this, so we'll shift the entire buffer over 4 bytes
			// and use the extra room to add a null character so cJSON can parse it.
			// memmove(in_buffer, in_buffer + 4, message_size - 4);
			// in_buffer[message_size - 3] = '\0';
			// result = server_settings_message_deserialize(
			// 	&(snapclient->server_settings_message), in_buffer);
			// if (result) {
			// 	ESP_LOGI(TAG, "Failed to read server settings: %d\r\n", result);
			// 	break;
			// }
			// log mute state, buffer, latency
			//buffer_ms = server_settings_message->buffer_ms;

			ESP_LOGI(TAG, "Buffer length:  %d", server_settings_message->buffer_ms);
			ESP_LOGI(TAG, "Ringbuffer size:%d", server_settings_message->buffer_ms * 48 * 4 );
			ESP_LOGI(TAG, "Latency:        %d", server_settings_message->latency);
			ESP_LOGI(TAG, "Mute:           %d", server_settings_message->muted);
			ESP_LOGI(TAG, "Setting volume: %d", server_settings_message->volume);

			// Not sure it is used somewhere ? 
			audio_element_getinfo(self, &snap_info);

			snap_info.sample_rates = sampleFormat.rate_;
			snap_info.bits         = sampleFormat.bits_;
			snap_info.channels     = sampleFormat.channels_;

			// Not sure it is used somewhere ? 
			// buffer_ms = server_settings_message->buffer_ms - server_settings_message->latency - player.latency;
			// snap_info.duration = buffer_ms > 0 ? buffer_ms : 0;
			
			snap_info.duration = server_settings_message->buffer_ms - server_settings_message->latency;
			audio_element_setinfo(self, &snap_info);

			// Volume setting using ADF HAL abstraction
			audio_hal_set_volume(s_volume_handle, server_settings_message->volume);
			break;
		}
		default:
		{
			ESP_LOGW(TAG, "UNKNOWN_MESSAGE_TYPE %d (size=%d)", base->type, base->size);
			break;
		}
	}
	return;
}