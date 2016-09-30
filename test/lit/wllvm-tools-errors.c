// Construct a file not build by WLLVM:
// (We set WLLVM_CONFIGURE_ONLY JIC the available compiler /is/ WLLVM)
// RUN: WLLVM_CONFIGURE_ONLY=1 cc %s -o %t

// RUN: not wllvm-dump %t |& FileCheck %s -check-prefix CHECK-DUMP
// RUN: not wllvm-extract %t |& FileCheck %s -check-prefix CHECK-EXTRACT

// CHECK-DUMP: wllvm-dump: unable to find WLLVM section
// CHECK-EXTRACT: Error reading file:
// CHECK-EXTRACT-NEXT: wllvm-extract: unable to find WLLVM section

int main() { return 0; }

