#ifndef __RTUTIL_H__
#define __RTUTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint32_t _crc32(uint32_t seed, uint8_t* data, int size);

#ifdef __cplusplus
}
#endif
#endif /* __RTUTIL_H__ */