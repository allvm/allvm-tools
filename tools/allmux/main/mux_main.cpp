#include <cstddef>
#include <cstdint>
#include <cstring>

struct main_info {
  int (*main)(int, char *[], char *[]);
  const char *name;
};

extern "C" main_info mains[] = {};

char *basename(char *name) {
  char *p = strrchr(name, '/');
  if (p)
    return p + 1;
  return name;
}

int main(int argc, char *argv[],
         char *envp[] /* not POSIX but needed by some things */) {
  // TODO: Look at busybox and coreutils for inspiration/ideas?

  // TODO: Static initializers?

  // TODO: Teach 'alley' how to do this, instead?
  // This loop could definitely be emitted directly...

  // XXX: ifunc-based selection?
  // * needs dynamic linker (which makes sense), not what we currently do
  // * sounds brutal to analyze statically :3
  // * musl doesn't support it: http://www.openwall.com/lists/musl/2014/11/11/2
  // * can probably roll our own w/o too much trouble

  char *name = basename(argv[0]);
#pragma clang loop unroll(full)
  for (size_t idx = 0; mains[idx].main; ++idx) {
    if (!strcmp(name, mains[idx].name)) {
      return mains[idx].main(argc, argv, envp);
    }
  }

  // TODO: best way to handle this?
  return -1;
}
