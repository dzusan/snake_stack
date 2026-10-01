#ifndef __TICK_H__
#define __TICK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void sys_tick_init(void);
uint64_t sys_get_tick(void);
void sys_tick_delay(uint64_t ms);
void clock_setup(void);

#ifdef __cplusplus
}
#endif
#endif /* __TICK_H__ */