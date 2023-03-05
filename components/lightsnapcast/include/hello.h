#ifndef __SNAPCAST_HELLO_H__
#define __SNAPCAST_HELLO_H__

#include <stdint.h>
#include <stddef.h>


/* Sample Hello message
{
    "Arch": "x86_64",
    "ClientName": "Snapclient",
    "HostName": "my_hostname",
    "ID": "00:11:22:33:44:55",
    "Instance": 1,
    "MAC": "00:11:22:33:44:55",
    "OS": "Arch Linux",
    "SnapStreamProtocolVersion": 2,
    "Version": "0.17.1"
}
*/

typedef struct hello_message {
    char *mac;
    char *hostname;
    char *version;
    char *client_name;
    char *os;
    char *arch;
    int  instance;
    char *id;
    int  protocol_version;
} hello_message_t;


char* hello_message_serialize(hello_message_t* msg, size_t *size);

#endif // __SNAPCAST_HELLO_H__