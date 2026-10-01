#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/ethernet/phy_lan87xx.h>

#include <netif/etharp.h>
#include <lwip/init.h>
#include <lwip/dhcp.h>
#include <lwip/ip_addr.h>

#include "tx_api.h"
#include "eth.h"
#include "rtctlsrv.h"
#include "rtmon.h"

struct netif networkif;
static ip_addr_t ipaddr, netmask, gateway;

void ip_thread(unsigned long p)
{
    sys_ethmac_init();

    lwip_init();
    ip4addr_aton("192.168.1.110", &ipaddr);
    ip4addr_aton("255.255.255.0", &netmask);
    ip4addr_aton("192.168.1.10", &gateway);
    netif_add(&networkif, &ipaddr, &netmask, &gateway, NULL, &sys_ethmac_netif_init, &ethernet_input);
    netif_set_default(&networkif);
    netif_set_up(&networkif);

    rtctlsrv_init();
    rtmon_init();

    for (;;)
    {
        sys_ethmac_poll(PHY0, &networkif);
        rtmon_proc();
        tx_thread_relinquish();
    }
}
