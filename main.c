#include "improg.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
static int msleep(unsigned msec) {
  Sleep(msec);
  return 0;
}
#else
static int msleep(unsigned msec) {
  struct timespec ts = { .tv_sec = msec / 1000, .tv_nsec = (msec % 1000) * 1000000 };
  int res;
  do { res = nanosleep(&ts, &ts); } while (res && (errno == EINTR));
  return res;
}
#endif

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
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 3, IMP_ARRAY(
    IMP_WIDGET_LABEL("Label   : "),
    IMP_WIDGET_LABEL("[simple] "),
    IMP_WIDGET_LABEL("[complex 🐛🐛🐛🐛🐛 ∅🍺🍻🍷🍹💯]")));

  imp_value_t const v =
    IMP_VALUE_COMPOSITE(3, IMP_ARRAY(IMP_VALUE_NULL(), IMP_VALUE_NULL(), IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_scalar(imp_ctx_t *ctx) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 9, IMP_ARRAY(
    IMP_WIDGET_LABEL("Scalar  : int=["),
    IMP_WIDGET_SCALAR(-1, -1),
    IMP_WIDGET_LABEL("] imax=["),
    IMP_WIDGET_SCALAR(-1, -1),
    IMP_WIDGET_LABEL("] fpos=["),
    IMP_WIDGET_SCALAR(-1, 9),
    IMP_WIDGET_LABEL("] fneg=["),
    IMP_WIDGET_SCALAR(-1, -1),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(9, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(12345678),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(9223372036854775807LL),
    IMP_VALUE_NULL(),
    IMP_VALUE_DOUBLE(1234.567891011),
    IMP_VALUE_NULL(),
    IMP_VALUE_DOUBLE(-1234.567891),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_scalar_bytes(imp_ctx_t *ctx) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 9, IMP_ARRAY(
    IMP_WIDGET_LABEL("Scalar  : b=["),
    IMP_WIDGET_SCALAR_UNIT(-1, -1, IMP_UNIT_SIZE_B),
    IMP_WIDGET_LABEL("] kb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_KB),
    IMP_WIDGET_LABEL("] mb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_MB),
    IMP_WIDGET_LABEL("] gb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_GB),
    IMP_WIDGET_LABEL("]")));

  int64_t const bytes = 1879048192LL;
  imp_value_t const v = IMP_VALUE_COMPOSITE(9, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(bytes),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(bytes),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(bytes),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(bytes),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_scalar_bytes_fw(imp_ctx_t *ctx) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 9, IMP_ARRAY(
    IMP_WIDGET_LABEL("Scalar  : b-fw=["),
    IMP_WIDGET_SCALAR_UNIT(12, -1, IMP_UNIT_SIZE_B),
    IMP_WIDGET_LABEL("] kb-fw=["),
    IMP_WIDGET_SCALAR_UNIT(13, 2, IMP_UNIT_SIZE_KB),
    IMP_WIDGET_LABEL("] mb-fw=["),
    IMP_WIDGET_SCALAR_UNIT(10, 2, IMP_UNIT_SIZE_MB),
    IMP_WIDGET_LABEL("] gb-fw=["),
    IMP_WIDGET_SCALAR_UNIT(7, 2, IMP_UNIT_SIZE_GB),
    IMP_WIDGET_LABEL("]")));

  int64_t const bytes = 1879048192LL;
  imp_value_t const v = IMP_VALUE_COMPOSITE(9, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(bytes),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(bytes),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(bytes),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(bytes),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_scalar_bytes_dynamic(imp_ctx_t *ctx) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 9, IMP_ARRAY(
    IMP_WIDGET_LABEL("Scalar  : dyn-b=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_DYNAMIC),
    IMP_WIDGET_LABEL("] dyn-kb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_DYNAMIC),
    IMP_WIDGET_LABEL("] dyn-mb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_DYNAMIC),
    IMP_WIDGET_LABEL("] dyn-gb=["),
    IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_DYNAMIC),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(9, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(1023),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(1048570),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(1073741824LL - 10000),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(1024LL * 1024 * 1024),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_scalar_time(imp_ctx_t *ctx) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 11, IMP_ARRAY(
    IMP_WIDGET_LABEL("Scalar  : sec=["),
    IMP_WIDGET_SCALAR_UNIT(-1, -1, IMP_UNIT_TIME_SEC),
    IMP_WIDGET_LABEL("] hms-colons=["),
    IMP_WIDGET_SCALAR_UNIT(-1, -1, IMP_UNIT_TIME_HMS_COLONS),
    IMP_WIDGET_LABEL("] hms-s=["),
    IMP_WIDGET_SCALAR_UNIT(-1, -1, IMP_UNIT_TIME_HMS_LETTERS),
    IMP_WIDGET_LABEL("] hms-m-s=["),
    IMP_WIDGET_SCALAR_UNIT(-1, -1, IMP_UNIT_TIME_HMS_LETTERS),
    IMP_WIDGET_LABEL("] hms=["),
    IMP_WIDGET_SCALAR_UNIT(-1, -1, IMP_UNIT_TIME_HMS_LETTERS),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(11, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(8424),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(8424),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(59),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(3599),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(3660),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_scalar_time_fw(imp_ctx_t *ctx) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 7, IMP_ARRAY(
    IMP_WIDGET_LABEL("Scalar  : fw-sec=["),
    IMP_WIDGET_SCALAR_UNIT(6, -1, IMP_UNIT_TIME_SEC),
    IMP_WIDGET_LABEL("] hms-letters=["),
    IMP_WIDGET_SCALAR_UNIT(9, -1, IMP_UNIT_TIME_HMS_LETTERS),
    IMP_WIDGET_LABEL("] hms-colons=["),
    IMP_WIDGET_SCALAR_UNIT(9, -1, IMP_UNIT_TIME_HMS_COLONS),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(7, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(8424),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(8424),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(8424),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_string(imp_ctx_t *ctx, double elapsed_s) {
  int const ml = (int)(float)roundf((float)elapsed_s);

  const imp_widget_def_t w = IMP_WIDGET_COMPOSITE(-1, 13, IMP_ARRAY(
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
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(13, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("hello"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("abc"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("abcdefghijklmnop"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("😀😃😄😁😆"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING(NULL),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("abcdefghijklmnop")));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_string_trim(imp_ctx_t *ctx) {
  const imp_widget_def_t w = IMP_WIDGET_COMPOSITE(-1, 13, IMP_ARRAY(
    IMP_WIDGET_LABEL("String  : rt=["),
    IMP_WIDGET_STRING(-1, 6),
    IMP_WIDGET_LABEL("] lt=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(-1, 6, NULL, true),
    IMP_WIDGET_LABEL("] rtdot=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(-1, 9, "···", false),
    IMP_WIDGET_LABEL("] ltdot=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(-1, 9, "···", true),
    IMP_WIDGET_LABEL("] rtiny=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(-1, 2, "···", false),
    IMP_WIDGET_LABEL("] ltiny=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(-1, 2, "···", true),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(13, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_string_trim_fw(imp_ctx_t *ctx) {
  const imp_widget_def_t w = IMP_WIDGET_COMPOSITE(-1, 13, IMP_ARRAY(
    IMP_WIDGET_LABEL("String  : rt-fw=["),
    IMP_WIDGET_STRING(7, 6),
    IMP_WIDGET_LABEL("] lt-fw=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(7, 6, NULL, true),
    IMP_WIDGET_LABEL("] rtd-fw=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(10, 9, "···", false),
    IMP_WIDGET_LABEL("] ltd-fw=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(10, 9, "···", true),
    IMP_WIDGET_LABEL("] rfw=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(3, 2, "···", false),
    IMP_WIDGET_LABEL("] lfw=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(3, 2, "···", true),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(13, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L1234554321R"),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_string_trim_unicode(imp_ctx_t *ctx) {
  const imp_widget_def_t w = IMP_WIDGET_COMPOSITE(-1, 13, IMP_ARRAY(
    IMP_WIDGET_LABEL("String  : rt-fw=["),
    IMP_WIDGET_STRING(7, 6),
    IMP_WIDGET_LABEL("] lt-fw=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(7, 6, NULL, true),
    IMP_WIDGET_LABEL("] rtd-fw=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(10, 9, "···", false),
    IMP_WIDGET_LABEL("] ltd-fw=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(10, 9, "···", true),
    IMP_WIDGET_LABEL("] rfw=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(3, 2, "···", false),
    IMP_WIDGET_LABEL("] lfw=["),
    IMP_WIDGET_STRING_CUSTOM_TRIM(3, 2, "···", true),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(13, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L😀😀😀😀😀😀😀R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L😀😀😀😀😀😀😀R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L😀😀😀😀😀😀😀R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L😀😀😀😀😀😀😀R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L😀😀😀😀😀😀😀R"),
    IMP_VALUE_NULL(),
    IMP_VALUE_STRING("L😀😀😀😀😀😀😀R"),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_spinner(imp_ctx_t *ctx, int elapsed_msec) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 11, IMP_ARRAY(
    IMP_WIDGET_LABEL("Spinner : ascii=["),
    IMP_WIDGET_SPINNER(250, 8, IMP_ARRAY("1", "2", "3", "4", "5", "6", "7", "8")),
    IMP_WIDGET_LABEL("] uni-1w=["),
    IMP_WIDGET_SPINNER(100, 8, IMP_ARRAY("⡿", "⣟", "⣯", "⣷", "⣾", "⣽", "⣻", "⢿")),
    IMP_WIDGET_LABEL("] uni-2w=["),
    IMP_WIDGET_SPINNER(300, 6, IMP_ARRAY("😀", "😃", "😄", "😁", "😆", "😅")),
    IMP_WIDGET_LABEL("] uni-many-1w=["),
    IMP_WIDGET_SPINNER(200, 8, IMP_ARRAY("▱▱▱▱▱▱▱", "▰▱▱▱▱▱▱", "▰▰▱▱▱▱▱", "▰▰▰▱▱▱▱",
      "▰▰▰▰▱▱▱", "▰▰▰▰▰▱▱", "▰▰▰▰▰▰▱", "▰▰▰▰▰▰▰")),
    IMP_WIDGET_LABEL("] uni-many-2w=["),
    IMP_WIDGET_SPINNER(80, 12, IMP_ARRAY(" 🧍⚽️       🧍", "🧍  ⚽️      🧍",
      "🧍   ⚽️     🧍", "🧍    ⚽️    🧍", "🧍     ⚽️   🧍", "🧍      ⚽️  🧍",
      "🧍       ⚽️🧍 ", "🧍      ⚽️  🧍", "🧍     ⚽️   🧍", "🧍    ⚽️    🧍",
      "🧍   ⚽️     🧍", "🧍  ⚽️      🧍")),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(11, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(elapsed_msec),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(elapsed_msec),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(elapsed_msec),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(elapsed_msec),
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(elapsed_msec),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(ctx, NULL, NULL, &w, &v));
}

static void test_percent(imp_ctx_t *ctx, double elapsed_s) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 11, IMP_ARRAY(
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
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(11, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_DOUBLE(elapsed_s),
    &(imp_value_t)IMP_VALUE_DOUBLE(10.),
    &w,
    &v));
}

static void test_progress_label(imp_ctx_t *ctx, double elapsed_s) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 7, IMP_ARRAY(
    IMP_WIDGET_LABEL("P-Label : ascii=[" IMP_COLOR_FG_MAGENTA_BRIGHT),
    IMP_WIDGET_PROGRESS_LABEL(11, 11, IMP_ARRAY(
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.1f, "zero"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.2f, "ten"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.3f, "twenty"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.4f, "thirty"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.5f, "forty"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.6f, "fifty"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.7f, "sixty"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.8f, "seventy"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.9f, "eighty"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(1.f, "ninety"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(2.f, "one hundred"),
    )),
    IMP_WIDGET_LABEL(IMP_COLOR_RESET "] bool=["),
    IMP_WIDGET_PROGRESS_LABEL(-1, 2, IMP_ARRAY(
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(1.f, IMP_COLOR_FG_RED_BRIGHT "✗" IMP_COLOR_RESET),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(2.f, IMP_COLOR_FG_GREEN_BRIGHT "✓" IMP_COLOR_RESET),
    )),
    IMP_WIDGET_LABEL("] uni=["),
    IMP_WIDGET_PROGRESS_LABEL(-1, 11, IMP_ARRAY(
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.1f, "😐"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.2f, "😐"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.3f, "😮"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.4f, "😮"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.5f, "😦"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.6f, "😦"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.7f, "😧"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.8f, "😧"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.9f, "🤯"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(1.f, "💥"),
      IMP_WIDGET_PROGRESS_LABEL_ENTRY(2.f, "✨"),
    )),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(7, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_DOUBLE(elapsed_s),
    &(imp_value_t)IMP_VALUE_DOUBLE(10.),
    &w,
    &v));
}

static void test_progress_scalar_int(imp_ctx_t *ctx, double elapsed_s) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 5, IMP_ARRAY(
    IMP_WIDGET_LABEL("P-Scalar: int=["),
    IMP_WIDGET_PROGRESS_SCALAR(-1, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("] int-fw=["),
    IMP_WIDGET_PROGRESS_SCALAR(12, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(5, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_INT(elapsed_s * 100000.),
    &(imp_value_t)IMP_VALUE_INT(10. * 100000.),
    &w,
    &v));
}

static void test_progress_scalar_float(imp_ctx_t *ctx, double elapsed_s) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 9, IMP_ARRAY(
    IMP_WIDGET_LABEL("P-Scalar: f-fw=["),
    IMP_WIDGET_PROGRESS_SCALAR(15, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("] f-fw-prec=["),
    IMP_WIDGET_PROGRESS_SCALAR(10, 2, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("] f=["),
    IMP_WIDGET_PROGRESS_SCALAR(-1, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("] f-prec=["),
    IMP_WIDGET_PROGRESS_SCALAR(-1, 1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(9, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_DOUBLE(elapsed_s * 100000.),
    &(imp_value_t)IMP_VALUE_DOUBLE(10. * 100000.),
    &w,
    &v));
}

static void test_progress_fraction_int(imp_ctx_t *ctx, double elapsed_s) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 5, IMP_ARRAY(
    IMP_WIDGET_LABEL("P-Frac  : int-fw=["),
    IMP_WIDGET_PROGRESS_FRACTION(25, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("] int=["),
    IMP_WIDGET_PROGRESS_FRACTION(-1, -1, IMP_UNIT_NONE),
    IMP_WIDGET_LABEL("]")));

  imp_value_t const v = IMP_VALUE_COMPOSITE(5, IMP_ARRAY(
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL()));

  VERIFY_IMP(imp_draw_line(
    ctx,
    &(imp_value_t)IMP_VALUE_INT(elapsed_s * 100000.),
    &(imp_value_t)IMP_VALUE_INT(10. * 100000.),
    &w,
    &v));
}

static void test_progress_bar(imp_ctx_t *ctx, double elapsed_s) {
  imp_value_t const cur_prog = IMP_VALUE_INT(elapsed_s * 100000.);
  imp_value_t const max_prog = IMP_VALUE_INT(10. * 100000.);
  imp_value_t const es_val = IMP_VALUE_DOUBLE(elapsed_s);
  imp_value_t const ems_val = IMP_VALUE_INT(elapsed_s * 1000.);

  {
    imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 4, IMP_ARRAY(
      IMP_WIDGET_LABEL("P-Bar   : short="),
      IMP_WIDGET_PROGRESS_BAR(10, "[", "]", "=", " ",
        &(imp_widget_def_t)IMP_WIDGET_LABEL(">")),
      IMP_WIDGET_LABEL(" long="),
      IMP_WIDGET_PROGRESS_BAR(55, "[", "]", "=", " ",
        &(imp_widget_def_t)IMP_WIDGET_LABEL(">"))));

    imp_value_t const v = IMP_VALUE_COMPOSITE(4,
      IMP_ARRAY(IMP_VALUE_NULL(), IMP_VALUE_NULL(), IMP_VALUE_NULL(), IMP_VALUE_NULL()));

    VERIFY_IMP(imp_draw_line(ctx, &cur_prog, &max_prog, &w, &v));
  }

  {
    imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 2, IMP_ARRAY(
      IMP_WIDGET_LABEL("P-Bar   : fill="),
      IMP_WIDGET_PROGRESS_BAR(-1, "[", "]", "=", " ",
        &(imp_widget_def_t)IMP_WIDGET_LABEL(">"))));

    imp_value_t const v =
      IMP_VALUE_COMPOSITE(2, IMP_ARRAY(IMP_VALUE_NULL(), IMP_VALUE_NULL()));

    VERIFY_IMP(imp_draw_line(ctx, &cur_prog, &max_prog, &w, &v));
  }

  {
    imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 3, IMP_ARRAY(
      IMP_WIDGET_LABEL("P-Bar   : fill-pct="),
      IMP_WIDGET_PROGRESS_BAR(-1, "[", "]", "=", " ",
        &(imp_widget_def_t)IMP_WIDGET_LABEL(">")),
      IMP_WIDGET_PROGRESS_PERCENT(8, 2)));

    imp_value_t const v = IMP_VALUE_COMPOSITE(3,
      IMP_ARRAY(IMP_VALUE_NULL(), IMP_VALUE_NULL(), IMP_VALUE_NULL()));

    VERIFY_IMP(imp_draw_line(ctx, &cur_prog, &max_prog, &w, &v));
  }

  {
    imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 6, IMP_ARRAY(
      IMP_WIDGET_LABEL("P-Bar   : fill-lots="),
      IMP_WIDGET_PROGRESS_BAR(-1, "[", "]", "=", " ",
        &(imp_widget_def_t)IMP_WIDGET_LABEL(">")),
      IMP_WIDGET_PROGRESS_FRACTION(20, 2, IMP_UNIT_SIZE_DYNAMIC),
      IMP_WIDGET_PROGRESS_PERCENT(8, 2),
      IMP_WIDGET_LABEL(" Elapsed:"),
      IMP_WIDGET_SCALAR_UNIT(5, -1, IMP_UNIT_TIME_SEC)));

    imp_value_t const v = IMP_VALUE_COMPOSITE(6, IMP_ARRAY(
      IMP_VALUE_NULL(),
      IMP_VALUE_NULL(),
      IMP_VALUE_NULL(),
      IMP_VALUE_NULL(),
      IMP_VALUE_NULL(),
      es_val));

    VERIFY_IMP(imp_draw_line(ctx, &cur_prog, &max_prog, &w, &v));
  }

  {
    imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 4, IMP_ARRAY(
      IMP_WIDGET_LABEL("P-Bar   : uni-1w="),
      IMP_WIDGET_PROGRESS_BAR(30, "｢", "｣", "⨯", " ",
        &(imp_widget_def_t)IMP_WIDGET_LABEL("⧽")),
      IMP_WIDGET_LABEL(" uni-2w="),
      IMP_WIDGET_PROGRESS_BAR(34, "🌎", "🌑", "·", " ",
        &(imp_widget_def_t)IMP_WIDGET_LABEL("🚀"))));

    imp_value_t const v = IMP_VALUE_COMPOSITE(4, IMP_ARRAY(
      IMP_VALUE_NULL(),
      IMP_VALUE_NULL(),
      IMP_VALUE_NULL(),
      IMP_VALUE_NULL()));

    VERIFY_IMP(imp_draw_line(ctx, &cur_prog, &max_prog, &w, &v));
  }

  {
    imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 2, IMP_ARRAY(
      IMP_WIDGET_LABEL("P-Bar   : pct-fill-pct="),
      IMP_WIDGET_PROGRESS_BAR(70, "｢", "｣", "·", " ",
        &(imp_widget_def_t)IMP_WIDGET_PROGRESS_PERCENT(-1, 3))));

    imp_value_t const v =
      IMP_VALUE_COMPOSITE(2, IMP_ARRAY(IMP_VALUE_NULL(), IMP_VALUE_NULL()));

    VERIFY_IMP(imp_draw_line(ctx, &cur_prog, &max_prog, &w, &v));
  }

  {
    imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 2, IMP_ARRAY(
      IMP_WIDGET_LABEL("P-Bar   : pct-fill-spn="),
      IMP_WIDGET_PROGRESS_BAR(70, "｢", "｣", "·", " ",
        &(imp_widget_def_t)IMP_WIDGET_SPINNER(500, 6,
          IMP_ARRAY("🍶", "🍷", "🍸", "🍹", "🍺", "🍻")))));

    imp_value_t const v = IMP_VALUE_COMPOSITE(2, IMP_ARRAY(IMP_VALUE_NULL(), ems_val));

    VERIFY_IMP(imp_draw_line(ctx, &cur_prog, &max_prog, &w, &v));
  }

  {
    imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 2, IMP_ARRAY(
      IMP_WIDGET_LABEL("P-Bar   : backwards="),
      IMP_WIDGET_PROGRESS_BAR(71, "🌎", "🌑", " ", "·",
        &(imp_widget_def_t)IMP_WIDGET_LABEL("🚀"))));

    imp_value_t const v =
      IMP_VALUE_COMPOSITE(2, IMP_ARRAY(IMP_VALUE_NULL(), IMP_VALUE_NULL()));

    imp_value_t const backward_cur = IMP_VALUE_INT((10. - elapsed_s) * 100000.);
    VERIFY_IMP(imp_draw_line(ctx, &backward_cur, &max_prog, &w, &v));
  }

  {
    imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 2, IMP_ARRAY(
      IMP_WIDGET_LABEL("P-Bar   : composite="),
      IMP_WIDGET_PROGRESS_BAR(73, "｢", "｣", "·", " ",
        &(imp_widget_def_t)IMP_WIDGET_COMPOSITE(-1, 2, IMP_ARRAY(
          IMP_WIDGET_SPINNER(500, 2, IMP_ARRAY("🔥", "💥")),
          IMP_WIDGET_LABEL("🚀"))))));

    imp_value_t const v = IMP_VALUE_COMPOSITE(2, IMP_ARRAY(
      IMP_VALUE_NULL(), IMP_VALUE_COMPOSITE(2, IMP_ARRAY(ems_val, IMP_VALUE_NULL()))));

    VERIFY_IMP(imp_draw_line(ctx, &cur_prog, &max_prog, &w, &v));
  }

  {
    imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 2, IMP_ARRAY(
      IMP_WIDGET_LABEL("P-Bar   : silly="),
      IMP_WIDGET_PROGRESS_BAR(77, "｢", "｣", "·", " ",
        &(imp_widget_def_t)IMP_WIDGET_COMPOSITE(-1, 7, IMP_ARRAY(
          IMP_WIDGET_SPINNER(500, 2, IMP_ARRAY("💥", "🔥")),
          IMP_WIDGET_LABEL("🚀"),
          IMP_WIDGET_PROGRESS_PERCENT(-1, 2),
          IMP_WIDGET_LABEL("🚀"),
          IMP_WIDGET_SCALAR_UNIT(-1, 2, IMP_UNIT_SIZE_DYNAMIC),
          IMP_WIDGET_LABEL("🚀"),
          IMP_WIDGET_SPINNER(500, 2, IMP_ARRAY("🔥", "💥")))))));

    imp_value_t const v = IMP_VALUE_COMPOSITE(2, IMP_ARRAY(
      IMP_VALUE_NULL(),
      IMP_VALUE_COMPOSITE(7, IMP_ARRAY(
        ems_val,
        IMP_VALUE_NULL(),
        es_val,
        IMP_VALUE_NULL(),
        cur_prog,
        IMP_VALUE_NULL(),
        ems_val))));

    VERIFY_IMP(imp_draw_line(ctx, &cur_prog, &max_prog, &w, &v));
  }

  {
    imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 2, IMP_ARRAY(
      IMP_WIDGET_LABEL("P-Bar   : block-elts="),
      IMP_WIDGET_PROGRESS_BAR_SCALE_EDGE_FILL(72, "[", "]",  "█", " ",
        &(imp_widget_def_t)IMP_WIDGET_PROGRESS_LABEL(-1, 8, IMP_ARRAY(
          IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.125f, " "),
          IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.250f, "▏"),
          IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.375f, "▎"),
          IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.500f, "▍"),
          IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.675f, "▌"),
          IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.750f, "▋"),
          IMP_WIDGET_PROGRESS_LABEL_ENTRY(0.875f, "▊"),
          IMP_WIDGET_PROGRESS_LABEL_ENTRY(1.000f, "█"))))));

    imp_value_t const v =
      IMP_VALUE_COMPOSITE(2, IMP_ARRAY(IMP_VALUE_NULL(), IMP_VALUE_NULL()));

    VERIFY_IMP(imp_draw_line(ctx, &cur_prog, &max_prog, &w, &v));
  }
}

static void test_add_and_remove_lines(imp_ctx_t *ctx, double elapsed_s) {
  imp_widget_def_t const w = IMP_WIDGET_COMPOSITE(-1, 2, IMP_ARRAY(
    IMP_WIDGET_LABEL("Add/Rem : "), IMP_WIDGET_SCALAR(-1, -1)));

  imp_value_t const cur_prog = IMP_VALUE_DOUBLE(elapsed_s * 100000.);
  imp_value_t const max_prog = IMP_VALUE_DOUBLE(10. * 100000.);

  int const si = (int)elapsed_s, lines = 1 + (si < 6 ? si : -(si - 10));
  for (int i = 0; i < lines; ++i) {
    imp_value_t const v =
      IMP_VALUE_COMPOSITE(2, IMP_ARRAY(IMP_VALUE_NULL(), IMP_VALUE_INT(i)));

    VERIFY_IMP(imp_draw_line(ctx, &cur_prog, &max_prog, &w, &v));
  }
}

static void test_improg(void) {
  imp_util_enable_utf8();

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

    VERIFY_IMP(imp_begin(&ctx, term_width));
    test_label(&ctx);
    test_scalar(&ctx);
    test_scalar_bytes(&ctx);
    test_scalar_bytes_fw(&ctx);
    test_scalar_bytes_dynamic(&ctx);
    test_scalar_time(&ctx);
    test_scalar_time_fw(&ctx);
    test_string(&ctx, elapsed_s);
    test_string_trim(&ctx);
    test_string_trim_fw(&ctx);
    test_string_trim_unicode(&ctx);
    test_spinner(&ctx, (int)(elapsed_s * 1000.));
    test_percent(&ctx, elapsed_s);
    test_progress_label(&ctx, elapsed_s);
    test_progress_scalar_int(&ctx, elapsed_s);
    test_progress_scalar_float(&ctx, elapsed_s);
    test_progress_fraction_int(&ctx, elapsed_s);
    test_progress_bar(&ctx, elapsed_s);
    test_add_and_remove_lines(&ctx, elapsed_s);
    test_label(&ctx);
    VERIFY_IMP(imp_end(&ctx, done));

    msleep(frame_time_ms);
  } while (!done);
}

int main(int argc, char const *argv[]) {
  (void)argc; (void)argv;
  test_improg();
  return 0;
}
