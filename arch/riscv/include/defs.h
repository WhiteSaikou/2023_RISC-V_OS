#pragma once

#include "stdint.h"

#define PHY_START 0x0000000080000000
#define PHY_SIZE  128 * 1024 * 1024 
#define PHY_END   (PHY_START + PHY_SIZE)

#define PGSIZE 0x1000 // 4KB
#define PGROUNDUP(addr) ((addr + PGSIZE - 1) & (~(PGSIZE - 1))) //跳转到当前地址所在页的下一个页的起始地址
#define PGROUNDDOWN(addr) (addr & (~(PGSIZE - 1)))

#define OPENSBI_SIZE (0x200000)

#define VM_START (0xffffffe000000000) 
#define VM_END   (0xffffffff00000000)
#define VM_SIZE  (VM_END - VM_START)


#define VPN0(va) (((uint64)va >> 12) & 0x1ff)
#define VPN1(va) (((uint64)va >> 21) & 0x1ff)
#define VPN2(va) (((uint64)va >> 30) & 0x1ff)

#define PA2VA_OFFSET (VM_START - PHY_START) // FFFFFFDF80000000

#define USER_START (0x0000000000000000) // user space start virtual address
#define USER_END   (0x0000004000000000) // user space end virtual address

#define BYTES2PAGES(bytes) (( (bytes) % PGSIZE) ? (bytes) / PGSIZE + 1 : (bytes) / PGSIZE )

#include "types.h"

#define csr_read(csr)                       \
({                                          \
    register uint64 __v;                    \
    asm volatile ("csrr %0, " #csr          \
                  : "=r" (__v));             \
    __v;                                    \
})

// arch/riscv/include/proc.h
#define csr_write(csr, val)                         \
({                                                  \
    uint64 __v = (uint64)(val);                     \
    asm volatile ("csrw " #csr ", %0"               \
                    : : "r" (__v)                   \
                    : "memory");                    \
})

struct pt_regs {
    uint64_t zero;    // x0      0
    uint64_t ra;      // x1      8
    uint64_t sp;      // x2      16
    uint64_t gp;      // x3      24
    uint64_t tp;      // x4      32

    uint64_t t0;      // x5      40
    uint64_t t1;      // x6      48
    uint64_t t2;      // x7      56

    uint64_t s0;      // x8      64
    uint64_t s1;      // x9      72

    uint64_t a0;      // x10     80
    uint64_t a1;      // x11     88
    uint64_t a2;      // x12     96
    uint64_t a3;      // x13     104
    uint64_t a4;      // x14     112
    uint64_t a5;      // x15     120
    uint64_t a6;      // x16     128
    uint64_t a7;      // x17     136

    uint64_t s2;      // x18     144
    uint64_t s3;      // x19     152
    uint64_t s4;      // x20     160
    uint64_t s5;      // x21     168
    uint64_t s6;      // x22     176
    uint64_t s7;      // x23     184
    uint64_t s8;      // x24     192
    uint64_t s9;      // x25     200
    uint64_t s10;     // x26     208
    uint64_t s11;     // x27     216

    uint64_t t3;      // x28     224
    uint64_t t4;      // x29     232
    uint64_t t5;      // x30     240
    uint64_t t6;      // x31     248


    uint64_t sepc;
    uint64_t sstatus;
    uint64_t stval;
    uint64_t sscratch;
    uint64_t scause;
};

