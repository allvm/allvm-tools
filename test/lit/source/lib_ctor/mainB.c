#include <stdio.h>

extern void B();

int main() {

  printf("main B\n");

  B();

  printf("end main B\n");
}
