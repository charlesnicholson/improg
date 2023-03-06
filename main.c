#include "improg.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
  do { if ((CALLABLE) != IMP_RET_SUCCESS) { printf("error\n"); exit(1); } } while (0)

static void test_label(imp_ctx_t *ctx) {
  static const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("Label   : "),
    IMP_WIDGET_LABEL("[simple] "),
    IMP_WIDGET_LABEL("[complex 🐛🐛🐛🐛🐛 ∅🍺🍻🍷🍹💯]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);
  VERIFY_IMP(imp_draw_line(
    ctx, NULL, NULL, n, s_widgets, (imp_value_t const * const[]) { NULL, NULL }));
}

static void test_string(imp_ctx_t *ctx, double elapsed_s) {
  int const ml = (int)(float)roundf((float)elapsed_s);
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("String  : simple=["),
    IMP_WIDGET_STRING(-1, -1),
    IMP_WIDGET_LABEL("] fw=["),
    IMP_WIDGET_STRING(5, -1),
    IMP_WIDGET_LABEL("] ml=["),
    IMP_WIDGET_STRING(-1, 5),
    IMP_WIDGET_LABEL("] ml-clip=["),
    IMP_WIDGET_STRING(10, ml),
    IMP_WIDGET_LABEL("] null=["),
    IMP_WIDGET_STRING(-1, -1),
    IMP_WIDGET_LABEL("] ml-dynw=["),
    IMP_WIDGET_STRING(-1, 10 - ml),
    IMP_WIDGET_LABEL("]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  VERIFY_IMP(imp_draw_line(
    ctx, NULL, NULL, n, s_widgets, (imp_value_t const * const[]) {
      NULL,
      &(imp_value_t)IMP_VALUE_STRING("hello"),
      NULL,
      &(imp_value_t)IMP_VALUE_STRING("abc"),
      NULL,
      &(imp_value_t)IMP_VALUE_STRING("abcdefghijklmnop"),
      NULL,
      &(imp_value_t)IMP_VALUE_STRING("😀😃😄😁😆"),
      NULL,
      &(imp_value_t)IMP_VALUE_STRING(NULL),
      NULL,
      &(imp_value_t)IMP_VALUE_STRING("abcdefghijklmnop")
    }));
}

static void test_spinner(imp_ctx_t *ctx) {
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("Spinner : ascii=["),
    { .type = IMP_WIDGET_TYPE_SPINNER,
      .w = { .spinner = {
        .frames = (char const * const[]) { "1", "2", "3", "4", "5", "6", "7", "8" },
        .frame_count = 8,
        .speed_msec = 250,
      } } },
    IMP_WIDGET_LABEL("] uni-1w=["),
    { .type = IMP_WIDGET_TYPE_SPINNER,
      .w = { .spinner = {
        .frames = (char const * const[]) { "⡿", "⣟", "⣯", "⣷", "⣾", "⣽", "⣻", "⢿" },
        .frame_count = 8,
        .speed_msec = 100,
      } } },
    IMP_WIDGET_LABEL("] uni-2w=["),
    { .type = IMP_WIDGET_TYPE_SPINNER,
      .w = { .spinner = {
        .frames = (char const * const[]) { "😀", "😃", "😄", "😁", "😆", "😅" },
        .frame_count = 6,
        .speed_msec = 300,
      } } },
    IMP_WIDGET_LABEL("] uni-many-1w=["),
    { .type = IMP_WIDGET_TYPE_SPINNER,
      .w = { .spinner = {
        .frames = (char const * const[]) {
          "▱▱▱▱▱▱▱", "▰▱▱▱▱▱▱", "▰▰▱▱▱▱▱", "▰▰▰▱▱▱▱", "▰▰▰▰▱▱▱", "▰▰▰▰▰▱▱", "▰▰▰▰▰▰▱",
          "▰▰▰▰▰▰▰", },
        .frame_count = 8,
        .speed_msec = 200,
      } } },
    IMP_WIDGET_LABEL("] uni-many-2w=["),
    { .type = IMP_WIDGET_TYPE_SPINNER,
      .w = { .spinner = {
        .frames = (char const * const[]) {
          " 🧍⚽️       🧍", "🧍  ⚽️      🧍", "🧍   ⚽️     🧍", "🧍    ⚽️    🧍",
          "🧍     ⚽️   🧍", "🧍      ⚽️  🧍", "🧍       ⚽️🧍 ", "🧍      ⚽️  🧍",
          "🧍     ⚽️   🧍", "🧍    ⚽️    🧍", "🧍   ⚽️     🧍", "🧍  ⚽️      🧍",
        },
        .frame_count = 12,
        .speed_msec = 80,
      } } },
    IMP_WIDGET_LABEL("]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  VERIFY_IMP(imp_draw_line(
    ctx,
    NULL,
    NULL,
    n,
    s_widgets,
    (imp_value_t const * const[]) { NULL, NULL, NULL, NULL, NULL, NULL, NULL }));
}

