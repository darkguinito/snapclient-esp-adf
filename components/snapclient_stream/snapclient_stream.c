#include "snapclient_stream.h"

#include "audio_mem.h"
#include "audio_element.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_transport_tcp.h"
#include "freertos/timers.h"
#include "lwip/sockets.h"
#include "ringbuf.h"

#include "client_connection.h"
#include "hello.h"

static const char *TAG = "SNAPCLIENT_STREAM";
#define CONNECT_TIMEOUT_MS        500

static audio_hal_handle_t s_volume_handle;
static TimerHandle_t      send_time_tm_handle;

static int _get_socket_error_code_reason(char *str, int sockfd)
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

static void send_time_timer_cb(TimerHandle_t xTimer)
{
    ESP_LOGD(TAG, "Send time cb");
    struct timeval now;
	time_message_full_t message = { 0 };
	
	if (snapclient == NULL) {
		ESP_LOGI(TAG, "snapclient not initialized, ignoring");
		return;
	}
	if (!snapclient->received_header) {
		ESP_LOGI(TAG, "NO Codec HEADER received, (not) ignoring");
		return;
	}

	if (gettimeofday(&now, NULL)) {
	 	ESP_LOGI(TAG, "Failed to gettimeofday\r\n");
		return;
	}

	message.base.type      = SNAPCAST_MESSAGE_TIME;
	message.base.id        = snapclient->id_counter++;
	message.base.sent.sec  = now.tv_sec; 
	message.base.sent.usec = now.tv_usec;
	message.base.size      = TIME_MESSAGE_SIZE;

	esp_transport_write(snapclient->t, (const char*)(&message), sizeof(time_message_full_t), snapclient->timeout_ms);
}

static esp_err_t _dispatch_event(audio_element_handle_t el, snapclient_stream_t *snapclient, void *data, int len, snapclient_stream_status_t state) 
{
    if (el && snapclient && snapclient->hook) {
        snapclient_stream_event_msg_t msg = { 0 };
        msg.data     = data;
        msg.data_len = len;
        msg.sock_fd  = snapclient->t;
        msg.source   = el;
        return snapclient->hook(&msg, state, snapclient->ctx);
    }
    return ESP_FAIL;
}

static esp_err_t _snapclient_open(audio_element_handle_t self)
{
    AUDIO_NULL_CHECK(TAG, self, return ESP_FAIL);

	esp_err_t return_code = ESP_FAIL;
	int result;
    struct timeval now;
	ESP_LOGI(TAG, "OPENING Snapclient stream");

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

    snapclient->sock = esp_transport_connect(t, snapclient->host, snapclient->port, CONNECT_TIMEOUT_MS);
    if (snapclient->sock < 0) {
        _get_socket_error_code_reason("TCP create",  snapclient->sock);
        esp_transport_destroy(t);
        esp_transport_list_destroy(transport_list);
        return ESP_FAIL;
    }

    snapclient->is_open           = true;
    snapclient->t                 = t;
	snapclient->received_header   = false;
	snapclient->last_sync.tv_sec  = 0;
	snapclient->last_sync.tv_usec = 0;
	snapclient->id_counter        = 0;

	char mac_address[18];
    uint8_t base_mac[6];

    // Get MAC address for WiFi station
    esp_read_mac(base_mac, ESP_MAC_WIFI_STA);
    sprintf(mac_address, "%02X:%02X:%02X:%02X:%02X:%02X",
			base_mac[0], base_mac[1], base_mac[2], base_mac[3], base_mac[4], base_mac[5]);

	result = gettimeofday(&now, NULL);
	if (result) {
		ESP_LOGI(TAG, "Failed to gettimeofday\r\n");
		return ESP_FAIL;
	}
	snapclient->base_message.type      = SNAPCAST_MESSAGE_HELLO;
	snapclient->base_message.sent.sec  = now.tv_sec;
	snapclient->base_message.sent.usec = now.tv_usec;

	hello_message_t hello_message = {
		mac_address,
		SNAPCLIENT_STREAM_CLIENT_NAME, // hostname
		"0.0.2",                       // client version
		"libsnapcast",                 // client name
		"esp32_d",                     // os name
		"xtensa",                      // arch
		1,                             // instance
		mac_address,                   // id
		2,                             // protocol version
	};

    char *hello_message_serialized;
	hello_message_serialized = hello_message_serialize(&hello_message, &(snapclient->base_message.size));

	if (!hello_message_serialized) {
		ESP_LOGI(TAG, "Failed to serialize hello message\r\b");
		return ESP_FAIL;
	}

	result = esp_transport_write(snapclient->t, &(snapclient->base_message), sizeof(base_message_t), snapclient->timeout_ms);

    if (result < 0) {
        _get_socket_error_code_reason("TCP write", snapclient->sock);
        goto _snapclient_open_exit;
    }

	result = esp_transport_write(snapclient->t, hello_message_serialized, snapclient->base_message.size, snapclient->timeout_ms);

    if (result < 0) {
        _get_socket_error_code_reason("TCP write", snapclient->sock);
        goto _snapclient_open_exit;
    }

	snapclient->base_message.type = SNAPCAST_MESSAGE_BASE;
	send_time_tm_handle           = xTimerCreate("snapclient_timer0", 1000 / portTICK_RATE_MS, pdTRUE, NULL, send_time_timer_cb);
	xTimerStart(send_time_tm_handle, 0);

    _dispatch_event(self, snapclient, NULL, 0, SNAPCLIENT_STREAM_STATE_CONNECTED);
	ESP_LOGI(TAG, "snapclient_stream_open OK");
	return_code = ESP_OK;

_snapclient_open_exit:
    free(hello_message_serialized);
    return return_code;
}

