.extern start_kernel
.extern mm_init
.extern task_init
.extern setup_vm
.extern early_pgtbl
.extern setup_vm_final
    .section .text.init
    .globl _start
_start:
    la sp, boot_stack_top
    li x5,0xFFFFFFDF80000000
    sub sp,sp,x5            # 此处尚未设置satp,需将sp转为物理地址
    call setup_vm
    call relocate

    jal ra,mm_init
    jal setup_vm_final
    jal ra,task_init
    # ----------------------
    la x5,_traps
    csrw stvec,x5
        # set stvec = _traps

    # ----------------------

        # set sie[STIE] = 1
    csrr x5,sie
    ori x5,x5,0x20
    csrw sie,x5
    
    # ----------------------
    rdtime x5
    li x6, 10000000
    add a0,x5,x6
    addi a7,x0,0x00
    addi a6,x0,0
    ecall
        # set first time interrupt

    # ----------------------
    # csrr x5,sstatus
    # ori x5,x5,0x2
    # csrw sstatus,x5
    #     # set sstatus[SIE] = 1

    # ----------------------

    jal start_kernel  # jump to start_kernel and save position to ra
    .globl relocate
relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET 
    li x5,0xFFFFFFDF80000000
    add ra,ra,x5
    add sp,sp,x5

    # set satp with early_pgtbl
    la x6,early_pgtbl
    sub x5,x6,x5
    srli x5,x5,12           # 设置PPN
    addi x6,x0,8
    slli x6,x6,60
    add x5,x6,x5
    la x6,aaa
    csrw stvec,x6
    csrw satp,x5

    # flush tlb
    
    .globl aaa
aaa:
        sfence.vma zero, zero

    # flush icache
    fence.i

    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # stack size

    .globl boot_stack_top
boot_stack_top:

