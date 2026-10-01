#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/ethernet/mac_stm32fxx7.h>
#include <libopencm3/ethernet/phy_lan87xx.h>
#include <libopencm3/stm32/common/syscfg_common_l1f234.h>

#include "tx_api.h"

#include <lwip/def.h>
#include <lwip/mem.h>
#include <lwip/pbuf.h>
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include <netif/etharp.h>

#include "eth.h"
#include "io746.h"
#include "clock.h"

#define MII_RMII_SEL  (1<<23) /* p. 239 */


/********** Constants copied from STM32 HAL ETH *******************/

#define ETH_MAX_PACKET_SIZE    ((uint32_t)1524U)    /*!< ETH_HEADER + ETH_EXTRA + ETH_VLAN_TAG + ETH_MAX_ETH_PAYLOAD + ETH_CRC */
#define ETH_HEADER               ((uint32_t)14U)    /*!< 6 byte Dest addr, 6 byte Src addr, 2 byte length/type */
#define ETH_CRC                   ((uint32_t)4U)    /*!< Ethernet CRC */
#define ETH_EXTRA                 ((uint32_t)2U)    /*!< Extra bytes in some cases */
#define ETH_VLAN_TAG              ((uint32_t)4U)    /*!< optional 802.1q VLAN Tag */
#define ETH_MIN_ETH_PAYLOAD       ((uint32_t)46U)    /*!< Minimum Ethernet payload size */
#define ETH_MAX_ETH_PAYLOAD       ((uint32_t)1500U)    /*!< Maximum Ethernet payload size */
#define ETH_JUMBO_FRAME_PAYLOAD   ((uint32_t)9000U)    /*!< Jumbo frame payload size */

/* Definition of the Ethernet driver buffers size and count */
#define ETH_RX_BUF_SIZE                ETH_MAX_PACKET_SIZE  /* buffer size for receive               */
#define ETH_TX_BUF_SIZE                ETH_MAX_PACKET_SIZE  /* buffer size for transmit              */
#define ETH_RXBUFNB                    ((uint32_t)4U)       /* 4 Rx buffers of size ETH_RX_BUF_SIZE  */
#define ETH_TXBUFNB                    ((uint32_t)4U)       /* 4 Tx buffers of size ETH_TX_BUF_SIZE  */

#define ETH_BUF_LEN ((ETH_RX_BUF_SIZE * ETH_RXBUFNB) + (ETH_TX_BUF_SIZE * ETH_TXBUFNB))

static uint8_t _eth_framebuffer[2000];
static bool last_link;

static err_t _sys_ethmac_ll_tx(struct netif* netif, struct pbuf* p);
static struct pbuf* _sys_ethmac_ll_rx(struct netif* netif);

static void conf_io(uint32_t port, uint16_t pins)
{
    gpio_mode_setup(port, GPIO_MODE_AF, GPIO_PUPD_NONE, pins);

    /* AF11 for all of the ETH periphery (datasheet p. 76) */
    gpio_set_af(port, GPIO_AF11, pins);
}

void sys_ethmac_init(void)
{
    static uint8_t buffer[ETH_BUF_LEN] = {0};
    // const uint8_t mac_addr[] = {0, 0x80, 0xE1, 0, 0, 0};

    // rcc_periph_clock_enable(RCC_DMA1);
    // rcc_periph_clock_enable(RCC_DMA2);
    rcc_periph_clock_enable(RCC_ETHMAC);
    rcc_periph_clock_enable(RCC_ETHMACTX);
    rcc_periph_clock_enable(RCC_ETHMACRX);

    /* https://stackoverflow.com/questions/67080702/how-to-make-built-in-ethernet-work-on-stm32f407 */
    /* https://www.linux.org.ru/forum/development/16265373 */
    rcc_periph_clock_enable(RCC_SYSCFG);
    SYSCFG_PMC |= MII_RMII_SEL; // Select RMII


    // rcc_periph_reset_pulse(RCC_ETHMAC);

    rcc_periph_clock_enable(RCC_GPIOA);
    conf_io(GPIOA, RMII_CRS_DV | RMII_MDIO | RMII_REF_CLK);

    rcc_periph_clock_enable(RCC_GPIOC);
    conf_io(GPIOC, RMII_RXD0 | RMII_RXD1 | RMII_MDC);

    rcc_periph_clock_enable(RCC_GPIOG);
    conf_io(GPIOG, RMII_TX0EN | RMII_TXD0 | RMII_TXD1);

    /* PD5 (RMII_RXER) is not MAC function. Optional by LAN8742A datasheet p. 30. */
    gpio_mode_setup(GPIOG, GPIO_MODE_INPUT, GPIO_PUPD_NONE, RMII_RXER);

    // rcc_periph_reset_pulse(RCC_ETHMAC);

    // phy_init(0);

    /* MAC software reset (from HAL_ETH_Init) */
    ETH_DMABMR |= ETH_DMABMR_SR;

    while (ETH_DMABMR & ETH_DMABMR_SR);


    eth_init(0, ETH_CLK_150_168MHZ); /* For HCLK 150-168 MHz */

    // eth_set_mac(mac_addr);
    eth_desc_init(buffer, ETH_TXBUFNB, ETH_RXBUFNB,
        ETH_TX_BUF_SIZE, ETH_RX_BUF_SIZE, false);
    eth_start();
}

