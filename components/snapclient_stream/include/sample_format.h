#ifndef __SAMPLE_FORMAT_H__
#define __SAMPLE_FORMAT_H__

#include <stdint.h>

typedef struct sample_s{
    uint16_t sample_size_;
    uint16_t frame_size_;
    uint32_t rate_;
    uint16_t bits_;
    uint16_t channels_;
} sample_t;

int sample_format_message_deserialize(sample_t *msg, const char *data, uint32_t size);
// void setFormat(const char* format);
// void setFormat(uint32_t rate, uint16_t bits, uint16_t channels);


#endif // __SAMPLE_FORMAT_H__