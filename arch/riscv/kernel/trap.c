#include"types.h"
#include"defs.h"
#include"printk.h"
#include"clock.h"
#include"proc.h"
#include "syscall.h"
#include "stdint.h"

extern char _sramdisk[];
extern char _eramdisk[];
void pg_fault_handle(uint64_t stval, uint64_t e_code) {
    extern struct task_struct* current; 
    struct vm_area_struct *target_vma = find_vma(current, stval);
    if(target_vma == NULL) {
        printk("[S]Page Fault Failed to Handle: 0x%lx is not in VMA\n",stval);
        return;
    }
    uint64_t p_flags = target_vma->vm_flags; // E W R

    if(
       !( ( (p_flags & VM_X_MASK) && e_code == 12 ) ||
        ( (p_flags & VM_R_MASK) && e_code == 13 ) ||
        ( (p_flags & VM_W_MASK) && e_code == 15 ) )
    )   {
        printk("[S]Page Fault Failed to Handle: 0x%lx flags error\n",stval);
        return;
    }
    
    if(p_flags & VM_ANONYM) {
        uint64_t mem_alloc = alloc_page();
        create_mapping(current->pgd, target_vma->vm_start, mem_alloc - PA2VA_OFFSET,PGSIZE, p_flags | 0b10001);
    }else {
        uint64_t pg_num = (target_vma->vm_end - target_vma->vm_start) / PGSIZE;
        uint64_t mem_alloc = alloc_pages(pg_num);
        memcpy((char *)mem_alloc + target_vma->vm_content_offset_in_file,
                (_sramdisk + target_vma->vm_content_offset_in_file),
                target_vma->vm_content_size_in_file);
        create_mapping(current->pgd, target_vma->vm_start, mem_alloc - PA2VA_OFFSET, pg_num*PGSIZE, p_flags | 0b10001);
    }
    
}


void trap_handler(uint64_t scause, uint64_t sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断trap类型
    // static int other_trap_happend = 0; // 只接收第一次其他trap;
    uint64_t interrupt = scause >> 63;
    uint64_t Exception_Code = (scause << 1) >> 1;
    if(interrupt) {
        switch (Exception_Code) {
            case 5:
                // printk("[S]Timer Interrupt\n");
                clock_set_next_event();
                do_timer();
                break;
            default:
                break;
        }
    }else {
        switch (Exception_Code) {
            case 8:
                regs->sepc = sepc + 4;
                uint64_t SYS_CODE = regs->a7;
                switch (SYS_CODE) {
                    case SYS_WRITE:
                            regs->a0 = sys_write(regs->a0,(char*)regs->a1,regs->a2);
                        break;

                    case SYS_GETPID:
                            regs->a0 = sys_getpid();
                        break;
                    case SYS_CLONE:
                            regs->a0 = sys_clone(regs);
                    default:
                        break;
                }
                break;
            case 12:
            case 13:
            case 15:
                printk("[S]Page Fault: stval = 0x%lx scause = 0x%lx sepc = 0x%lx\n",regs->stval, regs-> scause, regs->sepc);
                pg_fault_handle(regs->stval,Exception_Code);
                break;
            default:
                break;
        }
    }

}