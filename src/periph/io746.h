#ifndef __IO746_H__
#define __IO746_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RMII_TX0EN    GPIO11 /* PG11 af-out **/
#define RMII_TXD0     GPIO13 /* PG13 af-out **/
#define RMII_TXD1     GPIO14 /* PG14 af-out **/

#define RMII_RXD0     GPIO4  /* PC4 af-in **/
#define RMII_RXD1     GPIO5  /* PC5 af-in **/

#define RMII_RXER     GPIO2  /* PG2 in (optional) */

#define RMII_CRS_DV   GPIO7  /* PA7 af-in **/

#define RMII_MDC      GPIO1  /* PC1 af-out **/
#define RMII_MDIO     GPIO2  /* PA2 af-io **/

#define RMII_REF_CLK  GPIO1  /* PA1 af-in **/

#define LED_PORT      GPIOI  /* PI1 out */
#define LED_PIN       GPIO1  /* PI1 out */

#ifdef __cplusplus
}
#endif
#endif /* __IO746_H__ */
