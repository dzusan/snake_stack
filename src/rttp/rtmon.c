#include <string.h>

#include "tx_api.h"

#include <lwip/opt.h>
#include <lwip/debug.h>
#include <lwip/stats.h>
#include <lwip/udp.h>

#include "time/ntpdef.h"
#include "rtmon.h"
#include "clock.h"
#include "rtctlsrv.h"
#include "rtutil.h"

/* State flags */
#define MON_FLAG_IN_PROC     (1<<0) /* in process */
#define MON_FLAG_NPOS        (1<<1) /* last received control request is not possible */
#define MON_FLAG_LOSS        (1<<2) /* packet loss between last received packet and previous to last */
#define MON_FLAG_CORRUPT     (1<<3) /* last received packet was corrupted and dropped */
#define MON_FLAG_FULL        (1<<4) /* queue full */
#define MON_FLAG_CIRC_BREAK  (1<<5) /* electrical circuit break between ECU and mechanics */
#define MON_FLAG_CRIT        (1<<6) /* critical error, stop operation */

#pragma pack(push, 1)
struct mon_packet
{
    struct ntp_ts timestamp; /* 64-bit NTP timestamp (RFC 5905, line 711) */
    uint8_t  version;        /* Protocol version (current: 1) */
    uint32_t x_nm;           /* Transversal spread in nanometers */
    uint32_t y_nm;           /* Longitudinal spread in nanometers */
    int32_t  temp_mdegc;     /* Temperature in milli-degrees Celsius (signed) */
    uint32_t flow_ml_min;    /* Fluid flow in ml/min */
    uint32_t reserved;       /* Reserved field */
    uint8_t  flags;          /* State flags */
    uint32_t rollcnt;        /* Rolling counter overall packets sent */
    uint32_t crc32;          /* Packet CRC32 */
} mon = {.version = 1};
#pragma pack(pop)

static struct udp_pcb* pcb;
static ip_addr_t send_addr;

void rtmon_proc(void)
{
    static uint64_t curtick = 0;
    static uint32_t rollcnt = 0;
    struct pbuf* sbuf;
    uint32_t crc;

    if (pcb != NULL)
    {
        if (tx_time_get() - curtick >= 100)
        {
            curtick = tx_time_get();
            mon.y_nm = lwip_htonl(rtctlsrv_get_y());
            mon.temp_mdegc = lwip_htonl(rtctlsrv_get_temp());
            rollcnt++;
            mon.rollcnt = lwip_htonl(rollcnt);
            crc = _crc32(0, (uint8_t*)&mon, sizeof(struct mon_packet) - sizeof(uint32_t));
            mon.crc32 = lwip_htonl(crc);
            sbuf = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct mon_packet), PBUF_POOL);
            memcpy(sbuf->payload, (void*)&mon, sizeof(struct mon_packet));
            udp_sendto(pcb, sbuf, &send_addr, 50001);
            pbuf_free(sbuf);
        }
    }
}

void rtmon_init(void)
{
    pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    ip4addr_aton("192.168.1.10", &send_addr);
}
