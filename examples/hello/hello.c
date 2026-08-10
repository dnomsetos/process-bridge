#include <stdio.h>

void repeat() {
  for (int i = 0; i < 5; ++i) {
    printf("hello\n");
  }
}

int main() {
  printf("before repeat\n");
  repeat();
  printf("after repeat\n");
}
