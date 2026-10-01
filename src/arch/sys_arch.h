#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "lwip/opt.h"
#include "lwip/arch.h"

#include "tx_api.h"

typedef TX_SEMAPHORE sys_sem_t;
typedef TX_MUTEX     sys_mutex_t;
/* ThreadX identifies a thread by the address of its control block */
typedef TX_THREAD*   sys_thread_t;

/* Mailbox is a queue of pointer-sized messages; storage comes from the sys_arch byte pool */
typedef struct {
  TX_QUEUE queue;
  VOID*    storage;
} sys_mbox_t;

/** This is returned by _fromisr() sys functions to tell the outermost function
 * that a higher priority task was woken and the scheduler needs to be invoked.
 */
#define ERR_NEED_SCHED 123

void sys_arch_msleep(u32_t delay_ms);
#define sys_msleep(ms) sys_arch_msleep(ms)

#define sys_sem_valid_val(sem)       ((sem).tx_semaphore_id != TX_CLEAR_ID)
#define sys_sem_valid(sem)           (((sem) != NULL) && sys_sem_valid_val(*(sem)))
#define sys_sem_set_invalid(sem)     ((sem)->tx_semaphore_id = TX_CLEAR_ID)

#define sys_mbox_valid_val(mbox)     ((mbox).queue.tx_queue_id != TX_CLEAR_ID)
#define sys_mbox_valid(mbox)         (((mbox) != NULL) && sys_mbox_valid_val(*(mbox)))
#define sys_mbox_set_invalid(mbox)   ((mbox)->queue.tx_queue_id = TX_CLEAR_ID)

#if SYS_LIGHTWEIGHT_PROT
typedef u32_t sys_prot_t;
#endif /* SYS_LIGHTWEIGHT_PROT */

#if !LWIP_COMPAT_MUTEX
#define sys_mutex_valid_val(mutex)   ((mutex).tx_mutex_id != TX_CLEAR_ID)
#define sys_mutex_valid(mutex)       (((mutex) != NULL) && sys_mutex_valid_val(*(mutex)))
#define sys_mutex_set_invalid(mutex) ((mutex)->tx_mutex_id = TX_CLEAR_ID)
#endif /* !LWIP_COMPAT_MUTEX */

#if LWIP_NETCONN_SEM_PER_THREAD
sys_sem_t* sys_arch_netconn_sem_get(void);
void sys_arch_netconn_sem_alloc(void);
void sys_arch_netconn_sem_free(void);
#define LWIP_NETCONN_THREAD_SEM_GET()   sys_arch_netconn_sem_get()
#define LWIP_NETCONN_THREAD_SEM_ALLOC() sys_arch_netconn_sem_alloc()
#define LWIP_NETCONN_THREAD_SEM_FREE()  sys_arch_netconn_sem_free()
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

#endif /* LWIP_ARCH_SYS_ARCH_H */
