#include "improg.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static int msleep(unsigned msec) {
  struct timespec ts = { .tv_sec = msec / 1000, .tv_nsec = (msec % 1000) * 1000000 };
  int res;
  do { res = nanosleep(&ts, &ts); } while (res && (errno == EINTR));
  return res;
}

static double elapsed_sec_since(struct timespec const *start) {
  struct timespec now;
  timespec_get(&now, TIME_UTC);
  double const elapsed_msec =
    ((1000.0 * (double)now.tv_sec) + (1e-6 * (double)now.tv_nsec)) -
    ((1000.0 * (double)start->tv_sec) + (1e-6 * (double)start->tv_nsec));

  return elapsed_msec / 1000.;
}

#define VERIFY_IMP(CALLABLE) \
  do { \
    if ((CALLABLE) != IMP_RET_SUCCESS) { \
      printf("error\n"); \
      return; \
    } \
  } while (0)

static void test_label(imp_ctx_t *ctx) {
  static const imp_widget_def_t s_widgets[] = {
    { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = " Label test: " } } },
    { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = "[simple] " } } },
    { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = "[complex ∅🍺🍻🍷🍹💯]" } } }
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);
  VERIFY_IMP(imp_draw_line(
    ctx, NULL, NULL, n, s_widgets, (imp_value_t const * const[]) { NULL, NULL }));
}

static void test_string(imp_ctx_t *ctx, double elapsed_s) {
  int const ml = (int)(float)roundf(fmodf((float)elapsed_s, 10.f));
  const imp_widget_def_t s_widgets[] = {
    { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = "String test: one=[" } } },
    { .type = IMP_WIDGET_TYPE_STRING, .w = { .str = { .field_width = -1, .max_len = -1 } } },
    { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = "] two=["} } },
    { .type = IMP_WIDGET_TYPE_STRING, .w = { .str = { .field_width = -1, .max_len = -1 } } },
    { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = "] fw=["} } },
    { .type = IMP_WIDGET_TYPE_STRING, .w = { .str = { .field_width = 5, .max_len = -1 } } },
    { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = "] ml=["} } },
    { .type = IMP_WIDGET_TYPE_STRING, .w = { .str = { .field_width = -1, .max_len = 5 } } },
    { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = "] 2-wide=["} } },
    { .type = IMP_WIDGET_TYPE_STRING, .w = { .str = { .field_width = 10, .max_len = ml } } },
    { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = "]"} } },
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  static char const *s_two[] = { "first ", "second" };
  int const two_idx = (int)(float)fmodf((float)elapsed_s, 2.f);

  VERIFY_IMP(imp_draw_line(
    ctx, NULL, NULL, n, s_widgets, (imp_value_t const * const[]) {
      NULL,
      &(imp_value_t) { .type = IMP_VALUE_TYPE_STR, .v = { .s = "hello" } },
      NULL,
      &(imp_value_t) { .type = IMP_VALUE_TYPE_STR, .v = { .s = s_two[two_idx] } },
      NULL,
      &(imp_value_t) { .type = IMP_VALUE_TYPE_STR, .v = { .s = "abc" } },
      NULL,
      &(imp_value_t) { .type = IMP_VALUE_TYPE_STR, .v = { .s = "abcdefghijklmnop" } },
      NULL,
      &(imp_value_t) { .type = IMP_VALUE_TYPE_STR, .v = { .s = "😀😃😄😁😆" } },
    }));
}

