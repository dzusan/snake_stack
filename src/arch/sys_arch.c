/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "lwip/tcpip.h"

#include "tx_api.h"

/* SysTick is set up for 1 kHz in tx_initialize_low_level.S (tx_user.h is not used,
 * so TX_TIMER_TICKS_PER_SECOND from tx_api.h does not reflect the real rate) */
#ifndef SYS_ARCH_TICK_HZ
#define SYS_ARCH_TICK_HZ        1000U
#endif

/* Memory for thread stacks and mailbox storage */
#ifndef SYS_ARCH_POOL_SIZE
#define SYS_ARCH_POOL_SIZE      (16 * 1024)
#endif

/* Mailbox size used when lwIP asks for a mailbox of size 0 */
#ifndef SYS_MBOX_DEFAULT_SIZE
#define SYS_MBOX_DEFAULT_SIZE   16
#endif

/* Mailbox messages are pointers carried in a single ULONG */
typedef char sys_arch_pointer_fits_in_ulong[(sizeof(void*) <= sizeof(ULONG)) ? 1 : -1];

static TX_BYTE_POOL sys_arch_pool;
static UCHAR sys_arch_pool_area[SYS_ARCH_POOL_SIZE] __attribute__((aligned(8)));

struct sys_arch_thread {
  TX_THREAD      thread;
  lwip_thread_fn function;
  void*          arg;
};

/* Read the active exception number (0 when running in thread mode) */
static inline uint32_t __get_IPSR(void)
{
  uint32_t result;
  __asm volatile ("MRS %0, ipsr" : "=r" (result));
  return result;
}

static ULONG
sys_arch_ms_to_ticks(u32_t ms)
{
  ULONG ticks = (ULONG)(((uint64_t)ms * SYS_ARCH_TICK_HZ + 999U) / 1000U);
  return (ms != 0 && ticks == 0) ? 1 : ticks;
}

static u32_t
sys_arch_ticks_to_ms(ULONG ticks)
{
  return (u32_t)(((uint64_t)ticks * 1000U) / SYS_ARCH_TICK_HZ);
}

/* lwIP timeout of 0 means "wait forever" */
static ULONG
sys_arch_wait_option(u32_t timeout_ms)
{
  return (timeout_ms == 0) ? TX_WAIT_FOREVER : sys_arch_ms_to_ticks(timeout_ms);
}

void
sys_init(void)
{
  static int initialized;

  if (!initialized)
  {
    UINT s = tx_byte_pool_create(&sys_arch_pool, "lwip sys_arch", sys_arch_pool_area, sizeof(sys_arch_pool_area));
    LWIP_ASSERT("failed to create sys_arch byte pool", s == TX_SUCCESS);
    LWIP_UNUSED_ARG(s);
    initialized = 1;
  }
}

#if SYS_LIGHTWEIGHT_PROT
sys_prot_t
sys_arch_protect(void)
{
  return tx_interrupt_control(TX_INT_DISABLE);
}

void
sys_arch_unprotect(sys_prot_t pval)
{
  tx_interrupt_control(pval);
}
#endif /* SYS_LIGHTWEIGHT_PROT */

uint32_t sys_now(void)
{
    return sys_arch_ticks_to_ms(tx_time_get());
}

void
sys_arch_msleep(u32_t delay_ms)
{
  tx_thread_sleep(sys_arch_ms_to_ticks(delay_ms));
}

err_t
sys_mutex_new(sys_mutex_t *mutex)
{
  LWIP_ASSERT("mutex != NULL", mutex != NULL);

  UINT s = tx_mutex_create(mutex, "lwip mutex", TX_NO_INHERIT);

  if(s != TX_SUCCESS)
  {
    sys_mutex_set_invalid(mutex);
    SYS_STATS_INC(mutex.err);
    return ERR_MEM;
  }

  SYS_STATS_INC_USED(mutex);
  return ERR_OK;
}

void
sys_mutex_lock(sys_mutex_t *mutex)
{
  LWIP_ASSERT("mutex is valid", sys_mutex_valid(mutex));

  UINT s = tx_mutex_get(mutex, TX_WAIT_FOREVER);

  LWIP_ASSERT("failed to take the mutex", s == TX_SUCCESS);
  LWIP_UNUSED_ARG(s);
}

void
sys_mutex_unlock(sys_mutex_t *mutex)
{
  LWIP_ASSERT("mutex is valid", sys_mutex_valid(mutex));

  UINT s = tx_mutex_put(mutex);

  LWIP_ASSERT("failed to give the mutex", s == TX_SUCCESS);
  LWIP_UNUSED_ARG(s);
}

