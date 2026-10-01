#include "clock.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/systick.h>

static volatile uint64_t _systick = 0;

void clock_setup(void)
{
    /* libopencm3/ethernet/mac_stm32fxx7.h allow max 168 MHz */
    rcc_clock_setup_hse(&rcc_3v3[RCC_CLOCK_3V3_168MHZ], 25);
}

void sys_tick_init(void)
{
    /* TODO: use systick_set_frequency() and systick_clear() */

    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    STK_CVR = 0;
    systick_set_reload(rcc_ahb_frequency / 1000 - 1);
    systick_interrupt_enable();
    systick_counter_enable();
}

uint64_t sys_get_tick(void)
{
    return _systick;
}

void sys_tick_delay(uint64_t ms)
{
    const uint64_t wait = _systick + ms;

    while (_systick < wait);
}
