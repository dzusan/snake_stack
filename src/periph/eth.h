#ifndef __ETH_H__
#define __ETH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <lwip/def.h>
#include <lwip/netif.h>
#include <lwip/err.h>

void sys_ethmac_init(void);
err_t sys_ethmac_netif_init(struct netif* netif);
void sys_ethmac_poll(int phy, struct netif* netif);

#ifdef __cplusplus
}
#endif
#endif /* __ETH_H__ */
