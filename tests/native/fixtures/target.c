#include <unistd.h>

__attribute__((noinline, used)) void target_func(void) {
  write(STDOUT_FILENO, "reached\n", 8);
}

int main(void) {
  write(STDOUT_FILENO, "before\n", 7);
  target_func();
  write(STDOUT_FILENO, "after\n", 6);
  return 0;
}
