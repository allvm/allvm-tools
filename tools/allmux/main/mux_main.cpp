#include <cstring>

static char *basename(char *name) {
  char *p = strrchr(name, '/');
  if (p)
    return p + 1;
  return name;
}

extern "C" int __select(char *name, int argc, char *argv[], char *envp[]);

int main(int argc, char *argv[],
         char *envp[] /* not POSIX but needed by some things */) {
  char *name = basename(argv[0]);
  return __select(name, argc, argv, envp);
}
