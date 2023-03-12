#include "esp_log.h"
#include "esp_err.h"
#include "lwip/sockets.h"
#include "esp_transport_tcp.h"
#include "audio_mem.h"
#include "snapclient_stream.h"
#include "hello.h"
#include "codec_header.h"
#include "wire_chunk.h"
#include "server_settings.h"
#include "sample_format.h"
#include "time_message.h"
#include "controller.h"
#include "audio_element.h"
#include "ringbuf.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "client_connection.h"

static const char *TAG = "SNAPCLIENT_STREAM";
#define CONNECT_TIMEOUT_MS        500

static audio_hal_handle_t s_volume_handle;


//static snapclient_stream_t *snapclient = NULL;
static TimerHandle_t        send_time_tm_handle;

static void send_time_timer_cb(TimerHandle_t xTimer)
{
    ESP_LOGD(TAG, "Send time cb");
    struct timeval now;
	char message_serialized[BASE_MESSAGE_SIZE];

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
	snapclient->last_sync.tv_sec = now.tv_sec;
	snapclient->last_sync.tv_usec = now.tv_usec;

	base_message_t message = {
		SNAPCAST_MESSAGE_TIME,      // type
		snapclient->id_counter++,   // id
		0x0,                         // refersTo
		{ now.tv_sec, now.tv_usec }, // sent
		{ 0x0, 0x0 },                // received
		TIME_MESSAGE_SIZE           // size
	};

	if (base_message_serialize(
			&message,
			message_serialized)) {
		ESP_LOGE(TAG, "Failed to serialize base message for time\r\n");
		return;
	}
	esp_transport_write(snapclient->t,
						message_serialized, BASE_MESSAGE_SIZE,
						snapclient->timeout_ms);

	if (time_message_serialize(&(snapclient->time_message), message_serialized)) {
		ESP_LOGI(TAG, "Failed to serialize time message\r\b");
		return;
	}

	esp_transport_write(snapclient->t,
						message_serialized, sizeof(time_message_t),
						snapclient->timeout_ms);
	//ESP_LOGI(TAG, "SENT time message");
}

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
	int result;
    struct timeval now;
	ESP_LOGI(TAG, "OPENING Snapclient stream");

    snapclient = (snapclient_stream_t *)audio_element_getdata(self);
    if (snapclient->is_open) {
        ESP_LOGE(TAG, "Already opened");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Host is %s, port is %d\n", snapclient->host, snapclient->port);

    esp_transport_handle_t t = esp_transport_tcp_init();

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

    snapclient->is_open = true;
    snapclient->t = t;
	// snapclient->base_message.type = SNAPCAST_MESSAGE_BASE;  // default state, no current message
	// snapclient->base_message.sent.sec = 0;
	// snapclient->base_message.sent.usec = 0;
	// snapclient->base_message.received.sec = 0;
	// snapclient->base_message.received.usec = 0;
	snapclient->received_header = false;
	snapclient->last_sync.tv_sec = 0;
	snapclient->last_sync.tv_usec = 0;
	snapclient->id_counter = 0;
	snapclient->time_message.latency.sec = 0;
	snapclient->time_message.latency.usec = 0;

	char mac_address[18];
    uint8_t base_mac[6];
    // Get MAC address for WiFi station

    esp_read_mac(base_mac, ESP_MAC_WIFI_STA);
    sprintf(mac_address,
			"%02X:%02X:%02X:%02X:%02X:%02X",
			base_mac[0], base_mac[1], base_mac[2], base_mac[3], base_mac[4], base_mac[5]);

	result = gettimeofday(&now, NULL);
	if (result) {
		ESP_LOGI(TAG, "Failed to gettimeofday\r\n");
		return ESP_FAIL;
	}

	base_message_t base_message = {
		SNAPCAST_MESSAGE_HELLO,      // type
		0x0,                         // id
		0x0,                         // refersTo
		{ now.tv_sec, now.tv_usec }, // sent
		{ 0x0, 0x0 },                // received
		0x0,                         // size
	};

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

    char base_message_serialized[sizeof(base_message_t)];
    char *hello_message_serialized;

	// serialize the hello message putting the computed size in base_messge.size
	hello_message_serialized = hello_message_serialize(&hello_message, &(base_message.size));

	if (!hello_message_serialized) {
		ESP_LOGI(TAG, "Failed to serialize hello message\r\b");
		return ESP_FAIL;
	}

	result = base_message_serialize(&base_message, base_message_serialized);

	if (result) {
		ESP_LOGI(TAG, "Failed to serialize base message\r\n");
        return ESP_FAIL;
	}

	result = esp_transport_write(snapclient->t,
								 base_message_serialized, sizeof(base_message_t),
								 snapclient->timeout_ms);
    if (result < 0) {
        _get_socket_error_code_reason("TCP write", snapclient->sock);
        goto _snapclient_open_exit;
    }
	result = esp_transport_write(snapclient->t,
								 hello_message_serialized, base_message.size,
								 snapclient->timeout_ms);
    if (result < 0) {
        _get_socket_error_code_reason("TCP write", snapclient->sock);
        goto _snapclient_open_exit;
    }
	free(hello_message_serialized);

	// start the one second timer that sends Time messages
	send_time_tm_handle = xTimerCreate(
		"snapclient_timer0", 1000 / portTICK_RATE_MS,
		pdTRUE, NULL, send_time_timer_cb);
	xTimerStart(send_time_tm_handle, 0);

    _dispatch_event(self, snapclient, NULL, 0, SNAPCLIENT_STREAM_STATE_CONNECTED);
	ESP_LOGI(TAG, "snapclient_stream_open OK");

    return ESP_OK;

_snapclient_open_exit:
    free(hello_message_serialized);
    return ESP_FAIL;
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

static esp_err_t _snapclient_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    snapclient_stream_t *snapclient = (snapclient_stream_t *)audio_element_getdata(self);
	char *buff = buffer;
	int rem = len;
	int rlen;
	int loop = 0;
	while (rem > 0)
	{
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
		rem -= rlen;
		loop += 1;
	}

    // ESP_LOGI(TAG, "read len=%d, loops=%d", len, loop);
    if (rem > 0) {
        ESP_LOGI(TAG, "Could not read the whole buffer!");
    }
	audio_element_update_byte_pos(self, len);
    return len;
}



