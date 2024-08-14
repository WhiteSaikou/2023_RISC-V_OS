// arch/riscv/kernel/vm.c

/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
#include<string.h>
#include "defs.h"
#include "mm.h"
unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    /* 
    映射 1GB 的物理内存（kernel code） 1GB 打野
    va 的 64bit | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    Page Table Entry 的权限 V | R | W | X 位设置为 1

    RISC-V 64处理器在地址转换过程中，只要表项中的 V 为 1 且 R/W/X 不全为 0 就会直接从当前的页表项中取出物理页号
    再接上页内偏移，就完成最终的地址转换。注意这个过程可以发生在多级页表的任意一级。
    ref:https://rcore-os.cn/rCore-Tutorial-Book-v3/chapter4/3sv39-implementation-1.html
    */
    memset(early_pgtbl, 0, PGSIZE);
    uint64 pa = PHY_START;

    // 虚拟地址映射

    // 1111 1111 1111 1111 1111 1111 1110 0000 00
    // 
    uint64 va = VM_START;
    int index = VPN2(va);
    early_pgtbl[index] = (((pa >> 30) & 0x3FFFFFF) << 28) | 0xf;
}


/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

extern char _stext[];
extern char _srodata[];
extern char _sdata[];





/**** 创建多级页表映射关系 *****/
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小，单位为字节
    perm 为映射的权限 (即页表项的低 8 位)

    使用 V bit 来判断页表项是否存在
    */
    uint64 v_end = va + sz;
    uint64* new_addr;
    while(va < v_end) {
        uint64 PTE1 = pgtbl[VPN2(va)];       // 此虚拟地址对应的一级页表项
        if(!(PTE1 & 1)) {                      // 尚未分配二级页表
           new_addr = (uint64* )kalloc();
            PTE1 = ((((uint64)new_addr - PA2VA_OFFSET)>> 12) << 10) | 1;
            pgtbl[VPN2(va)] = PTE1;
        }

        uint64 *pgtbl2 = (uint64 *)(((PTE1 >> 10) << 12) + PA2VA_OFFSET);
        uint64 PTE2 = pgtbl2[VPN1(va)];
        if(!PTE2 & 1) {
            new_addr = (uint64*)kalloc();
            PTE2 = ((((uint64)new_addr - PA2VA_OFFSET)>> 12) << 10) | 1;
            pgtbl2[VPN1(va)] = PTE2;
        }
        
        uint64 *pgtbl3 = (uint64 *)(((PTE2 >> 10) << 12) + PA2VA_OFFSET );
        pgtbl3[VPN0(va)] = (uint64)(((pa >> 12) << 10) | perm);

        va += PGSIZE;
        pa += PGSIZE;
    }
}


void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    uint64 sz_text = (uint64) _srodata - (uint64) _stext;
    uint64 sz_rodata = (uint64) _sdata -  (uint64)_srodata;
    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    uint64 va = VM_START + OPENSBI_SIZE;
    uint64 pa = va - PA2VA_OFFSET;
    create_mapping(swapper_pg_dir,va,pa,sz_text,11);

    // mapping kernel rodata -|-|R|V
    va += sz_text;
    pa += sz_text;
    create_mapping(swapper_pg_dir,va,pa,sz_rodata,3);

    // mapping other memory -|W|R|V
    va += sz_rodata;
    pa += sz_rodata;
    create_mapping(swapper_pg_dir,va,pa,PHY_SIZE - ((uint64)(_sdata) - (uint64)(_stext)) ,7);
    // set satp with swapper_pg_dir
    uint64 satp_value = 0x8000000000000000 + (((uint64)(swapper_pg_dir) - PA2VA_OFFSET) >> 12);
    csr_write(satp, satp_value);

    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");
    return;
}