static void test_percent(imp_ctx_t *ctx, double elapsed_s) {
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("P-Pct   : fw=["),
    IMP_WIDGET_PROGRESS_PERCENT(4, 0),
    IMP_WIDGET_LABEL("] fw-prec=["),
    IMP_WIDGET_PROGRESS_PERCENT(7, 2),
    IMP_WIDGET_LABEL("] prec-1=["),
    IMP_WIDGET_PROGRESS_PERCENT(5, 1),
    IMP_WIDGET_LABEL("] prec-3=["),
    IMP_WIDGET_PROGRESS_PERCENT(6, 3),
    IMP_WIDGET_LABEL("] no-prec=["),
    IMP_WIDGET_PROGRESS_PERCENT(-1, -1),
    IMP_WIDGET_LABEL("]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_DOUBLE(elapsed_s),
    &(imp_value_t)IMP_VALUE_DOUBLE(10.),
    n, s_widgets, (imp_value_t const * const[]) {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }));
}

static void test_progress_label(imp_ctx_t *ctx, double elapsed_s) {
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("P-Label : ascii=["),
    { .type = IMP_WIDGET_TYPE_PROGRESS_LABEL,
      .w = { .progress_label = {
        .labels = (imp_widget_progress_label_entry_t[]){
          { .s = "zero", .threshold = 0.1f },   { .s = "ten", .threshold = 0.2f },
          { .s = "twenty", .threshold = 0.3f }, { .s = "thirty", .threshold = 0.4f },
          { .s = "forty", .threshold = 0.5f },  { .s = "fifty", .threshold = 0.6f },
          { .s = "sixty", .threshold = 0.7f },  { .s = "seventy", .threshold = 0.8f },
          { .s = "eighty", .threshold = 0.9f }, { .s = "ninety", .threshold = 1.f },
          { .s = "one hundred", .threshold = 2.f },
        },
        .label_count = 11,
        .field_width = 11,
      } } },
    IMP_WIDGET_LABEL("] bool=["),
    { .type = IMP_WIDGET_TYPE_PROGRESS_LABEL,
      .w = { .progress_label = {
        .labels = (imp_widget_progress_label_entry_t[]) {
          { .s = "✗", .threshold = 1.f },
          { .s = "✓", .threshold = 2.f }
        },
        .label_count = 2,
        .field_width = -1
      } } },
    IMP_WIDGET_LABEL("] uni=["),
    { .type = IMP_WIDGET_TYPE_PROGRESS_LABEL,
      .w = { .progress_label = {
        .labels = (imp_widget_progress_label_entry_t[]) {
          { .s = "😐", .threshold = 0.1f }, { .s = "😐", .threshold = 0.2f },
          { .s = "😮", .threshold = 0.3f }, { .s = "😮", .threshold = 0.4f },
          { .s = "😦", .threshold = 0.5f }, { .s = "😦", .threshold = 0.6f },
          { .s = "😧", .threshold = 0.7f }, { .s = "😧", .threshold = 0.8f },
          { .s = "🤯", .threshold = 0.9f }, { .s = "💥", .threshold = 1.f },
          { .s = "✨", .threshold = 2.f },
        },
        .label_count = 11,
        .field_width = -1
      } } },
    IMP_WIDGET_LABEL("]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_DOUBLE(elapsed_s),
    &(imp_value_t)IMP_VALUE_DOUBLE(10.),
    n,
    s_widgets,
    (imp_value_t const * const[]) { NULL, NULL, NULL, NULL, NULL, NULL, NULL }));
}

