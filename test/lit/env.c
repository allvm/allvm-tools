// Some basic sanity checking of argument/env values and layout
//
// This has been compiled to the allexe
// 'Inputs/env.allexe', since we don't require
// a bitcode-capable compiler for building yet.
//
// RUN: rm -rf %t-jit && mkdir %t-jit
// RUN: rm -rf %t-static && mkdir %t-static
//
// Make sure we can run this at all, on both code paths
// (the code contains various checks as asserts)
// RUN: ALLVM_CACHE_DIR=%t-jit alley %p/Inputs/env.allexe
//
// RUN: ALLVM_CACHE_DIR=%t-static alley -force-static %p/Inputs/env.allexe
//
// ('env -i' runs command with empty env)
// RUN: env -i %alley %p/Inputs/env.allexe 1 |& FileCheck %s --check-prefix=TWOARGS
// RUN: env -i ALLVM_CACHE_DIR=%t-jit %alley %p/Inputs/env.allexe |& FileCheck %s --check-prefix=ENV
//
// TWOARGS: args: 2
//
// Check single env var set:
// ENV-NOT: env:
// ENV: env: ALLVM_CACHE_DIR=
// ENV-NOT: env:
//
// Finally, check the host compiler produces same results:
// RUN: WLLVM_CONFIGURE_ONLY=1 cc %s -o %t
// RUN: %t
// RUN: env -i %t 1 |& FileCheck %s --check-prefix=TWOARGS
// RUN: env -i ALLVM_CACHE_DIR=foobar %t |& FileCheck %s --check-prefix=ENV


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

extern char **environ;

int main(int argc, char *argv[], char *envp[]) {
  // envp isn't POSIX, but generally works

  // These should always be true :)
  assert(argc >= 0);
  assert(argv[argc] == NULL);

  // envp starting after arguments is not guaranteed in general,
  // but it certainly is when using musl.  Check:
  assert(&argv[argc+1] == envp);
  // Sanity check environ as well
  assert(envp == environ);

  // Check arguments, dumping them
  int i = 0;
  for (i = 0; argv[i]; ++i)
    printf("argv[%d]=%s\n", i, argv[i]);
  printf("argc: %d\n", argc);
  printf("args: %d\n", i);
  assert(argc == i);

  // Dump env vars
  for (i = 0; envp[i]; ++i) {
    printf("env: %s\n", envp[i]);
  }

  return 0;
}