void
sys_mutex_free(sys_mutex_t *mutex)
{
  LWIP_ASSERT("mutex is valid", sys_mutex_valid(mutex));

  SYS_STATS_DEC(mutex.used);
  tx_mutex_delete(mutex);
  sys_mutex_set_invalid(mutex);
}

err_t
sys_sem_new(sys_sem_t *sem, u8_t count)
{
  LWIP_ASSERT("sem != NULL", sem != NULL);

  UINT s = tx_semaphore_create(sem, "lwip sem", count);

  if(s != TX_SUCCESS)
  {
    sys_sem_set_invalid(sem);
    SYS_STATS_INC(sem.err);
    return ERR_MEM;
  }

  SYS_STATS_INC_USED(sem);
  return ERR_OK;
}

void
sys_sem_signal(sys_sem_t *sem)
{
  LWIP_ASSERT("sem is valid", sys_sem_valid(sem));

  UINT s = tx_semaphore_put(sem);

  LWIP_ASSERT("failed to signal the semaphore", s == TX_SUCCESS);
  LWIP_UNUSED_ARG(s);
}

u32_t
sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
  LWIP_ASSERT("sem is valid", sys_sem_valid(sem));

  ULONG start = tx_time_get();

  if (tx_semaphore_get(sem, sys_arch_wait_option(timeout)) != TX_SUCCESS)
  {
    return SYS_ARCH_TIMEOUT;
  }

  return sys_arch_ticks_to_ms(tx_time_get() - start);
}

void
sys_sem_free(sys_sem_t *sem)
{
  LWIP_ASSERT("sem is valid", sys_sem_valid(sem));

  SYS_STATS_DEC(sem.used);
  tx_semaphore_delete(sem);
  sys_sem_set_invalid(sem);
}

err_t
sys_mbox_new(sys_mbox_t *mbox, int size)
{
  LWIP_ASSERT("mbox != NULL", mbox != NULL);

  if (size <= 0)
  {
    size = SYS_MBOX_DEFAULT_SIZE;
  }

  ULONG bytes = (ULONG)size * sizeof(ULONG);

  if (tx_byte_allocate(&sys_arch_pool, &mbox->storage, bytes, TX_NO_WAIT) != TX_SUCCESS)
  {
    sys_mbox_set_invalid(mbox);
    SYS_STATS_INC(mbox.err);
    return ERR_MEM;
  }

  if (tx_queue_create(&mbox->queue, "lwip mbox", TX_1_ULONG, mbox->storage, bytes) != TX_SUCCESS)
  {
    tx_byte_release(mbox->storage);
    sys_mbox_set_invalid(mbox);
    SYS_STATS_INC(mbox.err);
    return ERR_MEM;
  }

  SYS_STATS_INC_USED(mbox);
  return ERR_OK;
}

void
sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
  LWIP_ASSERT("mbox is valid", sys_mbox_valid(mbox));

  ULONG message = (ULONG)msg;
  UINT s = tx_queue_send(&mbox->queue, &message, TX_WAIT_FOREVER);

  LWIP_ASSERT("failed to post to the mbox", s == TX_SUCCESS);
  LWIP_UNUSED_ARG(s);
}

err_t
sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
  LWIP_ASSERT("mbox is valid", sys_mbox_valid(mbox));

  ULONG message = (ULONG)msg;

  if (tx_queue_send(&mbox->queue, &message, TX_NO_WAIT) != TX_SUCCESS)
  {
    SYS_STATS_INC(mbox.err);
    return ERR_MEM;
  }

  return ERR_OK;
}

err_t
sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
  /* ThreadX services with TX_NO_WAIT are ISR safe and reschedule on ISR exit */
  return sys_mbox_trypost(mbox, msg);
}

u32_t
sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
  LWIP_ASSERT("mbox is valid", sys_mbox_valid(mbox));

  ULONG message;
  ULONG start = tx_time_get();

  if (tx_queue_receive(&mbox->queue, &message, sys_arch_wait_option(timeout)) != TX_SUCCESS)
  {
    if (msg != NULL)
    {
      *msg = NULL;
    }
    return SYS_ARCH_TIMEOUT;
  }

  if (msg != NULL)
  {
    *msg = (void*)message;
  }

  return sys_arch_ticks_to_ms(tx_time_get() - start);
}

u32_t
sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
  LWIP_ASSERT("mbox is valid", sys_mbox_valid(mbox));

  ULONG message;

  if (tx_queue_receive(&mbox->queue, &message, TX_NO_WAIT) != TX_SUCCESS)
  {
    return SYS_MBOX_EMPTY;
  }

  if (msg != NULL)
  {
    *msg = (void*)message;
  }

  return 0;
}

