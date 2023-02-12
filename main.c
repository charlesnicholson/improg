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

#define VERIFY_IMP(CALLABLE) \
  do { \
    if ((CALLABLE) != IMP_RET_SUCCESS) { \
      printf("error\n"); \
      return; \
    } \
  } while (0)

void test_improg(void) {
  imp_ctx_t ctx;
  VERIFY_IMP(imp_init(&ctx, NULL, NULL));

  struct timespec start;
  timespec_get(&start, TIME_UTC);

  while (elapsed_sec_since(&start) < 5.0) {
    VERIFY_IMP(imp_begin(&ctx, 50, (unsigned)(1.f/60.f)));
    VERIFY_IMP(imp_drawline(&ctx,
                            (imp_widget_def_t[]) {
                              (imp_widget_def_t) {
                                .type = IMP_WIDGET_TYPE_LABEL,
                                .w = { (imp_widget_label_t) {
                                    .s = "hello improg",
                                    .display_width = -1,
                                }}
                              },
                            },
                            1));
    VERIFY_IMP(imp_end(&ctx));
    usleep(1000 * 16);
  }
}

int main(int argc, char const *argv[]) {
  (void)argc; (void)argv;

  test_improg();

  printf(IMP_FULL_HIDE_CURSOR);

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

  printf(IMP_FULL_SHOW_CURSOR);
  return 0;
}
