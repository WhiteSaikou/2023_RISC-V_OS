// QEMU 10MHz。
#include"sbi.h"
#include"clock.h"

unsigned long get_cycles() {
    unsigned long mtime;
    __asm__ volatile (
		"rdtime %[mtime]"
        : [mtime] "=r" (mtime)
        :
        : "memory"
    );
    return mtime;
}

void clock_set_next_event() {
    // the next time point of clock interrupter 
    unsigned long next = get_cycles() + 10000000;
    sbi_ecall(0x00,0x0,next,0,0,0,0,0);
} 