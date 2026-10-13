#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// interpose(mask, path): sandbox the calling process.  The bits of
// mask name system calls (by their SYS_ number) that should be
// rejected.  A rejected open or exec is still permitted when it uses
// the allowed pathname.  Restrictions can only be tightened: the mask
// is OR'd into whatever is already set, and the allowed path is fixed
// by the first call, so a sandboxed program cannot relax its own
// confinement by calling interpose() again.
uint64
sys_interpose(void)
{
  int mask;
  char path[MAXPATH];
  struct proc *p = myproc();
  int len;

  argint(0, &mask);

  // The mask may only add restrictions, never remove them.
  p->sbmask |= mask;

  // Only the first interpose() call sets the allowed pathname.  A path
  // of "-" (or an unreadable argument) means "no path is exempt".
  if (!p->sbactive) {
    len = argstr(1, path, MAXPATH);
    if (len > 0 && strncmp(path, "-", MAXPATH) != 0)
      safestrcpy(p->sbpath, path, sizeof(p->sbpath));
    else
      p->sbpath[0] = 0;
    p->sbactive = 1;
  }

  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
