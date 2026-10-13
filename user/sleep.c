#include "kernel/types.h"
#include "user/user.h"

// sleep: pause the calling process for a user-specified number of ticks.
// Usage: sleep <ticks>
int
main(int argc, char *argv[])
{
  int ticks;

  // The number of ticks is mandatory; report an error if it is missing.
  if (argc != 2) {
    fprintf(2, "Usage: sleep <ticks>\n");
    exit(1);
  }

  // Command-line arguments arrive as strings, so convert to an integer.
  ticks = atoi(argv[1]);

  // Ask the kernel to pause us.  Check the return value as required.
  if (pause(ticks) < 0) {
    fprintf(2, "sleep: pause failed\n");
    exit(1);
  }

  exit(0);
}
