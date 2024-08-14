// arch/riscv/include/proc.h

#include "types.h"
#include "stdint.h"

#define NR_TASKS  (1 + 15) // the maximum number of threads supported

#define TASK_RUNNING    0 

#define PRIORITY_MIN 1
#define PRIORITY_MAX 10

#define VM_X_MASK         0x0000000000000008 //X W R A
#define VM_W_MASK         0x0000000000000004
#define VM_R_MASK         0x0000000000000002
#define VM_ANONYM         0x0000000000000001


struct thread_info {
    uint64 kernel_sp;   //0
    uint64 user_sp;     //8
};

/* the contex of thread */
struct thread_struct {
    uint64 ra;          //48
    uint64 sp;          //56
    uint64 s[12];       //64
    uint64 sepc;        // 160
    uint64 sstatus;     // 168
    uint64 sscratch;    // 176
};


struct vm_area_struct {
    uint64_t vm_start;          
    uint64_t vm_end;            
    uint64_t vm_flags;          

    // uint64_t file_offset_on_disk // only one file
                                

    uint64_t vm_content_offset_in_file;   // the p_offset of every segment of ELF

    uint64_t vm_content_size_in_file;     // the size of segment in ELF. Not equal to vm_end - vm_start
};

/* PCB */
struct task_struct {
    struct thread_info thread_info;
    uint64 state;    // thread state 16
    uint64 counter;  // time piece 24
    uint64 priority; //  1最低 10最高 32
    uint64 pid;      

    struct thread_struct thread;    // 48
    uint64* pgd;    // 184
    uint64_t vma_cnt;  
    struct vm_area_struct vmas[0];          
};

void do_mmap(struct task_struct *task, uint64_t addr, uint64_t length, uint64_t flags,
    uint64_t vm_content_offset_in_file, uint64_t vm_content_size_in_file);

struct vm_area_struct *find_vma(struct task_struct *task, uint64_t addr);




void task_init();

void do_timer();

void schedule();

void switch_to(struct task_struct* next);

void dummy();