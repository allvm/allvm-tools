// This has been compiled into the allexe: Inputs/dlopen
// We would do this as part of the test process,
// but we don't currently require using a bitcode-capable
// compiler to build these tools.

// RUN: alley %p/Inputs/dlopen |& FileCheck %s
// (See printf.c)
// XFAIL: *

#include <dlfcn.h>
#include <stdio.h>

const char *libc_path = "/lib/libc.so.6";

void try_load(const char *lib) {
  void *handle = dlopen(lib, RTLD_LAZY);
  printf("handle = %p\n", handle);
  if (!handle)
    printf("Error loading library: %s\n", dlerror());
  else
    printf("Library loaded successfully!\n");
  dlclose(handle);
}

int main(int argc, const char *argv[]) {

// CHECK-LABEL: Trying to get handle to self
  printf("Trying to get handle to self...\n");
  try_load(NULL);
// CHECK: handle = 0
// CHECK: Error loading library: Dynamic loading not supported

// CHECK-LABEL: Trying to load libc...
  printf("Trying to load libc...\n");
  try_load(libc_path);
// CHECK: handle = 0
// CHECK: Error loading library: Dynamic loading not supported

  return 0;
}
