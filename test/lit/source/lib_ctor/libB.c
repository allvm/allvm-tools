
#include <stdio.h>

__attribute__((constructor)) void init() {
  printf("init B\n");
};

void B() {
  printf("B()\n");
}
