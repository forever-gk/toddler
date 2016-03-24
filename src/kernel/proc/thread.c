/*
 * Thread manager
 */


#include "common/include/data.h"
#include "common/include/memory.h"
#include "kernel/include/hal.h"
#include "kernel/include/mem.h"
#include "kernel/include/proc.h"


static int thread_salloc_id;


static ulong gen_thread_id(struct thread *t)
{
    ulong id = (ulong)t;
    return id;
}


struct thread *create_thread(
    struct process *p, ulong entry_point, ulong param,
    int pin_cpu_id,
    ulong stack_size, ulong tls_size
)
{
    // Allocate a thread struct
    struct thread *t = (struct thread *)salloc(thread_salloc_id);
    assert(t);
    
    // Assign a thread id
    t->thread_id = gen_thread_id(t);
    
    // Setup the thread
    t->proc_id = p->proc_id;
    t->proc = p;
    t->state = thread_enter;
    
    // Thread memory
    if (p->type == process_kernel) {
        t->memory.thread_block_base = PFN_TO_ADDR(palloc(4));
        
        t->memory.msg_send_offset = 0;
        t->memory.msg_recv_offset = PAGE_SIZE;
        
        t->memory.tls_start_offset = PAGE_SIZE * 2;
        
        t->memory.stack_limit_offset = PAGE_SIZE * 3;
        t->memory.stack_top_offset = PAGE_SIZE * 4;
    } else {
        // Round up stack size and tls size
        if (stack_size % PAGE_SIZE) {
            stack_size /= PAGE_SIZE;
            stack_size++;
            stack_size *= PAGE_SIZE;
        }
        
        if (tls_size % PAGE_SIZE) {
            tls_size /= PAGE_SIZE;
            tls_size++;
            tls_size *= PAGE_SIZE;
        }
        
        // Allocate a dynamic block
        ulong block_size = stack_size + tls_size + PAGE_SIZE * 2;
        t->memory.thread_block_base = dalloc(p, block_size);
        
        // Set up memory layout
        t->memory.msg_send_offset = 0;
        t->memory.msg_recv_offset = PAGE_SIZE;
        
        t->memory.tls_start_offset = PAGE_SIZE * 2;
        
        t->memory.stack_top_offset = block_size;
        t->memory.stack_limit_offset = block_size - stack_size;
        
        // Allocate memory and map it
        ulong paddr = PFN_TO_ADDR(palloc(1));
        assert(paddr);
        int succeed = hal->map_user(
            p->page_dir_pfn,
            t->memory.thread_block_base + t->memory.msg_send_offset,
            paddr, PAGE_SIZE, 0, 1, 1, 0
        );
        assert(succeed);
        
        paddr = PFN_TO_ADDR(palloc(1));
        assert(paddr);
        succeed = hal->map_user(
            p->page_dir_pfn,
            t->memory.thread_block_base + t->memory.msg_recv_offset,
            paddr, PAGE_SIZE, 0, 1, 1, 0
        );
        assert(succeed);
        
        paddr = PFN_TO_ADDR(palloc(tls_size / PAGE_SIZE));
        assert(paddr);
        succeed = hal->map_user(
            p->page_dir_pfn,
            t->memory.thread_block_base + t->memory.tls_start_offset,
            paddr, tls_size, 0, 1, 1, 0
        );
        assert(succeed);
        
        paddr = PFN_TO_ADDR(palloc(stack_size / PAGE_SIZE));
        assert(paddr);
        succeed = hal->map_user(
            p->page_dir_pfn,
            t->memory.thread_block_base + t->memory.stack_limit_offset,
            paddr, stack_size, 0, 1, 1, 0
        );
        assert(succeed);
    }
    
    // Prepare the param. Note that for user thread, we have to use indirect map, therefore we can't do a simple mem write
    if (p->type == process_kernel) {
        ulong *param_ptr = (ulong *)(t->memory.thread_block_base + t->memory.stack_top_offset - sizeof(ulong));
        *param_ptr = param;
    } else {
    }
    
    // Context
    hal->init_context(&t->context, entry_point,
                      t->memory.thread_block_base + t->memory.stack_top_offset - sizeof(ulong) * 2,
                      p->user_mode);
    
    // Scheduling
    t->sched = enter_sched(t);
    
    // Insert the thread into the thread list
    t->next = p->threads.next;
    t->prev = NULL;
    p->threads.next = t;
    p->threads.count++;
    
    // Done
    return t;
}

void run_thread(struct thread *t)
{
    t->state = thread_normal;
    ready_sched(t->sched);
}

void idle_thread(struct thread *t)
{
    t->state = thread_normal;
    idle_sched(t->sched);
}

void init_thread()
{
    kprintf("Initializing thread manager\n");
    
    // Create salloc obj
    thread_salloc_id = salloc_create(sizeof(struct thread), 0, 0, NULL, NULL);
    kprintf("\tThread salloc ID: %d\n", thread_salloc_id);
    
    // Create idel kernel threads, one for each CPU
    int i;
    for (i = 0; i < hal->num_cpus; i++) {
        ulong param = i;
        struct thread *t = create_thread(kernel_proc, (ulong)&kernel_idle_thread, param, i, 0, 0);
        idle_thread(t);
        
        kprintf("\tKernel idle thread for CPU #%d created, thread ID: %p, thraed block base: %p\n", i, t->thread_id, t->memory.thread_block_base);
    }
    
    // Create kernel demo threads
    for (i = 0; i < 8; i++) {
        ulong param = i;
        struct thread *t = create_thread(kernel_proc, (ulong)&kernel_demo_thread, param, -1, 0, 0);
        run_thread(t);
        
        kprintf("\tKernel demo thread created, thread ID: %p, thraed block base: %p\n", t->thread_id, t->memory.thread_block_base);
    }
}