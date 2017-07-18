// RUN: alley %p/Inputs/dlsym |& FileCheck %s

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

void try_dlsym(void *handle, const char *sym) {
  char *dl;
  void *addr;
  dl = dlerror();
  if (dl) {
    printf("error before: %s\n", dl);
    exit(-1);
  }

  addr = dlsym(handle, sym);
  printf("addr: %p\n", addr);

  dl = dlerror();
  if (dl) {
    printf("error after: %s\n", dl);
  } else {
    printf("dlsym succeeded!\n");
    exit(-2);
  }
}

int main() {

// CHECK-LABEL: Testing dlsym(RTLD_DEFAULT
  printf("Testing dlsym(RTLD_DEFAULT,...)\n");
  try_dlsym(RTLD_DEFAULT, "main");
// CHECK-NOT: error before
// CHECK: addr: 0
// CHECK: error after: Symbol not found
// CHECK-NOT: dlsym succeeded

// CHECK-LABEL: Testing dlsym(RTLD_NEXT
  printf("Testing dlsym(RTLD_NEXT,...)\n");
  try_dlsym(RTLD_NEXT, "main");
// CHECK-NOT: error before
// CHECK: addr: 0
// CHECK: error after: Symbol not found
// CHECK-NOT: dlsym succeeded

  return 0;
}
