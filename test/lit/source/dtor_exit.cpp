
#include <stdio.h>
#include <stdlib.h>

struct D {
  D() {
    printf("ctor D::D()\n");
  }
  ~D() {
    printf("dtor D::~D()\n");
  }
};

static D d;

__attribute__((constructor)) static void init_attr() {
  printf("ctor init_attr()\n");
}

__attribute__((destructor)) static void fini_attr() {
  printf("dtor fini_attr()\n");
}

int main(int argc, char**argv) {
  printf("main\n");
  if (argc > 1)
    exit(0);
  return 0;
}
