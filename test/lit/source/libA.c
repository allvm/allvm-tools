
#include <stdio.h>

__attribute__((constructor)) void init() {
  printf("init A\n");
};

void A() {
  printf("A()\n");
}
