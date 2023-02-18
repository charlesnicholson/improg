#include "improg.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>

static double elapsed_sec_since(struct timespec const *start) {
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

static imp_widget_def_t const s_demo_bar_def[] = {
  (imp_widget_def_t) {
    .type = IMP_WIDGET_TYPE_STRING,
    .w = { .str = (imp_widget_string_t){ .field_width = -1 } }
  },
  (imp_widget_def_t) {
    .type = IMP_WIDGET_TYPE_LABEL,
    .w = { .label = (imp_widget_label_t){ .s = " improg " } }
  },
  (imp_widget_def_t) {
    .type = IMP_WIDGET_TYPE_SPINNER,
    .w = { .spinner = (imp_widget_spinner_t) {
      .frames = (char const * const[]){ "😀", "😃", "😄", "😁", "😆", "😅" },
      .frame_count = 6,
      .speed_msec = 250,
    } }
  },
  (imp_widget_def_t) {
    .type = IMP_WIDGET_TYPE_PROGRESS_BAR,
    .w = { .progress_bar = (imp_widget_progress_bar_t) {
      .left_end = " 🌎", .right_end = "🌑 ", .empty_fill = " ", .full_fill = "·",
      .edge_fill = &(imp_widget_def_t){
//          .type=IMP_WIDGET_TYPE_PROGRESS_PERCENT,
//          .w = { .percent = (imp_widget_progress_percent_t) {
//            .field_width = 0,
//            .precision = 0
//          } },
        .type=IMP_WIDGET_TYPE_LABEL,
        .w = { .label = (imp_widget_label_t) { .s="🚀🚀" } }
      },
      .field_width = -1 }
    }
  },
  (imp_widget_def_t) {
    .type = IMP_WIDGET_TYPE_PROGRESS_PERCENT,
    .w = { .percent = (imp_widget_progress_percent_t){ .field_width = 6, .precision = 2} }
  },
  (imp_widget_def_t) {
    .type = IMP_WIDGET_TYPE_LABEL,
    .w = { .label = (imp_widget_label_t){ .s = " 🚀 " } }
  },
  (imp_widget_def_t) {
    .type = IMP_WIDGET_TYPE_PROGRESS_LABEL,
    .w = { .progress_label = (imp_widget_progress_label_t) {
      .labels = (imp_widget_progress_label_entry_t[]){
        (imp_widget_progress_label_entry_t){ .s = "liftoff*", .threshold = 0.3f },
        (imp_widget_progress_label_entry_t){ .s = "going..*", .threshold = 0.999f },
        (imp_widget_progress_label_entry_t){ .s = "gone!!!*", .threshold = 1.0f },
      },
      .label_count = 3,
    } }
  }
};

static int const s_bar_count[] = { 4, 4, 6, 7, 8, 8, 7, 6, 5, 4, 3, 2, 1 };
static void test_improg(void) {
  imp_ctx_t ctx;
  VERIFY_IMP(imp_init(&ctx, NULL, NULL));

  struct timespec start;
  timespec_get(&start, TIME_UTC);

  double elapsed_s = 0;
  bool done = false;

  unsigned const frame_time_ms = 50;

  do {
    elapsed_s = elapsed_sec_since(&start);
    done = elapsed_s >= 10.;
    unsigned term_width = 50;
    imp_util_get_terminal_width(&term_width);
    int const bars = s_bar_count[(int)elapsed_s];
    //int const bars = ((int)elapsed_s + 1) > 4 ? 4 : ((int)elapsed_s + 1);

    VERIFY_IMP(imp_begin(&ctx, term_width, frame_time_ms));

    for (int i = 0; i < bars; ++i) {
      VERIFY_IMP(imp_draw_line(
        &ctx,
        &(imp_value_t) { .type = IMP_VALUE_TYPE_DOUBLE, .v.d = elapsed_s - i },
        &(imp_value_t) { .type = IMP_VALUE_TYPE_DOUBLE, .v.d = 10. },
        s_demo_bar_def,
        sizeof(s_demo_bar_def) / sizeof(*s_demo_bar_def),
        (imp_value_t[]) {
          (imp_value_t) {
            .type = IMP_VALUE_TYPE_STR,
            .v = { .s = elapsed_s < 2.5 ? "hello   😊" : "goodbye 🙁" }
          },
        },
        1));
    }

    VERIFY_IMP(imp_end(&ctx, done));
    usleep(frame_time_ms * 1000);
  } while (!done);
}

int main(int argc, char const *argv[]) {
  (void)argc; (void)argv;

  unsigned tw;
  if (imp_util_get_terminal_width(&tw)) {
    printf("terminal width: %d\n", tw);
  } else {
    printf("terminal width: unavailable\n");
  }
  test_improg();

  return 0;
}
