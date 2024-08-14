#include "printk.h"
#include "sbi.h"

extern void schedule(void);
extern void test();

int start_kernel() {
    printk("2023");
    printk(" Hello RISC-V\n");
    schedule();
    test(); 
    return 0;
}