static imp_widget_def_t const s_demo_bar1_def[] = {
  { .type = IMP_WIDGET_TYPE_STRING, .w = { .str = { .field_width = 12, .max_len = -1 } } },
  { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = "improg " } } },
  { .type = IMP_WIDGET_TYPE_SPINNER,
    .w = { .spinner = {
      .frames = (char const * const[]){ "😀", "😃", "😄", "😁", "😆", "😅" },
      .frame_count = 6,
      .speed_msec = 250,
    } } },
  { .type = IMP_WIDGET_TYPE_PROGRESS_BAR,
    .w = { .progress_bar = {
      .left_end = " ∅" /*"🌎"*/, .right_end = "💯 "/*"🌑 "*/, .empty_fill = " ", .full_fill = "·",
      .edge_fill = &(imp_widget_def_t){
        .type=IMP_WIDGET_TYPE_PROGRESS_PERCENT,
        .w = { .percent = { .field_width = 0, .precision = 0 } },
      },
      .field_width = -1 }
    } },
  { .type = IMP_WIDGET_TYPE_PROGRESS_PERCENT,
    .w = { .percent = { .field_width = 6, .precision = 2 } } },
  { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = " 🚀 " } } },
  { .type = IMP_WIDGET_TYPE_PROGRESS_LABEL,
    .w = { .progress_label = {
      .labels = (imp_widget_progress_label_entry_t[]){
        { .s = "liftoff*", .threshold = 0.3f },
        { .s = "going..*", .threshold = 0.999f },
        { .s = "gone!!!*", .threshold = 1.0f },
      },
      .label_count = 3,
    } } }
};

static imp_widget_def_t const s_demo_bar2_def[] = {
  { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = "Compiling " } } },
  { .type = IMP_WIDGET_TYPE_STRING, .w = { .str = { .field_width = -1, .max_len = -1 } } },
  { .type = IMP_WIDGET_TYPE_PROGRESS_BAR,
    .w = { .progress_bar = {
      .left_end = " [", .right_end = "] ", .empty_fill = " ", .full_fill = "⨯",
      .edge_fill = &(imp_widget_def_t) {
        .type = IMP_WIDGET_TYPE_SPINNER,
        .w = { .spinner = {
          .frames = (char const * const[]){ "🍺", "🍻", "🍷", "🍹" },
          .frame_count = 4,
          .speed_msec = 300
        } }
      },
      .field_width = -1
    } }, },
  { .type = IMP_WIDGET_TYPE_PROGRESS_PERCENT,
    .w = { .percent = { .field_width = 4, .precision = 0 } } },
};


static char const *s_fns[] = { "foo.c", "bar.c", "baz.c" };
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

    VERIFY_IMP(imp_begin(&ctx, term_width, frame_time_ms));
    test_label(&ctx);
    test_string(&ctx, elapsed_s);

    VERIFY_IMP(imp_draw_line(
      &ctx,
      &(imp_value_t) { .type = IMP_VALUE_TYPE_DOUBLE, .v.d = elapsed_s },
      &(imp_value_t) { .type = IMP_VALUE_TYPE_DOUBLE, .v.d = 10. },
      sizeof(s_demo_bar2_def) / sizeof(*s_demo_bar2_def),
      s_demo_bar2_def,
      (imp_value_t const * const[]) {
        NULL,
        &(imp_value_t) {
          .type = IMP_VALUE_TYPE_STR,
          .v = {
            .s = s_fns[(int)(float)fmodf((float)elapsed_s, sizeof(s_fns) / sizeof(*s_fns))]
          }
        },
        NULL,
      }
    ));

    for (int i = 0; i < bars; ++i) {
      VERIFY_IMP(imp_draw_line(
        &ctx,
        &(imp_value_t) { .type = IMP_VALUE_TYPE_DOUBLE, .v.d = elapsed_s - i },
        &(imp_value_t) { .type = IMP_VALUE_TYPE_DOUBLE, .v.d = 10. },
        sizeof(s_demo_bar1_def) / sizeof(*s_demo_bar1_def),
        s_demo_bar1_def,
        (imp_value_t const * const[]) {
          &(imp_value_t) {
            .type = IMP_VALUE_TYPE_STR,
            .v = { .s = elapsed_s < 2.5 ? "hello🌎" : "🌎goodbye" }
          },
          NULL,
          NULL,
          NULL,
          NULL,
          NULL,
          NULL,
        }
      ));
    }

    VERIFY_IMP(imp_end(&ctx, done));
    msleep(frame_time_ms);
  } while (!done);
}

int main(int argc, char const *argv[]) {
  (void)argc; (void)argv;
  test_improg();
  return 0;
}
