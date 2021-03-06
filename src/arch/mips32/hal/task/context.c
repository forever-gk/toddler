#include "common/include/data.h"
#include "common/include/memlayout.h"
#include "common/include/memory.h"
#include "common/include/context.h"
#include "common/include/proc.h"
#include "hal/include/print.h"
#include "hal/include/mem.h"
#include "hal/include/lib.h"
#include "hal/include/cpu.h"
#include "hal/include/int.h"
#include "hal/include/task.h"


dec_per_cpu(ulong, cur_running_sched_id);

dec_per_cpu(int, cur_in_user_mode);
dec_per_cpu(struct saved_context, cur_context);

dec_per_cpu(ulong, cur_tcb_vaddr);


/*
 * Initialize context for a newly created thread
 */
void init_thread_context(struct context *context, ulong entry, ulong param, ulong stack_top, int user_mode)
{
    // Set GPRs
    memzero(context, sizeof(struct context));
    
    // Set param
    context->a0 = param;
    
    // Set other states
    context->sp = stack_top;
    context->pc = entry;
    context->delay_slot = 0;
}

void set_thread_context_param(struct context *context, ulong param)
{
    context->a0 = param;
}


/*
 * Save context for interrupt handling
 */
u32 asmlinkage save_context(struct context *context)
{
    // Set local interrupt state to disabled
    disable_local_int();
    
    // Save PC and branch delay slot state
    u32 cause = 0;
    __asm__ __volatile__ (
        "mfc0   %0, $14;"
        "mfc0   %1, $13;"
        : "=r" (context->pc), "=r" (cause)
        :
    );
    context->delay_slot = cause & 0x80000000;
}


/*
 * Context switch
 */
static void no_opt switch_page_dir(ulong page_dir_pfn)
{
    struct page_frame **page = get_per_cpu(struct page_frame *, cur_page_dir);
    *page = (struct page_frame *)PFN_TO_ADDR(page_dir_pfn);
}

static void no_opt switch_to(struct context *context, int user_mode, ulong asid)
{
    // Set EPC to PC
    __asm__ __volatile__ (
        "mtc0   %0, $14;"
        :
        : "r" (context->pc)
    );
    
    // Set CAUSE based on delay slot state
    u32 cause = 0;
    __asm__ __volatile__ (
        "mfc0   %0, $13;"
        : "=r" (cause)
        :
    );
    cause |= context->delay_slot;
    __asm__ __volatile__ (
        "mtc0   %0, $13;"
        :
        : "r" (cause)
    );
    
    // Set SR based on user/kernel mode
    // Also set EXL bit - switching is enabled
    u32 sr = 0;
    __asm__ __volatile__ (
        "mfc0   %0, $12;"
        : "=r" (sr)
        :
    );
    sr &= ~0x18;    // kernel
    if (user_mode) {
        sr |= 0x10; // user
    }
    sr |= 0x2;      // EXL
    __asm__ __volatile__ (
        "mtc0   %0, $12;"
        :
        : "r" (sr)
    );
    
    // Set ASID in TLB EntryHi
    u32 hi = 0;
    __asm__ __volatile__ (
        "mfc0   %0, $10;"
        : "=r" (hi)
        :
    );
    hi &= ~0xff;
    hi |= asid & 0xff;
    __asm__ __volatile__ (
        "mtc0   %0, $10;"
        :
        : "r" (hi)
    );
    
    // Set local interrupt state to enabled
    enable_local_int();
    
    // Restore GPRs
    restore_context_gpr();
}

void no_opt switch_context(ulong sched_id, struct context *context,
                                      ulong page_dir_pfn, int user_mode, ulong asid, ulong tcb)
{
//     kprintf("To switch context, PC: %x, SP: %x, ASID: %x, user: %d\n", context->pc, context->sp, asid, user_mode);
    
    // Copy the context to local, thus prevent TLB miss
    struct saved_context *per_cpu_context = get_per_cpu(struct saved_context, cur_context);
    memcpy(&per_cpu_context->context, context, sizeof(struct context));
    
    // Set sched id
    *get_per_cpu(ulong, cur_running_sched_id) = sched_id;
    *get_per_cpu(int, cur_in_user_mode) = user_mode;
    
//     // Set TCB
//     struct thread_control_block *cur_tcb = (struct thread_control_block *)get_my_cpu_tcb_start_vaddr();
//     cur_tcb->msg_send = tcb->msg_send;
//     cur_tcb->msg_recv = tcb->msg_recv;
//     cur_tcb->tls = tcb->tls;
//     cur_tcb->proc_id = tcb->proc_id;
//     cur_tcb->thread_id = tcb->thread_id;
    
    // Make k1 ponit to TCB
    __asm__ __volatile__ (
        "move   $27, %0;"
        :
        : "r" (tcb)
    );
    
    *(ulong *)get_per_cpu(ulong, cur_tcb_vaddr) = tcb;
    
    // Switch page dir
    switch_page_dir(page_dir_pfn);
    
    // Do the actual switch
    switch_to(context, user_mode, asid);
}

void init_context_mp()
{
    ulong *sched_id = get_per_cpu(ulong, cur_running_sched_id);
    *sched_id = 0;
    
    int *user_mode = get_per_cpu(int, cur_in_user_mode);
    *user_mode = 0;
    
    ulong *cur_tcb = get_per_cpu(ulong, cur_tcb_vaddr);
    *cur_tcb = 0;
}

void init_context()
{
    ulong *sched_id = get_per_cpu(ulong, cur_running_sched_id);
    *sched_id = 0;
    
    int *user_mode = get_per_cpu(int, cur_in_user_mode);
    *user_mode = 0;
    
    ulong *cur_tcb = get_per_cpu(ulong, cur_tcb_vaddr);
    *cur_tcb = 0;
}
