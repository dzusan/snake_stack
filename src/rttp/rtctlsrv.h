#ifndef __RTCTLSRV_H__
#define __RTCTLSRV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void rtctlsrv_init(void);
uint32_t rtctlsrv_get_y(void);
int32_t rtctlsrv_get_temp(void);

#ifdef __cplusplus
}
#endif
#endif /* __RTCTLSRV_H__ */