static esp_err_t _snapclient_close(audio_element_handle_t self)
{
	ESP_LOGI(TAG, "CLOSING Snapclient stream");

    AUDIO_NULL_CHECK(TAG, self, return ESP_FAIL);

    snapclient_stream_t *snapclient = (snapclient_stream_t *)audio_element_getdata(self);
    AUDIO_NULL_CHECK(TAG, snapclient, return ESP_FAIL);
    if (!snapclient->is_open) {
        ESP_LOGE(TAG, "Already closed");
        return ESP_FAIL;
    }
    if (-1 == esp_transport_close(snapclient->t)) {
        ESP_LOGE(TAG, "Snapclient stream close failed");
        return ESP_FAIL;
    }
    snapclient->is_open = false;
    if (AEL_STATE_PAUSED != audio_element_get_state(self)) {
        audio_element_set_byte_pos(self, 0);
    }
    return ESP_OK;
}

static esp_err_t _snapclient_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context) {
	char *buff = buffer;
	int rem    = len;
	int rlen   = 0;
    snapclient_stream_t *snapclient = (snapclient_stream_t *)audio_element_getdata(self);

	while (rem > 0) {
		rlen = esp_transport_read(snapclient->t, buff, rem, snapclient->timeout_ms);

		if (rlen < 0) {
			ESP_LOGE(TAG, "Error reading th TCP socket");
			_get_socket_error_code_reason("TCP read", snapclient->sock);
			return ESP_FAIL;
		} else if (rlen == 0) {
			ESP_LOGI(TAG, "Get end of the file");
			break;
		}

		buff += rlen;
		rem  -= rlen;
	}
    if (rem > 0) {
        ESP_LOGI(TAG, "Could not read the whole buffer!");
    }
	audio_element_update_byte_pos(self, len);
    return len;
}

int read_buf(audio_element_handle_t self, char *in_buffer, int buffer_size, int read_size){
	if(read_size > buffer_size) {
		ESP_LOGD(TAG, "Remaining Buffer to small, Asked %d on %d!!!", read_size, buffer_size);
		return 0;
	}

	return audio_element_input(self, in_buffer, read_size);
}

