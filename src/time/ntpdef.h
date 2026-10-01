#ifndef __NTPDEF_H__
#define __NTPDEF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#pragma pack(push, 1)
struct ntp_ts
{
    uint32_t integral;
    uint32_t frac;
};
#pragma pack(pop)

#ifdef __cplusplus
}
#endif
#endif /* __NTPDEF_H__ */