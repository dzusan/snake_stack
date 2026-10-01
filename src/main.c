/*
 * lenkov.work
 */

#include "tx_api.h"
#include "clock.h"
#include "heater.h"

int main(void)
{
    clock_setup();
    sys_tick_init();
    tx_kernel_enter();
    return 0;
}
