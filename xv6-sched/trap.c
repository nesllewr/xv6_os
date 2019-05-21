#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#define quantum1 4
#define quantum2 8

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
extern int numpro;
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  
  SETGATE(idt[128],1,SEG_KCODE<<3, vectors[128], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
     if(myproc()->killed)
      exit();
    return;
  }
  if(tf->trapno == 128){
    if(myproc()->killed)
        exit();
    myproc()->tf = tf;
    cprintf("user interrupt 128 called!\n");
    exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      updatetime();
      wakeup(&ticks);
      release(&tickslock);
    }
    //alarm sys
    if(myproc() !=0 &&(tf->cs &3)==3){
      myproc()->elapsedticks++;
      if(myproc()->alarmticks && myproc()->elapsedticks==myproc()->alarmticks){
        myproc()->elapsedticks =0;//initialize
        tf->esp -= 4;
        *(unsigned int*)(tf->esp) = tf -> eip;
        tf->eip = (unsigned int)(myproc()->alarmhandler);
      }
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
  #ifdef FCFS_SCHED
  if(myproc() && myproc()->runningtime >100){
    kill(myproc()->pid);
    cprintf("killed process %d\n",myproc()->pid);
  }
  #endif
  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
    tf->trapno == T_IRQ0+IRQ_TIMER){
    #ifdef FCFS_SCHED
   // yield();
    #elif MLFQ_SCHED
    if(myproc()->level==0 && myproc()->passedticks==quantum1&&myproc()->mono==0){
      myproc()->level =1;
      if(myproc()->pid>1) numpro--;
      myproc()->passedticks = 0;
      yield();
    }
    else if(myproc()->level==1 && myproc()->passedticks==quantum2&&myproc()->mono==0){
      if(myproc()->priority > 0) myproc()->priority--;
      myproc()->passedticks= 0;
      yield();
    }

    #endif
  }

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
  if(myproc() && myproc()->mono ==1){

  }
}