void
sys_mbox_free(sys_mbox_t *mbox)
{
  LWIP_ASSERT("mbox is valid", sys_mbox_valid(mbox));

  ULONG enqueued = 0;
  tx_queue_info_get(&mbox->queue, TX_NULL, &enqueued, TX_NULL, TX_NULL, TX_NULL, TX_NULL);
  /* Messages left in a freed mbox are a programming error in lwIP terms */
  LWIP_ASSERT("mbox not empty on free", enqueued == 0);

  SYS_STATS_DEC(mbox.used);
  tx_queue_delete(&mbox->queue);
  tx_byte_release(mbox->storage);
  mbox->storage = NULL;
  sys_mbox_set_invalid(mbox);
}

/* ThreadX entry takes a ULONG, lwIP threads take a void* */
static void
sys_arch_thread_entry(ULONG input)
{
  struct sys_arch_thread *t = (struct sys_arch_thread*)input;
  t->function(t->arg);
  /* lwIP threads are not expected to return; their memory is not reclaimed */
}

sys_thread_t
sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
  struct sys_arch_thread *t = NULL;
  VOID *stack = NULL;

  if (stacksize <= 0)
  {
    stacksize = (DEFAULT_THREAD_STACKSIZE > 0) ? DEFAULT_THREAD_STACKSIZE : 1024;
  }
  if (stacksize < TX_MINIMUM_STACK)
  {
    stacksize = TX_MINIMUM_STACK;
  }
  LWIP_ASSERT("invalid thread priority", prio >= 0 && prio < TX_MAX_PRIORITIES);

  if (tx_byte_allocate(&sys_arch_pool, (VOID**)&t, sizeof(*t), TX_NO_WAIT) != TX_SUCCESS)
  {
    LWIP_ASSERT("no memory for thread control block", 0);
    return NULL;
  }
  if (tx_byte_allocate(&sys_arch_pool, &stack, (ULONG)stacksize, TX_NO_WAIT) != TX_SUCCESS)
  {
    tx_byte_release(t);
    LWIP_ASSERT("no memory for thread stack", 0);
    return NULL;
  }

  t->function = thread;
  t->arg = arg;

  /* ThreadX: lower number is higher priority */
  UINT s = tx_thread_create(&t->thread, (CHAR*)name, sys_arch_thread_entry, (ULONG)t,
                            stack, (ULONG)stacksize, (UINT)prio, (UINT)prio,
                            TX_NO_TIME_SLICE, TX_AUTO_START);
  if (s != TX_SUCCESS)
  {
    tx_byte_release(stack);
    tx_byte_release(t);
    LWIP_ASSERT("failed to create thread", 0);
    return NULL;
  }

  return &t->thread;
}

#if LWIP_TCPIP_CORE_LOCKING
/** Flag the core lock held. A counter for recursive locks. */
static u8_t lwip_core_lock_count;
static TX_THREAD* lwip_core_lock_holder_thread;

void
sys_lock_tcpip_core(void)
{
   sys_mutex_lock(&lock_tcpip_core);
   if (lwip_core_lock_count == 0) {
     lwip_core_lock_holder_thread = tx_thread_identify();
   }
   lwip_core_lock_count++;
}

void
sys_unlock_tcpip_core(void)
{
   lwip_core_lock_count--;
   if (lwip_core_lock_count == 0) {
       lwip_core_lock_holder_thread = 0;
   }
   sys_mutex_unlock(&lock_tcpip_core);
}
#endif /* LWIP_TCPIP_CORE_LOCKING */

#if !NO_SYS
static TX_THREAD* lwip_tcpip_thread;
#endif

void
sys_mark_tcpip_thread(void)
{
#if !NO_SYS
  lwip_tcpip_thread = tx_thread_identify();
#endif
}

void
sys_check_core_locking(void)
{
  /* Embedded systems should check we are NOT in an interrupt context here */
  /* E.g. core Cortex-M3/M4 ports:
         configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 );
  */

  if (__get_IPSR() != 0)
  {
    /* This means here is interrupt context */
    for(;;)
    {
      /* Catch */
    }
  }

#if !NO_SYS
  if (lwip_tcpip_thread != 0) {
    TX_THREAD* current_thread = tx_thread_identify();

#if LWIP_TCPIP_CORE_LOCKING
    LWIP_ASSERT("Function called without core lock",
                current_thread == lwip_core_lock_holder_thread && lwip_core_lock_count > 0);
#else /* LWIP_TCPIP_CORE_LOCKING */
    LWIP_ASSERT("Function called from wrong thread", current_thread == lwip_tcpip_thread);
#endif /* LWIP_TCPIP_CORE_LOCKING */
  }
#endif /* !NO_SYS */
}

