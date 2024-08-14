//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"
#include "test.h"
#include <string.h>
#include "elf.h"
#define UINT_MAX 0xFFFFFFFFFFFFFFFF

//arch/riscv/kernel/proc.c

extern void __dummy();
extern void __switch_to(struct task_struct* prev, struct task_struct* next);
struct task_struct* idle;           // idle process
struct task_struct* current;        // the current task
struct task_struct* task[NR_TASKS]; // all tasks 

extern unsigned long  swapper_pg_dir[];
/**
 * new content for unit test of 2023 OS lab2
*/
extern uint64 task_test_priority[]; // initial task[i].priority
extern uint64 task_test_counter[];  // initial task[i].counter

extern char _sramdisk[];
extern char _eramdisk[];

extern void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm);

static uint64_t load_program(struct task_struct* task);

void task_init() {
    test_init(NR_TASKS);
    struct task_struct* r = (struct task_struct*) kalloc();

    r->state = TASK_RUNNING;
    r->counter = 0;
    r->priority = 0;
    r->pid = 0;
    r->thread.ra = 0;
    r->thread.sp = (uint64)r + PGSIZE ;
    current = r;
    task[0] = r;

    for(int i=1;i < 2;i++) {
        r = (struct task_struct*) kalloc();
        r->state = TASK_RUNNING;
        r->counter  = task_test_counter[i];
        r->priority = task_test_priority[i];
        r->pid = i;
        r->thread.ra = (uint64)__dummy;
        r->thread.sp = (uint64)r + PGSIZE ;

        
        // 申请页表
        r->pgd = (uint64* )alloc_page();
        memcpy(r->pgd,swapper_pg_dir,PGSIZE);

        load_program(r);
        task[i] = r;
        
    }
    for(int i = 2; i < NR_TASKS; i++)
        task[i] = NULL;
    printk("...proc_init done!\n");
}

// arch/riscv/kernel/proc.c
void dummy() {
    schedule_test();
    uint64 MOD = 1000000007; 
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if ((last_counter == -1 || current->counter != last_counter) && current->counter > 0) {
            if(current->counter == 1){
                --(current->counter);   // forced the counter to be zero if this thread is going to be scheduled
            }                           // in case that the new counter is also 1, leading the information not printed.
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
        }
    }
}


void switch_to(struct task_struct* next) {
    printk("[S]Switch to PID = %d\n",next->pid);
    struct task_struct* temp = current;
    current = next;
    __switch_to(temp,next);

}

void do_timer(void) {
    if(current->pid == 0)
        schedule();
    else {
        if(--current->counter) 
            return;
        else
            schedule();
    }
}




void schedule(void) {
#ifdef SJF
    struct task_struct* next = NULL;
    uint64 counter_min = UINT_MAX;
    for(int i = 1;i < NR_TASKS;i++) {
        if(task[i] == NULL) continue;
        if(task[i]->counter > 0 && task[i]->counter < counter_min) {
            counter_min = task[i]->counter;
            next = task[i];
        }
    }
    while(next == NULL) {
        
        counter_min = UINT_MAX;
        for(int i = 1;i < NR_TASKS;i++) {
            if(task[i] == NULL) continue;
            task[i]->counter = rand();
            if(task[i]->counter < counter_min) {
                counter_min = task[i]->counter;
                next = task[i];
            }
        }
    }
    switch_to(next);
    return ;
#elif defined(PRIORITY)
    struct task_struct* next = NULL;
    uint64 counter_max = 0;
    for(int i = NR_TASKS - 1;i > 0;i--) {
        if(task[i] == NULL) continue;
        if(task[i]->counter > counter_max) {
            counter_max = task[i]->counter;
            next = task[i];
        }
    }

    while(next == NULL) {
        counter_max = 0;
        for(int i = NR_TASKS - 1;i > 0;i--) {
            if(task[i] == NULL) continue;
            task[i]->counter = (task[i]->counter >> 1) + task[i]->priority;
            if(task[i]->counter > counter_max) {
                counter_max = task[i]->counter;
                next = task[i];
            }
        }
    }
    switch_to(next);
    return ;
#else
#endif
}

static uint64_t load_program(struct task_struct* task) {
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)_sramdisk;

    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff;
    int phdr_cnt = ehdr->e_phnum;
    static uint64 perm[8] = {17,25,21,29,19,27,23,31};
    Elf64_Phdr* phdr;
    int load_phdr_cnt = 0;
    for (int i = 0; i < phdr_cnt; i++) {
        phdr = (Elf64_Phdr*)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD) {
            do_mmap(task,phdr->p_vaddr,phdr->p_memsz,perm[phdr->p_flags] & 0b1110,phdr->p_offset,phdr->p_filesz);
        }
    }
    do_mmap(task,USER_END - PGSIZE,PGSIZE,VM_W_MASK | VM_R_MASK | VM_ANONYM ,0,0);
    task->thread.sepc = ehdr->e_entry;
    task->thread.sstatus = 0x40020;
    task->thread.sscratch = USER_END;
}

void do_mmap(struct task_struct *task, uint64_t addr, uint64_t length, uint64_t flags,
    uint64_t vm_content_offset_in_file, uint64_t vm_content_size_in_file) 
{
    // find the first page and the last page between which the new vma is located
    uint64_t start_addr = PGROUNDDOWN(addr);
    uint64_t end_addr = PGROUNDUP(addr + length);

    // create a new vma
    struct vm_area_struct new_vma;
    new_vma.vm_start = start_addr;
    new_vma.vm_end = end_addr;
    new_vma.vm_flags = flags;
    new_vma.vm_content_offset_in_file = vm_content_offset_in_file;
    new_vma.vm_content_size_in_file = vm_content_size_in_file;

    // 将其拷贝到vmas里
    memcpy((uint64_t)((uint64_t)(task->vmas) + task->vma_cnt * sizeof(struct vm_area_struct)),
        &new_vma,
        sizeof(struct vm_area_struct));
    ++task->vma_cnt;
}

struct vm_area_struct *find_vma(struct task_struct *task, uint64_t addr) {
    struct vm_area_struct *target = NULL;
    for(int i = 0; i < task->vma_cnt; ++i) {
        if(addr >= task->vmas[i].vm_start && addr <= task->vmas[i].vm_end) {
            target = &task->vmas[i];
            break;
        }
    }
    return target;
}