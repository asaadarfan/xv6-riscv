#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data, int len);

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    printf("Example 1:\n");
    int a[2] = {61810, 2026};
    memdump("ii", (char *)a, sizeof(a));

    printf("Example 2:\n");
    memdump("S", "a string", sizeof("a string"));

    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *)&s, sizeof(s));

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;

    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");

    printf("Example 4:\n");
    memdump("pihcS", (char *)&example, sizeof(example));

    printf("Example 5:\n");
    memdump("sccccc", (char *)&example, sizeof(example));
  } else if (argc == 2) {
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while (n < sizeof(data)) {
      int nn = read(0, data + n, sizeof(data) - n);
      if (nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data, n);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

// Print the `len` valid bytes at `data` according to the format string
// `fmt`.  Each format character consumes some bytes of `data`:
//   i  4 bytes, printed as a signed 32-bit decimal integer
//   p  8 bytes, printed as a 64-bit hexadecimal integer
//   h  2 bytes, printed as a 16-bit decimal integer
//   c  1 byte, printed as an ASCII character
//   s  8 bytes holding a pointer to a C string, which is printed
//   S  all remaining bytes, printed as a C string (stops at a NUL)
// memdump never reads past the `len` valid bytes.  If a format character
// needs more bytes than remain, it prints an error and stops.
void
memdump(char *fmt, char *data, int len)
{
  int off = 0; // bytes of `data` consumed so far
  int i;

  for (i = 0; fmt[i]; i++) {
    char f = fmt[i];
    int need;

    // Number of bytes this format character requires (0 for 'S', which
    // simply consumes whatever is left).
    switch (f) {
    case 'i': need = 4; break;
    case 'p': need = 8; break;
    case 'h': need = 2; break;
    case 'c': need = 1; break;
    case 's': need = 8; break;
    case 'S': need = 0; break;
    default:
      printf("memdump: unknown format '%c'\n", f);
      return;
    }

    // Bounds check: refuse to read past the valid data.
    if (off + need > len) {
      printf("memdump: not enough data for '%c'\n", f);
      return;
    }

    switch (f) {
    case 'i': {
      int v;
      memmove(&v, data + off, sizeof(v)); // copy to avoid unaligned access
      printf("%d\n", v);
      off += 4;
      break;
    }
    case 'p': {
      uint64 v;
      memmove(&v, data + off, sizeof(v));
      printf("%lx\n", v);
      off += 8;
      break;
    }
    case 'h': {
      unsigned short v;
      memmove(&v, data + off, sizeof(v));
      printf("%d\n", (int)v);
      off += 2;
      break;
    }
    case 'c': {
      printf("%c\n", data[off]);
      off += 1;
      break;
    }
    case 's': {
      char *p;
      memmove(&p, data + off, sizeof(p));
      printf("%s\n", p);
      off += 8;
      break;
    }
    case 'S': {
      // Print the remaining bytes up to a NUL or the end of valid data.
      int j;
      for (j = off; j < len && data[j] != '\0'; j++)
        printf("%c", data[j]);
      printf("\n");
      off = len; // S consumes the rest of the data
      break;
    }
    }
  }
}
