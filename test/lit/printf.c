// (This is the source for Inputs/printf;
//  manually built until we require building w/clang)
// RUN: alley %p/Inputs/printf |& FileCheck %s
// XFAIL: *

#include <stdio.h>

int main() {
// CHECK: Yolo
  printf("Yolo\n");
// CHECK-NEXT: Octo
  printf("Octo\n");
// CHECK-NEXT: Danger
  printf("Danger\n");
// CHECK-NEXT: Zone
  printf("Zone\n");

  return 0;
}
