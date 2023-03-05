#include "hello.h"

//#ifdef ESP_PLATFORM
// The ESP-IDF changes the include directory for cJSON
#include <cJSON.h>
//#else
//#include "json/cJSON.h"
//#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <buffer.h>

static cJSON* hello_message_to_json(hello_message_t *msg) {
    cJSON *mac;
    cJSON *hostname;
    cJSON *version;
    cJSON *client_name;
    cJSON *os;
    cJSON *arch;
    cJSON *instance;
    cJSON *id;
    cJSON *protocol_version;
    cJSON *json = NULL;

    json = cJSON_CreateObject();
    if (!json) {
        goto error;
    }

    mac = cJSON_CreateString(msg->mac);
    if (!mac) {
        goto error;
    }
    cJSON_AddItemToObject(json, "MAC", mac);

    hostname = cJSON_CreateString(msg->hostname);
    if (!hostname) {
        goto error;
    }
    cJSON_AddItemToObject(json, "HostName", hostname);

    version = cJSON_CreateString(msg->version);
    if (!version) {
        goto error;
    }
    cJSON_AddItemToObject(json, "Version", version);

    client_name = cJSON_CreateString(msg->client_name);
    if (!client_name) {
        goto error;
    }
    cJSON_AddItemToObject(json, "ClientName", client_name);

    os = cJSON_CreateString(msg->os);
    if (!os) {
        goto error;
    }
    cJSON_AddItemToObject(json, "OS", os);

    arch = cJSON_CreateString(msg->arch);
    if (!arch) {
        goto error;
    }
    cJSON_AddItemToObject(json, "Arch", arch);

    instance = cJSON_CreateNumber(msg->instance);
    if (!instance) {
        goto error;
    }
    cJSON_AddItemToObject(json, "Instance", instance);

    id = cJSON_CreateString(msg->id);
    if (!id) {
        goto error;
    }
    cJSON_AddItemToObject(json, "ID", id);

    protocol_version = cJSON_CreateNumber(msg->protocol_version);
    if (!protocol_version) {
        goto error;
    }
    cJSON_AddItemToObject(json, "SnapStreamProtocolVersion", protocol_version);

    goto end;
error:
    cJSON_Delete(json);

end:
    return json;
}

char* hello_message_serialize(hello_message_t* msg, size_t *size) {
    size_t s_str = 0;
    size_t s_prefixed = 0;
    cJSON *p__json;
    char *p_c_str = NULL;
	char *p_c_prefixed_str = NULL;

    p__json = hello_message_to_json(msg);
    if (!p__json) {
        return NULL;
    }

    p_c_str = cJSON_PrintUnformatted(p__json);
	if (!p_c_str) {
		return NULL;
	}
    cJSON_Delete(p__json);

	s_str = strlen(p_c_str);
	s_prefixed = s_str + 4;
	p_c_prefixed_str = malloc(s_prefixed);
	if (!p_c_prefixed_str) {
		return NULL;
	}

	p_c_prefixed_str[0] = s_str & 0xff;
	p_c_prefixed_str[1] = (s_str >> 8) & 0xff;
	p_c_prefixed_str[2] = (s_str >> 16) & 0xff;
	p_c_prefixed_str[3] = (s_str >> 24) & 0xff;
	memcpy(&(p_c_prefixed_str[4]), p_c_str, s_str);
	free(p_c_str);
	*size = s_prefixed;

    return p_c_prefixed_str;
}
