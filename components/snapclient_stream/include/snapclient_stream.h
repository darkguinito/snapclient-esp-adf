
#ifndef __SNAPCLIENT_STREAM_H__
#define __SNAPCLIENT_STREAM_H__

#include "audio_error.h"
#include "audio_element.h"
#include "esp_transport.h"
#include "audio_hal.h"
#include "client_settings.h"

typedef enum {
    SNAPCLIENT_STREAM_STATE_NONE,
    SNAPCLIENT_STREAM_STATE_CONNECTED,
    SNAPCLIENT_STREAM_STATE_SYNC,
} snapclient_stream_status_t;


typedef struct snapclient_stream_event_msg {
    void                          *source;          /*!< Element handle */
    void                          *data;            /*!< Data of input/output */
    int                           data_len;         /*!< Data length of input/output */
    esp_transport_handle_t        sock_fd;          /*!< handle of socket*/
} snapclient_stream_event_msg_t;


typedef esp_err_t (*snapclient_stream_event_handle_cb)(snapclient_stream_event_msg_t *msg, snapclient_stream_status_t state, void *event_ctx);

/**
 * @brief Stream configuration, if any entry is zero then the configuration
 * will be set to default values
 */
typedef struct {
    audio_stream_type_t                type;               /*!< Type of stream */
    int                                out_rb_size;        /*!< Size of output ringbuffer */
    int                                timeout_ms;         /*!< time timeout for read/write*/
    int                                port;               /*!< TCP port> */
    char                               *host;              /*!< TCP host> */
    int                                task_stack;         /*!< Task stack size */
    int                                task_core;          /*!< Task running in core (0 or 1) */
    int                                task_prio;          /*!< Task priority (based on freeRTOS priority) */
    bool                               ext_stack;          /*!< Allocate stack on extern ram */
    snapclient_stream_event_handle_cb  event_handler;      /*!< snapclient stream event callback*/
    player_t                           player;             /*!< Player settings */
    void                               *event_ctx;         /*!< User context*/
} snapclient_stream_cfg_t;

#define SNAPCLIENT_DEFAULT_PORT             (1704)
#define SNAPCLIENT_STREAM_TASK_STACK        (3072)
#define SNAPCLIENT_STREAM_BUF_SIZE          (4096)
#define SNAPCLIENT_STREAM_TASK_PRIO         (5)
#define SNAPCLIENT_STREAM_TASK_CORE         (0)
#define SNAPCLIENT_STREAM_CLIENT_NAME       ("esp32")
#define SNAPCLIENT_STREAM_RINGBUFFER_SIZE   (20 * 1024)

#define SNAPCLIENT_STREAM_CFG_DEFAULT() {               \
    .type          = AUDIO_STREAM_READER,               \
    .out_rb_size   = SNAPCLIENT_STREAM_RINGBUFFER_SIZE, \
    .timeout_ms    = 30 *1000,                          \
    .port          = SNAPCLIENT_DEFAULT_PORT,           \
    .host          = NULL,                              \
    .task_stack    = SNAPCLIENT_STREAM_TASK_STACK,      \
    .task_core     = SNAPCLIENT_STREAM_TASK_CORE,       \
    .task_prio     = SNAPCLIENT_STREAM_TASK_PRIO,       \
    .ext_stack     = true,                              \
    .event_handler = NULL,                              \
    .event_ctx     = NULL,                              \
}

audio_element_handle_t snapclient_stream_init(snapclient_stream_cfg_t *config, audio_hal_handle_t volume_handle);

#endif