static void test_scalar(imp_ctx_t *ctx, double elapsed_s) {
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("Scalar  : int=["),
    IMP_WIDGET_SCALAR(-1, -1),
    IMP_WIDGET_LABEL("] imax=["),
    IMP_WIDGET_SCALAR(-1, -1),
    IMP_WIDGET_LABEL("] fpos=["),
    IMP_WIDGET_SCALAR(-1, 9),
    IMP_WIDGET_LABEL("] fneg=["),
    IMP_WIDGET_SCALAR(-1, -1),
    IMP_WIDGET_LABEL("]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_DOUBLE(elapsed_s),
    &(imp_value_t)IMP_VALUE_DOUBLE(10.),
    n,
    s_widgets,
    (imp_value_t const * const[]) {
      NULL,
      &(imp_value_t)IMP_VALUE_INT(12345678),
      NULL,
      &(imp_value_t)IMP_VALUE_INT(9223372036854775807LL),
      NULL,
      &(imp_value_t)IMP_VALUE_DOUBLE(1234.567891011),
      NULL,
      &(imp_value_t)IMP_VALUE_DOUBLE(-1234.567891),
      NULL,
    }));
}

static void test_scalar_bytes(imp_ctx_t *ctx, double elapsed_s) {
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("Scalar  : b=["),
    IMP_WIDGET_SCALAR_UNIT(-1, -1, IMP_UNIT_SIZE_B),
    IMP_WIDGET_LABEL("] kb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_KB),
    IMP_WIDGET_LABEL("] mb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_MB),
    IMP_WIDGET_LABEL("] gb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_GB),
    IMP_WIDGET_LABEL("]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  int64_t const bytes = 1879048192LL;
  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_DOUBLE(elapsed_s),
    &(imp_value_t)IMP_VALUE_DOUBLE(10.),
    n,
    s_widgets,
    (imp_value_t const * const[]) {
      NULL,
      &(imp_value_t)IMP_VALUE_INT(bytes),
      NULL,
      &(imp_value_t)IMP_VALUE_INT(bytes),
      NULL,
      &(imp_value_t)IMP_VALUE_INT(bytes),
      NULL,
      &(imp_value_t)IMP_VALUE_INT(bytes),
      NULL,
    }));
}

static void test_scalar_bytes_fw(imp_ctx_t *ctx, double elapsed_s) {
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("Scalar  : b-fw=["),
    IMP_WIDGET_SCALAR_UNIT(12, -1, IMP_UNIT_SIZE_B),
    IMP_WIDGET_LABEL("] kb-fw=["),
    IMP_WIDGET_SCALAR_UNIT(13, 2, IMP_UNIT_SIZE_KB),
    IMP_WIDGET_LABEL("] mb-fw=["),
    IMP_WIDGET_SCALAR_UNIT(10, 2, IMP_UNIT_SIZE_MB),
    IMP_WIDGET_LABEL("] gb-fw=["),
    IMP_WIDGET_SCALAR_UNIT(7, 2, IMP_UNIT_SIZE_GB),
    IMP_WIDGET_LABEL("]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  int64_t const bytes = 1879048192LL;
  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_DOUBLE(elapsed_s),
    &(imp_value_t)IMP_VALUE_DOUBLE(10.),
    n,
    s_widgets,
    (imp_value_t const * const[]) {
      NULL,
      &(imp_value_t)IMP_VALUE_INT(bytes),
      NULL,
      &(imp_value_t)IMP_VALUE_INT(bytes),
      NULL,
      &(imp_value_t)IMP_VALUE_INT(bytes),
      NULL,
      &(imp_value_t)IMP_VALUE_INT(bytes),
      NULL,
    }));
}


static void test_scalar_bytes_dynamic(imp_ctx_t *ctx, double elapsed_s) {
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("Scalar  : dyn-b=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_DYNAMIC),
    IMP_WIDGET_LABEL("] dyn-kb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_DYNAMIC),
    IMP_WIDGET_LABEL("] dyn-mb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_DYNAMIC),
    IMP_WIDGET_LABEL("] dyn-gb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_DYNAMIC),
    IMP_WIDGET_LABEL("]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_DOUBLE(elapsed_s),
    &(imp_value_t)IMP_VALUE_DOUBLE(10.),
    n,
    s_widgets,
    (imp_value_t const * const[]) {
      NULL,
      &(imp_value_t)IMP_VALUE_INT(1023),
      NULL,
      &(imp_value_t)IMP_VALUE_INT(1048570),
      NULL,
      &(imp_value_t)IMP_VALUE_INT(1073741824LL - 10000),
      NULL,
      &(imp_value_t)IMP_VALUE_INT(1024LL * 1024 * 1024),
      NULL,
    }));
}


static void test_progress_scalar_int(imp_ctx_t *ctx, double elapsed_s) {
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("P-Scalar: int=["),
    IMP_WIDGET_PROGRESS_SCALAR(-1, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("] int-fw=["),
    IMP_WIDGET_PROGRESS_SCALAR(12, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_INT(elapsed_s * 100000.),
    &(imp_value_t)IMP_VALUE_INT(10. * 100000.),
    n,
    s_widgets,
    (imp_value_t const * const[]) { NULL, NULL, NULL, NULL, NULL, }));
}

static void test_progress_scalar_float(imp_ctx_t *ctx, double elapsed_s) {
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("P-Scalar: f-fw=["),
    IMP_WIDGET_PROGRESS_SCALAR(15, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("] f-fw-prec=["),
    IMP_WIDGET_PROGRESS_SCALAR(10, 2, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("] f=["),
    IMP_WIDGET_PROGRESS_SCALAR(-1, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("] f-prec=["),
    IMP_WIDGET_PROGRESS_SCALAR(-1, 1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_DOUBLE(elapsed_s * 100000.),
    &(imp_value_t)IMP_VALUE_DOUBLE(10. * 100000.),
    n,
    s_widgets,
    (imp_value_t const * const[]) { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }));
}

static void test_progress_fraction_int(imp_ctx_t *ctx, double elapsed_s) {
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("P-Frac  : int-fw=["),
    IMP_WIDGET_PROGRESS_FRACTION(25, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("] int=["),
    IMP_WIDGET_PROGRESS_FRACTION(-1, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("]"),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_INT(elapsed_s * 100000.),
    &(imp_value_t)IMP_VALUE_INT(10. * 100000.),
    n,
    s_widgets,
    (imp_value_t const * const[]) { NULL, NULL, NULL, NULL, NULL, }));
}

static void test_add_and_remove_lines(imp_ctx_t *ctx, double elapsed_s) {
  const imp_widget_def_t s_widgets[] = {
    IMP_WIDGET_LABEL("Add/Rem : "),
    IMP_WIDGET_SCALAR(-1, -1),
  };
  int const n = sizeof(s_widgets) / sizeof(*s_widgets);

  int const si = (int)elapsed_s;
  int const lines = 1 + (si < 6 ? si : -(si - 10));
  for (int i = 0; i < lines; ++i) {
    VERIFY_IMP(imp_draw_line(
      ctx,
      &(imp_value_t)IMP_VALUE_DOUBLE(elapsed_s * 100000.),
      &(imp_value_t)IMP_VALUE_DOUBLE(10. * 100000.),
      n,
      s_widgets,
      (imp_value_t const * const[]) { NULL, &(imp_value_t)IMP_VALUE_INT(i) }));
  }
}

/*
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
      .left_end = "🌎", .right_end = "🌑 ", .empty_fill = " ", .full_fill = "·",
      .edge_fill = &(imp_widget_def_t){
        .type=IMP_WIDGET_TYPE_PROGRESS_PERCENT,
        .w = { .progress_percent = { .field_width = 0, .precision = 0 } },
      },
      .field_width = -1 }
    } },
  { .type = IMP_WIDGET_TYPE_PROGRESS_PERCENT,
    .w = { .progress_percent = { .field_width = 6, .precision = 2 } } },
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
    .w = { .progress_percent = { .field_width = 4, .precision = 0 } } },
};

static char const *s_fns[] = { "foo.c", "bar.c", "baz.c" };
static int const s_bar_count[] = { 4, 4, 6, 7, 8, 8, 7, 6, 5, 4, 3, 2, 1 };
*/

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
    if (elapsed_s > 10.) { elapsed_s = 10.; }

    unsigned term_width = 50;
    imp_util_get_terminal_width(&term_width);

    VERIFY_IMP(imp_begin(&ctx, term_width, frame_time_ms));
    test_label(&ctx);
    test_scalar(&ctx, elapsed_s);
    test_scalar_bytes(&ctx, elapsed_s);
    test_scalar_bytes_fw(&ctx, elapsed_s);
    test_scalar_bytes_dynamic(&ctx, elapsed_s);
    test_string(&ctx, elapsed_s);
    test_spinner(&ctx);
    test_percent(&ctx, elapsed_s);
    test_progress_label(&ctx, elapsed_s);
    test_progress_scalar_int(&ctx, elapsed_s);
    test_progress_scalar_float(&ctx, elapsed_s);
    test_progress_fraction_int(&ctx, elapsed_s);
    test_add_and_remove_lines(&ctx, elapsed_s);
    test_label(&ctx);

    /*
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
    */
    VERIFY_IMP(imp_end(&ctx, done));
    msleep(frame_time_ms);
  } while (!done);
}

int main(int argc, char const *argv[]) {
  (void)argc; (void)argv;
  test_improg();
  return 0;
}