static esp_err_t _snapclient_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    // struct timeval now, tv1, tv3; //, last_time_sync;
	// int32_t buffer_ms = 0;
	// int result        = 0;
    // int r_size        = 0;
    // int w_size        = 0;
	// int size          = 0;
	// int message_size  = 0;
	// char *start;
	//audio_element_info_t snap_info = {0};

	// ESP_LOGI(TAG, "Process: %d available bytes", in_len);

	//snapclient_stream_t *snapclient = (snapclient_stream_t *)audio_element_getdata(self);
	void* message;

	//start = in_buffer;

	while(true) {

		// if (snapclient->base_message.type == SNAPCAST_MESSAGE_BASE)
		// 	message_size = BASE_MESSAGE_SIZE;
		// else
		// 	message_size = snapclient->base_message.size;

		// if (in_len < message_size) {
		// 	// not enough data available, exit this loop
		// 	//ESP_LOGD(TAG, "Not enought data left for message %d: %d/%d",
		// 	//		 snapclient->base_message.type, in_len, message_size);
		// 	break;
		// }
		message = getNextMessage(self, in_buffer);
		ESP_LOGI(TAG, "Message received %d !!", (uint32_t)(message));
		controller_getNextMessage(self, (base_message_t*) message, s_volume_handle);

		// r_size = audio_element_input(self, in_buffer, message_size);
		// if (r_size <= 0) {
		// 	// ring buffer cannot provide enough data (weird...)
		// 	ESP_LOGI(TAG, "Cannot retrieved %d bytes of data!!!", message_size);
		// 	break;
		// }
		// in_len -= r_size;
		// if (r_size < message_size)  // XXX
		// {
		// 	ESP_LOGE(TAG, "Retrieved only %d bytes of data instead of %d (on %d)!!! ABORT", r_size, message_size, in_len);
		// 	snapclient->base_message.type = SNAPCAST_MESSAGE_BASE;
		// 	break;
		// 	//return ESP_FAIL;
		// }

	// 	switch (snapclient->base_message.type) {
	// 		// case SNAPCAST_MESSAGE_BASE:
	// 		// 	result = base_message_deserialize(&(snapclient->base_message), in_buffer);
	// 		// 	if (result) {
	// 		// 		ESP_LOGI(TAG, "Failed to read base message: %d\r\n", result);
	// 		// 		break; //return ESP_FAIL;
	// 		// 	}
	// 		// 	break;
	// 		case SNAPCAST_MESSAGE_TIME:
	// 			/*
	// 			Normally received as a reply to the TIME message sent once a
	// 			second by the client (this code).

	// 			Protocol doc says:

	// 			- Client periodically sends a Time message, carrying a sent
    //               timestamp (t_client-sent)

	// 			- Receives a Time response containing the "client to server"
    //               time delta:

	// 			  latency_c2s = t_server-recv - t_client-sent +
    //                             t_network-latency

	// 			  and the server sent timestamp (t_server-sent)

	// 			- Calculates:

	// 			  latency_s2c = t_client-recv - t_server-sent + t_network_latency

	// 			- Calcutates the time diff between server and client as
    //               (latency_c2s - latency_s2c) / 2, eliminating the network
    //               latency (assumed to be symmetric)

	// 			*/
	// 			// ESP_LOGD(TAG, "SNAPCAST_MESSAGE_TIME (size=%d/%d)", message_size, r_size);
	// 			// // snapclient->base_message.type = SNAPCAST_MESSAGE_BASE;
	// 			// result = time_message_deserialize(&(snapclient->time_message), in_buffer /*, message_size*/);
	// 			// if (result) {
	// 			// 	ESP_LOGI(TAG, "Failed to deserialize time message\r\n");
	// 			// 	break;
	// 			// }
	// 			// /*
	// 			// ESP_LOGI(TAG, "BaseTX     : %d %d ", snapclient->base_message.sent.sec , snapclient->base_message.sent.usec);
	// 			// ESP_LOGI(TAG, "BaseRX     : %d %d ", snapclient->base_message.received.sec , snapclient->base_message.received.usec);
	// 			// ESP_LOGI(TAG, "baseTX->RX : %d s ", (snapclient->base_message.received.sec - snapclient->base_message.sent.sec);
	// 			// ESP_LOGI(TAG, "baseTX->RX : %d ms ", (snapclient->base_message.received.usec - snapclient->base_message.sent.usec)/1000);
	// 			// ESP_LOGI(TAG, "Latency : %d.%d ", snapclient->time_message.latency.sec,  snapclient->time_message.latency.usec/1000);
	// 			// */

	// 			// // tv == server to client latency (s2c)
	// 			// // time_message.latency == client to server latency(c2s)
	// 			// // TODO the fact that I have to do this simple conversion means
	// 			// // I should probably use the timeval struct instead of my own
	// 			// tv_t timeMsg = { 0, 0 };

	// 			// struct timeval s2c = { 0, 0 };
	// 			// struct timeval c2s = { 0, 0 };
	// 			// tv1.tv_sec = snapclient->base_message.received.sec;
	// 			// tv1.tv_usec = snapclient->base_message.received.usec;
	// 			// tv3.tv_sec = snapclient->base_message.sent.sec;
	// 			// tv3.tv_usec = snapclient->base_message.sent.usec;
	// 			// //
	// 			// timersub(&tv1, &tv3, &s2c);
	// 			// c2s.tv_sec = snapclient->time_message.latency.sec;
	// 			// c2s.tv_usec = snapclient->time_message.latency.usec;

	// 			// Note: server sends timestamps as seconds since boot, not epoch
	// 			/*
	// 			ESP_LOGI(TAG, "c2s: %ld %ld", c2s.tv_sec, c2s.tv_usec);
	// 			ESP_LOGI(TAG, "s2c: %ld %ld", s2c.tv_sec, s2c.tv_usec);
	// 			double time_diff = (
	// 				(((double)(c2s.tv_sec - s2c.tv_sec) / 2) * 1000) +
	// 				(((double)(c2s.tv_usec - s2c.tv_usec) / 2) / 1000));
	// 			ESP_LOGI(TAG, "Time diff: %.2fms", time_diff);
	// 			*/
	// 			break;

	// 		// case SNAPCAST_MESSAGE_CLIENT_INFO:
	// 		// 	ESP_LOGI(TAG, "SNAPCAST_MESSAGE_STREAM_TAGS (size=%d/%d) [IGNORED]", message_size, r_size);
	// 		// 	snapclient->base_message.type = SNAPCAST_MESSAGE_BASE;
	// 		// 	break;

	// 		// default:
	// 		// 	ESP_LOGI(TAG, "UNKNOWN_MESSAGE_TYPE %d (size=%d/%d)",
	// 		// 			 snapclient->base_message.type, message_size, r_size);
	// 		// 	snapclient->base_message.type = SNAPCAST_MESSAGE_BASE;
	// 		//	break;

	// 	} // switch
	// }  // while(r_size)
	//ESP_LOGI(TAG, "PROCESSING DONE");
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

	audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
	audio_element_handle_t el;
	s_volume_handle = (audio_hal_handle_t) volume_handle;
    cfg.open = _snapclient_open;
    cfg.close = _snapclient_close;
    cfg.process = _snapclient_process;
    cfg.destroy = _snapclient_destroy;

    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.stack_in_ext = config->ext_stack;
    cfg.out_rb_size = config->out_rb_size;
    cfg.tag = "snapclient_client";

	cfg.buffer_len = SNAPCLIENT_STREAM_BUF_SIZE;
	if (config->type == AUDIO_STREAM_READER) {
        cfg.read = _snapclient_read;
    } else if (config->type == AUDIO_STREAM_WRITER) {
        ESP_LOGE(TAG, "No writer for snapclient stream");
        goto _snapclient_init_exit;
    }

	snapclient_stream_t *snapclient = audio_calloc(1, sizeof(snapclient_stream_t));
    AUDIO_MEM_CHECK(TAG, snapclient, return NULL);

    snapclient->port = config->port;
    snapclient->host = config->host;
    snapclient->timeout_ms = config->timeout_ms;
	snapclient->player = config->player;

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
