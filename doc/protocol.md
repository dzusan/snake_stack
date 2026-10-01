**XRD experiment cell device user manual**

Document release: 1 (05.02.2023)

Protocol version: 1

&nbsp;
&nbsp;
&nbsp;

**TABLE OF CONTENTS**

* [Specification](#specification)
* [Timestamp format for all the packets](#timestamp-format-for-all-the-packets)
* [Real time control (PC -\> device)](#real-time-control-pc---device)
* [State monitoring (device -\> PC)](#state-monitoring-device---pc)
* [Planned features](#planned-features)

## Specification

* Static IP address: 192.168.1.100 (no DHCP client yet)
* Default IP gateway address 192.168.1.254
* Destination IP address: 192.168.1.10 (unless another specified)
* Data endianness is big-endian unless explicitly noticed about little-endian

## Timestamp format for all the packets

* 64-bit NTP timestamp format is used in packet headers (RFC 5905, line 711)
* Big-endian (RFC 5905, line 644)
* Actual fraction accuracy is 15 bit (source clock is 32768 Hz)

| Bytes | Description        |
|-------|--------------------|
| 0-3   | Integral (seconds) |
| 4-7   | Fractional         |

**Data structure implementation on C:**

```C
struct ntp_ts
{
    uint32_t integral;
    uint32_t frac;
};
```

## Real time control (PC -> device)

To implement variable parameter profile in real time, the following protocol must be used.

**Properties:**

* It uses UDP as transport
* Default address 192.168.1.100 and port 50000 (configurable)
* No response

**Packet data:**

| Bytes  | Description                                   |
|--------|-----------------------------------------------|
| 0-7    | 64-bit NTP timestamp (RFC 5905, line 711)     |
| 8      | Protocol version (current: 1)                 |
| 9-12   | Magic number for authorization (security)     |
| 13-16  | Transversal spread in nanometers              |
| 17-20  | Longitudinal spread in nanometers             |
| 21-24  | Temperature in milli-degrees Celsius (signed) |
| 25-28  | Fluid flow in ml/min                          |
| 29-32  | Reserved field                                |
| 33     | State control flags                           |
| 34-37  | Rolling counter overall packets sent          |
| 38-41  | Packet CRC32                                  |

**Flags (possible in state control flags):**

| Bit | Description                                      |
|-----|--------------------------------------------------|
| 0   | Ignore timestamp, implement settings immediately |

**Data structure implementation on C:**

```C
#define CTL_FLAG_APPLY_NOW (1<<0) /* Ignore timestamp, apply settings immediately */

struct ctl_packet
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
};
```

The device clock can be synchronized with World or PC. Control messages will be collected in queue and physically applied only in time specified in packet timestamp. But it can be applied immediately if the CTL_FLAG_APPLY_NOW flag is set, then the entire queue will be cleared.

Access token will be ignored in the current version. Rolling counter must be incremented with each subsequent packet. If the CRC does not match, then the packet will be dropped.

## State monitoring (device -> PC)

**Properties:**

* It uses UDP as transport
* Default destination address 192.168.1.10 and port 50001 (configurable)
* Sent periodically every 100 milliseconds (configurable) 
* No need response

**Packet data:**

| Bytes  | Description                                   |
|--------|-----------------------------------------------|
| 0-7    | 64-bit NTP timestamp (RFC 5905, line 711)     |
| 8      | Protocol version (current: 1)                 |
| 9-12   | Transversal spread in nanometers              |
| 13-16  | Longitudinal spread in nanometers             |
| 17-20  | Temperature in milli-degrees Celsius (signed) |
| 21-24  | Fluid flow in ml/min                          |
| 25-28  | Reserved field                                |
| 29     | State control flags                           |
| 30-33  | Rolling counter overall packets sent          |
| 34-37  | Packet CRC32                                  |

**Flags (possible in state control flags):**

| Bit | Description                                                        |
|-----|--------------------------------------------------------------------|
| 0   | The transient process to achieve subsequent settigs is now running |
| 1   | The last received control request is not possible                  |
| 2   | Packet loss between last received packet and previous to last      |
| 3   | The last received packet was corrupted and dropped                 |   
| 4   | Control packet queue full, next packets will be dropped            |
| 5   | Electrical circuit break between ECU and mechanics                 |
| 6   | Critical error, stop operation                                     |

The packet contains data directly measured from feedback sensors. The timestamp field reflects the actual measurement time. The rolling counter is incremented with each subsequent packet.

**Data structure implementation on C:**

```C
#define MON_FLAG_IN_PROC     (1<<0) /* in process */
#define MON_FLAG_NPOS        (1<<1) /* last received control request is not possible */
#define MON_FLAG_LOSS        (1<<2) /* packet loss between last received packet and previous to last */
#define MON_FLAG_CORRUPT     (1<<3) /* last received packet was corrupted and dropped */
#define MON_FLAG_FULL        (1<<4) /* queue full */
#define MON_FLAG_CIRC_BREAK  (1<<5) /* electrical circuit break between ECU and mechanics */
#define MON_FLAG_CRIT        (1<<6) /* critical error, stop operation */

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
};
```

## Planned features

* Single page web interface on port 80
* Firmware upgrade by HTTP
* Configuration by YAML file

<!-- ----------------------------------------------

Generate PDF:

`pandoc doc/protocol.md --pdf-engine=xelatex -o bin/protocol_r1.pdf --mathjax` -->

