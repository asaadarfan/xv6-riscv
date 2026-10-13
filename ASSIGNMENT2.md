# CS311 Operating Systems — Assignment 02 (xv6 System Calls)

All work for both tasks lives on this single `assignment2` branch.

## Task 1 — xv6 Unix utilities (MIT util lab)
Implemented user programs:
- `user/sleep.c`   — pause for N ticks, with argument checking.
- `user/sixfive.c` — print numbers that are multiples of 5 or 6 from
  each file (or stdin); reports `cannot open` for bad files.
- `user/memdump.c` — `memdump()` with the `i p h c s S` formats and
  strict bounds checking (never reads past `len`).
- `user/find.c`    — recursive `find`, including `-exec cmd`.

Grade it with:

    ./grade-lab-util        # expected: Score 156/156

## Task 2 — add a system call (MIT syscall lab)
- New `interpose(mask, path)` system call that sandboxes a process by
  rejecting the system calls named in `mask`. A masked `open`/`exec` is
  still allowed on the permitted `path`. The mask only tightens and the
  path is fixed by the first call, so a process cannot escape its own
  sandbox; both are inherited across `fork`.
  Touches: `user/user.h`, `user/usys.pl`, `kernel/syscall.h`,
  `kernel/sysproc.c`, `kernel/syscall.c`, `kernel/proc.h`, `kernel/proc.c`.
- `user/attack.c` — recovers a freed secret from reused, unscrubbed pages.
- `answers-syscall.txt` — answers to the gdb questions.

Grade it with:

    make grade              # (LAB=syscall) expected: Score 60/60
    #   or: ./grade-lab-syscall

## Build / run
    make qemu               # boot xv6 in QEMU (Ctrl-a x to quit)

Requires a riscv64 gcc toolchain and `qemu-system-riscv64` in PATH.
`time.txt` records the hours spent.
