#include "improg.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>

double elapsed_sec_since(struct timespec const *start) {
  struct timespec now;
  timespec_get(&now, TIME_UTC);

  double const elapsed_msec =
    1000.0*now.tv_sec + 1e-6*now.tv_nsec - (1000.0*start->tv_sec + 1e-6*start->tv_nsec);

  return elapsed_msec / 1000.;
}

int main(int c, char const *v[]) {
  printf("%s", IMP_ESC IMP_CSI IMP_DECTCEM IMP_HIDECURSOR);

  struct timespec start;
  timespec_get(&start, TIME_UTC);
  while (elapsed_sec_since(&start) < 5.0) {
    if (elapsed_sec_since(&start) < 2.5) {
      printf("hello" IMP_FULL_ERASE_CURSOR_TO_END
             "\nworld" IMP_FULL_ERASE_CURSOR_TO_END
             "\nmulti" IMP_FULL_ERASE_CURSOR_TO_END
             "\nline" IMP_FULL_ERASE_CURSOR_TO_END
             "\noutput" IMP_FULL_ERASE_CURSOR_TO_END
             "\n");
    } else {
      printf("here" IMP_FULL_ERASE_CURSOR_TO_END
             "\nare" IMP_FULL_ERASE_CURSOR_TO_END
             "\nsome" IMP_FULL_ERASE_CURSOR_TO_END
             "\nshort" IMP_FULL_ERASE_CURSOR_TO_END
             "\nlines" IMP_FULL_ERASE_CURSOR_TO_END
             "\n");
    }
    printf("elapsed: %.2fs\n", elapsed_sec_since(&start));
    printf(IMP_FULL_PREVLINE, 6);
    usleep(1000 * 16);
  }

  printf("%s", IMP_ESC IMP_CSI IMP_DECTCEM IMP_SHOWCURSOR);
  return 0;
}
