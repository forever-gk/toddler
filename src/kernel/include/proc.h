#ifndef __KERNEL_INCLUDE_PROC__
#define __KERNEL_INCLUDE_PROC__


#include "common/include/data.h"
#include "common/include/task.h"


/*
 * Scheduling
 */
enum sched_state {
    sched_enter,
    sched_ready,
    sched_run,
    sched_exit,
};

struct sched {
    // Sched list
    struct sched *prev;
    struct sched *next;
    
    // Sched control info
    ulong sched_id;
    enum sched_state state;
    
    // Priority
    int base_priority;
    int priority;
    
    // Containing proc and thread
    ulong proc_id;
    struct process *proc;
    ulong thread_id;
    struct thread *thread;
    
    // Affinity
    int pin_cpu_id;
};

struct sched_list {
    ulong count;
    struct sched *next;
};


/*
 * Thread
 */
enum thread_state {
    thread_enter,
    thread_ready,
    thread_run,
    thread_stall,
    thread_wait,
    thread_exit,
};

struct thread_memory {
    ulong thread_block_base;
    
    ulong stack_top_offset;
    ulong stack_limit_offset;
    ulong tls_start_offset;
    
    ulong msg_recv_offset;
    ulong msg_send_offset;
};

struct thread {
    // Thread list
    struct thread *next;
    struct thread *prev;
    
    // Thread info
    ulong thread_id;
    enum thread_state state;
    struct thread_memory memory;
    
    // Containing process
    ulong proc_id;
    struct process *proc;
    
    // Context
    struct context context;
    
    // CPU affinity
    int pin_cpu_id;
    
    // Scheduling
    ulong sched_id;
    struct sched *sched;
};

struct thread_list {
    ulong count;
    struct thread *next;
};


/*
 * Process
 */
enum process_type {
    // Native types
    process_kernel,
    process_driver,
    process_system,
    process_user,
    
    // Emulation - e.g. running Linux on top of Toddler
    process_emulate,
};

enum process_state {
    process_enter,
    process_normal,
    process_error,
    process_exit,
};

struct process_memory {
    // Entry point
    ulong entry_point;
    
    // Memory layout
    ulong program_start;
    ulong program_end;
    
    ulong dynamic_top;
    ulong dynamic_bottom;
    
    ulong heap_start;
    ulong heap_end;
};

struct process {
    // Process list
    struct process *next;
    struct process *prev;
    
    // Process ID, -1 = No parent
    ulong proc_id;
    ulong parent_id;
    
    // Name and URL
    char *name;
    char *url;
    
    // Type and state
    enum process_type type;
    enum process_state state;
    int user_mode;
    
    // Page table
    ulong page_dir_pfn;
    
    // Virtual memory
    struct process_memory memory;
    
    // Thread list
    struct thread_list threads;
    
    // Scheduling
    uint priority;
};

struct process_list {
    ulong count;
    struct process  *next;
};


/*
 * Thread Control Block
 */
struct thread_control_block {
    ulong proc_id;
    ulong thread_id;
    
    int cpu_id;
    
    void *tls;
    void *msg_recv;
    void *msg_send;
};



/*
 * Process
 */
extern struct process *kernel_proc;;

extern void init_process();


/*
 * Thread
 */
extern void init_thread();
extern void kernel_dummy_thread(ulong param);


/*
 * Scheduling
 */
extern void init_sched();
extern struct sched *enter_sched(struct thread *t);
extern void read_sched(struct sched *s);
extern void exit_sched(struct sched *s);
extern void clean_sched(struct sched *s);
extern void sched();


#endif