static esp_err_t _snapclient_process(audio_element_handle_t self, char *in_buffer, int in_len) {
	audio_element_err_t rc_input  = 0;
	int                 b_read    = 0;
	size_t              remaining = in_len;
	snapclient_stream_t *snapclient = (snapclient_stream_t *)audio_element_getdata(self);

	while(remaining > 0) {

		if (snapclient->base_message.type == SNAPCAST_MESSAGE_BASE){

			b_read = read_buf(self, in_buffer, remaining, sizeof(base_message_t));

			if (b_read <= 0) {
				ESP_LOGD(TAG, "Cannot retrieved %d bytes of data %d!!!", sizeof(base_message_t), rc_input);
				break;
			} else {
				remaining -= b_read;
			}
			base_message_deserialize(&snapclient->base_message, in_buffer);
		//	print_base_message(&snapclient->base_message);
		}
		else {
			b_read = read_buf(self, in_buffer, remaining, snapclient->base_message.size);

			if (b_read <= 0) {
				ESP_LOGD(TAG, "Cannot retrieved %d bytes of data %d!!!", sizeof(base_message_t), rc_input);
				break;
			} else {
				remaining -= b_read;
			}
			createMessage(&snapclient->base_message, in_buffer, self, s_volume_handle, &(snapclient->time_message));
			if (snapclient->base_message.type == SNAPCAST_MESSAGE_CODEC_HEADER){
				snapclient->received_header = true;
			} 
			snapclient->base_message.type = SNAPCAST_MESSAGE_BASE;
		}
	}
	return 1;  // Make sure we are not considered as closed
}

static esp_err_t _snapclient_destroy(audio_element_handle_t self)
{
    AUDIO_NULL_CHECK(TAG, self, return ESP_FAIL);
    snapclient_stream_t *snapclient = (snapclient_stream_t *)audio_element_getdata(self);
    AUDIO_NULL_CHECK(TAG, snapclient, return ESP_FAIL);

    if (snapclient->t) {
        esp_transport_destroy(snapclient->t);
        snapclient->t = NULL;
    }
    audio_free(snapclient);
    return ESP_OK;
}

audio_element_handle_t snapclient_stream_init(snapclient_stream_cfg_t *config, audio_hal_handle_t volume_handle)
{
	AUDIO_NULL_CHECK(TAG, config, return NULL);
	ESP_LOGI(TAG, "snapclient_stream_init");

	audio_element_cfg_t    cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
	audio_element_handle_t el;

	s_volume_handle = (audio_hal_handle_t) volume_handle;

    cfg.open         = _snapclient_open;
    cfg.close        = _snapclient_close;
    cfg.process      = _snapclient_process;
    cfg.destroy      = _snapclient_destroy;
    cfg.task_stack   = config->task_stack;
    cfg.task_prio    = config->task_prio;
    cfg.task_core    = config->task_core;
    cfg.stack_in_ext = config->ext_stack;
    cfg.out_rb_size  = config->out_rb_size;
    cfg.tag          = "snapclient_client";
	cfg.buffer_len   = SNAPCLIENT_STREAM_BUF_SIZE;

	if (config->type == AUDIO_STREAM_READER) {
        cfg.read = _snapclient_read;
    } else if (config->type == AUDIO_STREAM_WRITER) {
        ESP_LOGE(TAG, "No writer for snapclient stream");
        goto _snapclient_init_exit;
    }

	snapclient_stream_t *snapclient = audio_calloc(1, sizeof(snapclient_stream_t));
    AUDIO_MEM_CHECK(TAG, snapclient, return NULL);

	memset(&snapclient->time_message, 0, sizeof(time_message_full_t));
	memset(&snapclient->base_message, 0, sizeof(base_message_t));
    snapclient->port              = config->port;
    snapclient->host              = config->host;
    snapclient->timeout_ms        = config->timeout_ms;
	snapclient->player            = config->player;
	snapclient->base_message.type = SNAPCAST_MESSAGE_BASE;

    if (config->event_handler) {
        snapclient->hook = config->event_handler;
        if (config->event_ctx) {
            snapclient->ctx = config->event_ctx;
        }
    }

    el = audio_element_init(&cfg);
    AUDIO_MEM_CHECK(TAG, el, goto _snapclient_init_ini_exit);
    audio_element_setdata(el, snapclient);

	ESP_LOGI(TAG, "snapclient_stream_init OK");

    return el;

_snapclient_init_ini_exit:
    audio_free(snapclient);
_snapclient_init_exit:
    return NULL;
}
