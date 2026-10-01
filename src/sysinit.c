#include "tx_api.h"
#include "ip_thread.h"
#include "heater.h"

#define DEMO_STACK_SIZE         4096
#define DEMO_BYTE_POOL_SIZE     9120
TX_THREAD               thread_0;
TX_BYTE_POOL            byte_pool_0;
UCHAR                   memory_area[DEMO_BYTE_POOL_SIZE];

TX_THREAD heater_thread_ctl;
char heater_thread_stack[512];

void tx_application_define(void* first_unused_memory)
{
    CHAR*    pointer = TX_NULL;

    /* Create a byte memory pool from which to allocate the thread stacks.  */
    tx_byte_pool_create(&byte_pool_0, "byte pool 0", memory_area, DEMO_BYTE_POOL_SIZE);

    /* Put system definition stuff in here, e.g. thread creates and other assorted
       create information.  */

    /* Allocate the stack for thread 0.  */
    tx_byte_allocate(&byte_pool_0, (VOID**) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create the main thread.  */
    tx_thread_create(&thread_0, "thread 0", ip_thread, 0,
        pointer, DEMO_STACK_SIZE,
        1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Create heather thread */
    tx_thread_create(
        &heater_thread_ctl,          /* TX_THREAD* thread_ptr        Thread control block pointer  */
        "heater",                    /* CHAR*      name              Pointer to thread name string */
        heater_thread,               /* VOID (*)() entry_function    Entry function of the thread  */
        0,                           /* ULONG      entry_input       32-bit input value to thread  */
        (void*)heater_thread_stack,  /* VOID*      stack_start       Pointer to start of stack     */
        sizeof(heater_thread_stack), /* ULONG      stack_size        Stack size in bytes           */
        1,                           /* UINT       priority          Priority of thread (0-31)     */
        1,                           /* UINT       preempt_threshold Preemption threshold          */
        TX_NO_TIME_SLICE,            /* ULONG      time_slice        Thread time-slice value       */
        TX_AUTO_START                /* UINT       auto_start        Automatic start selection     */
    );
}
