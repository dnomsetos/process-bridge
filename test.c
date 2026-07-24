#include <stdio.h>

static void print_yo() {
  printf("Yo!\n");
}

static void print_hello() {
  printf("Hello!\n");
}

static void print_hui() {
  printf("Hui!\n");
}

int main() {
  printf("Hello, World!\n");
  for (int i = 0; i < 5; ++i) {
    print_yo();
  }
  return 0;
}
