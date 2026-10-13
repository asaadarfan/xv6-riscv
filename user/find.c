#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

static char *target;      // the name we are searching for
static char **exec_argv;  // command for -exec, or 0 if printing matches
static int exec_argc;     // number of words in exec_argv

// Return a pointer to the final component of a path (the part after the
// last '/'), e.g. "a/aa/b" -> "b", "b" -> "b".
static char *
lastname(char *path)
{
  char *p;

  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  return p + 1;
}

// Handle a path whose name matches the target: either run the -exec
// command on it or print it.
static void
matched(char *path)
{
  if (exec_argv == 0) {
    printf("%s\n", path);
    return;
  }

  // Build "cmd args... path" and run it in a child process.
  char *argv[MAXARG];
  int i;

  for (i = 0; i < exec_argc && i < MAXARG - 2; i++)
    argv[i] = exec_argv[i];
  argv[i++] = path;
  argv[i] = 0;

  int pid = fork();
  if (pid < 0) {
    fprintf(2, "find: fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    exec(argv[0], argv);
    fprintf(2, "find: exec %s failed\n", argv[0]);
    exit(1);
  }
  wait(0); // reap the child so we don't leak process slots
}

// Recursively search the file or directory tree rooted at `path`.
static void
find(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  // A directory as well as a file can match the target name.
  if (strcmp(lastname(path), target) == 0)
    matched(path);

  switch (st.type) {
  case T_FILE:
  case T_DEVICE:
    break;

  case T_DIR:
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
      fprintf(2, "find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      // Do not recurse into the current or parent directory.
      if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = '\0';
      find(buf);
    }
    break;
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if (argc < 3) {
    fprintf(2, "Usage: find <dir> <name> [-exec cmd args...]\n");
    exit(1);
  }

  target = argv[2];
  exec_argv = 0;
  exec_argc = 0;

  // Everything after "-exec" is the command to run on each match.
  for (i = 3; i < argc; i++) {
    if (strcmp(argv[i], "-exec") == 0) {
      exec_argv = &argv[i + 1];
      exec_argc = argc - (i + 1);
      if (exec_argc == 0) {
        fprintf(2, "find: -exec requires a command\n");
        exit(1);
      }
      break;
    }
  }

  find(argv[1]);
  exit(0);
}
