/* Snapcast client


   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "lwip/err.h"
#include "lwip/apps/sntp.h"

#include "esp_log.h"
#include "esp_wifi.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "snapclient_stream.h"
#include "opus_decoder.h"
#include "flac_decoder.h"
#include "pcm_decoder.h"
#include "filter_resample.h"

#include "nvs_flash.h"

#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "board.h"

static const char *TAG = "SNAPCAST";

int16_t Gain(int16_t s, int vol) {
    int32_t v = 0;
    v= (s * vol) >> 6;
    return (int16_t)(v & 0xffff);
}

int mp3_music_volume(audio_element_handle_t el, char *buf, int len, TickType_t wait_time, void *ctx)
{
    size_t bytes_written = 0;
    uint16_t test = 0;
    int vol = 0; 
    u_int16_t m_lastSample[1024];
    audio_board_handle_t board_handle = (audio_board_handle_t)ctx;
    audio_hal_get_volume(board_handle->audio_hal, &vol);
    memset(m_lastSample, 0, len);
   
    for(size_t i = 1; i < len; i+=2 ){
        test = (buf[i] << 8) + buf[i + 1];
        m_lastSample[i/2] = Gain(test, vol);
    }
    i2s_write(0, m_lastSample, len, &bytes_written, portMAX_DELAY);  

	return bytes_written;
}


void app_main(void)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t i2s_stream_writer;
    //audio_element_handle_t pcm_decoder;
    audio_element_handle_t snapclient_stream;

	// setup logging
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_INFO);

	// flash init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
	tcpip_adapter_init();

	// now setip the audio pipeline
    ESP_LOGI(TAG, "[ 1 ] Start audio codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[ 2 ] Create audio pipeline, add all elements to pipeline, and subscribe pipeline event");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[2.0] Create snapclient source stream");
    snapclient_stream_cfg_t snapclient_cfg = SNAPCLIENT_STREAM_CFG_DEFAULT();
	snapclient_cfg.port   = CONFIG_SNAPSERVER_PORT;
	snapclient_cfg.host   = CONFIG_SNAPSERVER_HOST;
	snapclient_cfg.player.latency = 20;
    snapclient_stream = snapclient_stream_init(&snapclient_cfg, board_handle->audio_hal);


    ESP_LOGI(TAG, "[2.1] Create pcm decoder");
   // pcm_decoder_cfg_t pcm_cfg = DEFAULT_PCM_DECODER_CONFIG();
   // pcm_decoder = pcm_decoder_init(&pcm_cfg);

    ESP_LOGI(TAG, "[2.2] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[2.3] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, snapclient_stream, "snapclient");
    //audio_pipeline_register(pipeline, pcm_decoder, "pcm");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");
    audio_element_set_write_cb(i2s_stream_writer, mp3_music_volume, (void*)board_handle);

    ESP_LOGI(TAG, "[2.4] Link it together");

    const char *link_tag[2] = {"snapclient", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 2);

    ESP_LOGI(TAG, "[ 3 ] Start and wait for Wi-Fi network");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    periph_wifi_cfg_t wifi_cfg = {
        .ssid = CONFIG_ESP_WIFI_SSID,
        .password = CONFIG_ESP_WIFI_PASSWORD,
    };

    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    esp_periph_start(set, wifi_handle);
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);

	struct timeval now;
	if (gettimeofday(&now, NULL)) {
		ESP_LOGW(TAG, "Failed to gettimeofday");
	} else {
		ESP_LOGI(TAG, "Current timestamp is %ld.%ld", now.tv_sec, now.tv_usec);
	}

    ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    ESP_LOGI(TAG, "[ 5 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);

	i2s_stream_set_clk(i2s_stream_writer, 44100 , 16, 2);

    while (1) {
		char source[20];

        audio_event_iface_msg_t msg;
		ESP_LOGI(TAG, "[ X ] Waiting for a new message");
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.source == (void *) snapclient_stream)
			sprintf(source, "%s", "snapclient");
		else if (msg.source == (void *) i2s_stream_writer)
			sprintf(source, "%s", "i2s");
		else
			sprintf(source, "%s", "unknown");


		ESP_LOGI(TAG, "[ X ] Event message %d:%d from %s", msg.source_type, msg.cmd, source);
		if (msg.cmd == AEL_MSG_CMD_REPORT_STATUS)
			switch ( (int) msg.data ) {
				case AEL_STATUS_NONE:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_NONE");
					break;
				case AEL_STATUS_ERROR_OPEN:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_ERROR_OPEN");
					break;
				case AEL_STATUS_ERROR_INPUT:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_ERROR_INPUT");
					break;
				case AEL_STATUS_ERROR_PROCESS:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_ERROR_PROCESS");
					break;
				case AEL_STATUS_ERROR_OUTPUT:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_ERROR_OUTPUT");
					break;
				case AEL_STATUS_ERROR_CLOSE:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_ERROR_CLOSE");
					break;
				case AEL_STATUS_ERROR_TIMEOUT:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_ERROR_TIMEOUT");
					break;
				case AEL_STATUS_ERROR_UNKNOWN:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_ERROR_UNKNOWN");
					break;
				case AEL_STATUS_INPUT_DONE:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_INPUT_DONE");
					break;
				case AEL_STATUS_INPUT_BUFFERING:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_INPUT_BUFFERING");
					break;
				case AEL_STATUS_OUTPUT_DONE:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_OUTPUT_DONE");
					break;
				case AEL_STATUS_OUTPUT_BUFFERING:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_OUTPUT_BUFFERING");
					break;
				case AEL_STATUS_STATE_RUNNING:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_STATE_RUNNING");
					break;
				case AEL_STATUS_STATE_PAUSED:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_STATE_PAUSED");
					break;
				case AEL_STATUS_STATE_STOPPED:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_STATE_STOPPED");
					break;
				case AEL_STATUS_STATE_FINISHED:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_STATE_FINISHED");
					break;
				case AEL_STATUS_MOUNTED:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_MOUNTED");
					break;
				case AEL_STATUS_UNMOUNTED:
					ESP_LOGI(TAG, "[ X ]   status AEL_STATUS_UNMOUNTED");
					break;
			}

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
			&& msg.source == (void *) snapclient_stream
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
			ESP_LOGI(TAG, "[ X ] report music info ");
            audio_element_info_t music_info = {0};
            audio_element_getinfo(snapclient_stream, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from snapclient decoder, sample_rates=%d, bits=%d, ch=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(i2s_stream_writer, &music_info);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates , music_info.bits, music_info.channels);
            continue;
        }

        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
			ESP_LOGI(TAG, "[ X ] i2s wants to stop!");

            //break;
        }
    }

    ESP_LOGI(TAG, "[ 5 ] Stop audio_pipeline");
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

	audio_pipeline_unregister(pipeline, snapclient_stream);
    //audio_pipeline_unregister(pipeline, opus_decoder);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    /* Make sure audio_pipeline_remove_listener is called before destroying event_iface */
    audio_event_iface_destroy(evt);

    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_stream_writer);
    audio_element_deinit(snapclient_stream);
}
