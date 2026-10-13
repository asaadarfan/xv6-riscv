#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// Characters that separate numbers in the input.  Start and end of file
// are treated as implicit separators as well.
static char *separators = " -\r\t\n./,";

// A number is a maximal run of decimal digits bounded by separators on
// both sides.  A run of characters that contains anything other than a
// digit (e.g. the "6" in "xv6") is not a number and must be ignored.

// Evaluate one completed token.  Print the value when it is a valid,
// digits-only number that is a multiple of 5 or 6.
static void
emit(char *num, int len, int valid)
{
  int v;

  if (len == 0 || !valid)
    return;

  num[len] = '\0';
  v = atoi(num);
  if (v % 5 == 0 || v % 6 == 0)
    printf("%d\n", v);
}

// Read fd a character at a time, splitting it into tokens and printing
// the numbers that are multiples of 5 or 6.
static void
scan(int fd)
{
  char buf[512];
  char num[16]; // long enough for any 32-bit value plus the NUL
  int len = 0;  // digits collected in the current token
  int valid = 1; // is the current token still a pure number?
  int n, i;

  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    for (i = 0; i < n; i++) {
      char c = buf[i];
      if (strchr(separators, c) != 0) {
        emit(num, len, valid);
        len = 0;
        valid = 1;
      } else if (c >= '0' && c <= '9') {
        if (len < (int)sizeof(num) - 1)
          num[len++] = c;
        else
          valid = 0; // overflows our buffer; not a number we handle
      } else {
        // a non-digit, non-separator character spoils the token
        valid = 0;
      }
    }
  }

  // End of file acts as a final separator for the last token.
  emit(num, len, valid);
}

int
main(int argc, char *argv[])
{
  int i, fd;

  // With no file arguments, read numbers from standard input.
  if (argc == 1) {
    scan(0);
    exit(0);
  }

  for (i = 1; i < argc; i++) {
    if ((fd = open(argv[i], O_RDONLY)) < 0) {
      fprintf(2, "sixfive: cannot open %s\n", argv[i]);
      continue;
    }
    scan(fd);
    close(fd);
  }

  exit(0);
}
