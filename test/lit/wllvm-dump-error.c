// RUN: cc %s -o %t
// RUN: not wllvm-dump %t |& FileCheck %s

// CHECK: wllvm-dump: unable to find WLLVM section

int main() { return 0; }

