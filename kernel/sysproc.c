#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#ifdef PGTBL_SOL
#include "riscv.h"
#endif
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
    if (addr + n > UTOP)
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


#ifdef LAB_PGTBL
int
sys_vmprint(void)
{
  struct proc *p;

  p = myproc();
  vmprint(p->pagetable);
  return 0;
}
#endif

#ifdef LAB_PGTBL
// upper limit on the number of pages pgaccess() scans in one call:
// the bitmask for that many pages exactly fills one kernel page.
#define PGACCESS_MAX (8 * PGSIZE)

// pgaccess(base, npages, mask)
// Report which of the npages user pages starting at base have been
// accessed (read, written or executed) since the previous call, as
// a bitmask copied out to user address mask: bit i (counting from
// the least significant bit of the first byte) is set if page i
// was accessed. The access bits of the reported pages are cleared.
// base is rounded down to a page boundary.
// Returns 0 on success, or -1 if npages is out of range, if any page
// is outside the process's memory or not user-accessible, or if
// mask is not a writable user address.
uint64
sys_pgaccess(void)
{
  struct proc *p = myproc();
  uint64 base, maskaddr, va;
  int npages, nbytes, i;
  pte_t *pte;
  char *mask;

  argaddr(0, &base);
  argint(1, &npages);
  argaddr(2, &maskaddr);

  if (npages <= 0 || npages > PGACCESS_MAX)
    return -1;

  // every scanned page must lie within the process's memory [0, sz).
  base = PGROUNDDOWN(base);
  if (base >= p->sz || npages > (PGROUNDUP(p->sz) - base) / PGSIZE)
    return -1;

  // first pass: validate every page, so that a failed call
  // leaves all access bits untouched.
  for (i = 0; i < npages; i++) {
    va = base + (uint64)i * PGSIZE;
    pte = walk(p->pagetable, va, 0);
    // an unmapped page inside sz was allocated lazily by sbrk()
    // and never touched; it is simply reported as not accessed.
    if (pte != 0 && (*pte & PTE_V) && (*pte & PTE_U) == 0)
      return -1; // e.g. the stack guard page
  }

  // build the bitmask in a kernel buffer.
  if ((mask = kalloc()) == 0)
    return -1;
  nbytes = (npages + 7) / 8;
  memset(mask, 0, nbytes);
  for (i = 0; i < npages; i++) {
    va = base + (uint64)i * PGSIZE;
    pte = walk(p->pagetable, va, 0);
    if (pte != 0 && (*pte & PTE_V) && (*pte & PTE_A))
      mask[i / 8] |= 1 << (i % 8);
  }

  if (copyout(p->pagetable, p->sz, maskaddr, mask, nbytes) < 0) {
    kfree(mask);
    return -1;
  }

  // the bits reached the user, so now clear them, so the next
  // call only reports pages accessed after this one.
  for (i = 0; i < npages; i++) {
    if (mask[i / 8] & (1 << (i % 8))) {
      pte = walk(p->pagetable, base + (uint64)i * PGSIZE, 0);
      *pte &= ~PTE_A;
    }
  }
  // drop cached translations, so that the hardware sets PTE_A again
  // on the next access instead of using a stale TLB entry.
  sfence_vma();

  kfree(mask);
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
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

#ifdef LAB_LOCK
uint64
sys_cpupin(void)
{
  struct proc *p = myproc();
  int cpu;

  argint(0, &cpu);
  if (cpu < 0 || cpu >= NCPU)
    return -1;
  acquire(&p->lock);
  p->pincpu = &cpus[cpu];
  release(&p->lock);
  return 0;
}
#endif
