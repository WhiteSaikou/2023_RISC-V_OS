#include "printk.h"
#include "defs.h"
#include "types.h"


void test() {
    uint64 r_sstatus = csr_read(sstatus);
    csr_write(sscratch,2102);
    while (1);
}
