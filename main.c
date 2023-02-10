#include "improg.h"
#include <stdio.h>

int main(int c, char const *v[]) {
  printf("hello\nworld\nmulti\nline\noutput\n");
  printf("\033[5F");
  return 0;
}
