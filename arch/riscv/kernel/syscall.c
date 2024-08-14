#include "syscall.h"
#include "proc.h"
#include "stdint.h"
#include "printk.h"
#include "string.h"
#include "defs.h"
uint64_t sys_getpid() {
    extern struct task_struct* current; 
    return current->pid;
}

size_t sys_write(unsigned int fd, const char* buf, size_t count) {
    size_t ret = 0;
    if(fd == 1) {
        int i;
        for(i=0; i < count; i++) {
            printk("%c",buf[i]);
        }
        ret = i;
    }else {
        ret = printk("[S]Unsupported fd = %d!\n",fd);
    }
    return ret;
}

uint64_t sys_clone(struct pt_regs *regs) {
    /*
        1. create a child task and copy the memory of parent task to the new task
        2. set the thread.ra to __ret_from_fork and set the thread.sp correctly
    */
    extern char __ret_from_fork[];
    extern struct task_struct* current; 
    extern struct task_struct* task[NR_TASKS];

    uint64_t r_pid = -1;
    for(int i = 2;i < NR_TASKS; i++) {
        if(task[i] == NULL){
            r_pid = i;
            break;
        }
    }
    if(r_pid == -1) return r_pid;
    
    struct task_struct* r = (struct task_struct*) alloc_page();
    memcpy((char *)r,(char *)current, PGSIZE);
    r->thread.ra = (uint64_t)__ret_from_fork;
    r->thread.sp = (uint64_t)((uint64_t)regs - (uint64_t)current + (uint64_t)r);
    r->pid = r_pid;
    r->thread.sscratch= csr_read(sscratch);
    current->thread.sscratch = r->thread.sscratch;

    struct pt_regs *r_pt_regs = (struct pt_regs *)r->thread.sp;
    r_pt_regs->a0 = 0;
    r_pt_regs->sp = r->thread.sp;    // pt_regs中存的是内核态sp，用户态栈在sscratch
    // printk("    [SB] 0x%x 0x%x 0x%x\n", r_pt_regs,r, r + PGSIZE);

    r->pgd = (uint64_t)alloc_page();
    memset(r->pgd, 0x0, PGSIZE);
    extern char _stext[];
    extern char _srodata[];
    extern char _sdata[];

    uint64_t sz_text = (uint64) _srodata - (uint64) _stext;
    uint64_t sz_rodata = (uint64) _sdata -  (uint64)_srodata;
    // 1: map _text
    uint64_t va = VM_START + OPENSBI_SIZE;
    uint64_t pa = va - PA2VA_OFFSET;
    create_mapping(r->pgd,va,pa,sz_text,11);
    // 2: map _rodata
    va += sz_text;
    pa += sz_text;
    create_mapping(r->pgd,va,pa,sz_rodata,3);
    // 3: map other
    va += sz_rodata;
    pa += sz_rodata;
    create_mapping(r->pgd,va,pa,PHY_SIZE - ((uint64)(_sdata) - (uint64)(_stext)) ,7);

    /*
        copy the memory of parent task to the new task
    */
    for(int i = 0; i < current->vma_cnt; i++) {
        uint64_t vma_start = current->vmas[i].vm_start;
        uint64_t vma_end = current->vmas[i].vm_end;
        for( uint64_t vma_va = vma_start; vma_va < vma_end; vma_va += PGSIZE) {
            uint64_t index = VPN2(vma_va);
            uint64_t* pgtb = (uint64_t *)current->pgd;
            uint64_t valid = (pgtb[index]) & 0x1;
            if(valid) {
                // 代表二级页表项有效，查看三级页表是否存在
                pgtb = (uint64_t *)( (((pgtb[index]) >> 10 ) << 12) + PA2VA_OFFSET);
                index = VPN1(vma_va);
                valid = (pgtb[index]) & 0x1;
                if(valid) {
                    // 代表三级页表项有效，查看物理地址是否有效
                    pgtb = (uint64_t *)( (((pgtb[index]) >> 10 ) << 12) + PA2VA_OFFSET);
                    index = VPN0(vma_va);
                    valid = pgtb[index] & 0x1;
                    if(valid) {
                        // 代表物理地址有效，开始copy
                        uint64_t pg_cp = alloc_page();
                        memcpy((char *)pg_cp, (char *)vma_va, PGSIZE);
                        uint64_t map_flag = current->vmas[i].vm_flags | 0b10001 ;
                        
                        create_mapping(r->pgd, vma_va, pg_cp - PA2VA_OFFSET, PGSIZE, map_flag); 
                    }

                }
            }
        }
    }

    task[r_pid] = r;
    // uint64_t sb= csr_read(sscratch);
    // r->thread.sscratch= csr_read(sscratch);
    // current->thread.sscratch = r->thread.sscratch;
    // printk("[Sys_Clone]: sb = 0x%lx sbr = 0x%lx sbc = 0x%lx\n", sb, r->thread.sscratch, current->thread.sscratch);
    return r_pid;
}