err_t sys_ethmac_netif_init(struct netif* netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    netif->hostname = "lwip";
#endif

    //NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100000000);

    netif->name[0] = 'e';
    netif->name[1] = 'n';
    netif->output = etharp_output;
    netif->linkoutput = _sys_ethmac_ll_tx;

    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    netif->hwaddr[0] = 0x00;
    netif->hwaddr[1] = 0x80;
    netif->hwaddr[2] = 0xFF;
    netif->hwaddr[3] = 0xFF;
    netif->hwaddr[4] = 0xFF;
    netif->hwaddr[5] = 0x01;
    eth_set_mac(netif->hwaddr);

    netif->mtu = 1500;

    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    return ERR_OK;
}

void sys_ethmac_poll(int phy, struct netif* netif)
{
    bool link_active;
    struct eth_hdr* ethhdr;
    struct pbuf* p;

    link_active = phy_link_isup(phy);

    if (link_active != last_link)
    {
        last_link = link_active;

        if (last_link)
        {
            netif_set_link_up(netif);
            // printf("[%09.3lf] en%d link changed to UP.\n", sys_get_tick() / (double)1000, phy);
        }
        else
        {
            netif_set_link_down(netif);
            // printf("[%09.3lf] en%d link changed to DOWN.\n", sys_get_tick() / (double)1000, phy);
        }
    }

    p = _sys_ethmac_ll_rx(netif);

    if (p == NULL)
    {
        return;
    }

    ethhdr = p->payload;

    switch (htons(ethhdr->type))
    {
        case ETHTYPE_IP:
        case ETHTYPE_ARP:
#if PPPOE_SUPPORT
        case ETHTYPE_PPPOEDISC:
        case ETHTYPE_PPPOE:
#endif
            if (netif->input(p, netif) != ERR_OK)
            {
                LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
                pbuf_free(p);
                p = NULL;
            }

            break;

        default:
            pbuf_free(p);
            p = NULL;
            break;
    }
}

static err_t _sys_ethmac_ll_tx(struct netif* netif, struct pbuf* p)
{
    uint32_t idx = 0;
    struct pbuf* cur;

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE);
#endif

    // Copy the complete frame into the framebuffer.
    for (cur = p; cur != NULL; cur = p->next)
    {
        memcpy(&_eth_framebuffer[idx], cur->payload, cur->len);
        idx += cur->len;
    }

    // Transmit the frame.
    eth_tx(_eth_framebuffer, idx);

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);
#endif

    return ERR_OK;
}

static struct pbuf* _sys_ethmac_ll_rx(struct netif* netif)
{
    struct pbuf* p, *cur;
    uint32_t idx = 0, packet_size = 0;

    // Copy a recieved packet into the framebuffer and retrieve it's size.
    if (!eth_rx(_eth_framebuffer, &packet_size, sizeof(_eth_framebuffer)))
    {
        return NULL;
    }

#if ETH_PAD_SIZE
    packet_size += ETH_PAD_SIZE;
#endif

    p = pbuf_alloc(PBUF_RAW, packet_size, PBUF_POOL);

    if (p != NULL)
    {
#if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE);
#endif

        for (cur = p; cur != NULL; cur = cur->next)
        {
            memcpy(cur->payload, &_eth_framebuffer[idx], cur->len);
            idx += cur->len;
        }

#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE);
#endif
    }

    return p;
}
