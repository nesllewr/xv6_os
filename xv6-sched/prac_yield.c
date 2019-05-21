#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

void myyield(void){
    yield();
}

void sys_myyield(void){
    yield();
}
