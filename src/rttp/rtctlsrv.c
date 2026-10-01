#include <string.h>

#include <lwip/opt.h>
#include <lwip/debug.h>
#include <lwip/stats.h>
#include <lwip/udp.h>
#include <lwip/def.h>

#include "time/ntpdef.h"
#include "rtctlsrv.h"
#include "rtutil.h"

#define CTL_FLAG_APPLY_NOW (1<<0) /* Ignore timestamp, apply settings immediately */

#pragma pack(push, 1)
struct ctl_packet /* Big endian ! */
{
    struct ntp_ts timestamp; /* 64-bit NTP timestamp (RFC 5905, line 711) */
    uint8_t  version;        /* Protocol version (current: 1) */
    uint32_t access_token;   /* Magic number for authorization (security) */
    uint32_t x_nm;           /* Transversal spread in nanometers */
    uint32_t y_nm;           /* Longitudinal spread in nanometers */
    int32_t  temp_mdegc;     /* Temperature in milli-degrees Celsius (signed) */
    uint32_t flow_ml_min;    /* Fluid flow in ml/min */
    uint32_t reserved;       /* Reserved field */
    uint8_t  flags;          /* State control flags */
    uint32_t rollcnt;        /* Rolling counter overall packets sent */
    uint32_t crc32;          /* Packet CRC32 */
} cmd = {0};
#pragma pack(pop)

static void recv_cb(void* arg, struct udp_pcb* upcb, struct pbuf* p,
    const ip_addr_t* addr, u16_t port)
{
    LWIP_UNUSED_ARG(arg);
    uint32_t crc;
    struct ctl_packet* pctl;

    if (p != NULL)
    {
        if (p->len == sizeof(struct ctl_packet))
        {
            pctl = p->payload;

            if (pctl->version == 1)
            {
                crc = _crc32(0, (uint8_t*)pctl, sizeof(struct ctl_packet) - sizeof(uint32_t));
                crc = lwip_htonl(crc);

                if (crc == pctl->crc32)
                {
                    memcpy((void*)&cmd, p->payload, sizeof(struct ctl_packet));
                }
            }
        }

        pbuf_free(p);
    }
}

void rtctlsrv_init(void)
{
    static struct udp_pcb* buf;
    buf = udp_new_ip_type(IPADDR_TYPE_ANY);

    if (buf != NULL)
    {
        err_t err = udp_bind(buf, IP_ANY_TYPE, 50000);

        if (err == ERR_OK)
        {
            udp_recv(buf, recv_cb, NULL);
        }
    }
}

uint32_t rtctlsrv_get_y(void)
{
    return lwip_ntohl(cmd.y_nm);
}

int32_t rtctlsrv_get_temp(void)
{
    return lwip_ntohl(cmd.temp_mdegc);
}
