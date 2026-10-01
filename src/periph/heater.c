#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "tx_api.h"
#include "heater.h"

static void led_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOI);
    gpio_mode_setup(GPIOI, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
}

void heater_thread(unsigned long p)
{
    led_init();
    for(;;)
    {
        gpio_toggle(GPIOI, GPIO1);
        tx_thread_sleep(200);
    }
}
