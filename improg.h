// ImProg: An immediate-mode UTF-8 progress bar system for ANSI terminals.
#ifndef IMPROG_H
#define IMPROG_H

#include <stdbool.h>
#include <stdint.h>

typedef enum imp_ret {
  IMP_RET_SUCCESS = 0,
  IMP_RET_ERR_ARGS,
  IMP_RET_ERR_WRONG_VALUE_TYPE,
  IMP_RET_ERR_AMBIGUOUS_WIDTH,
  IMP_RET_ERR_EXHAUSTED,
} imp_ret_t;

typedef enum imp_widget_type {
  IMP_WIDGET_TYPE_LABEL,              // constant text
  IMP_WIDGET_TYPE_PING_PONG_BAR,      // dynamic-width bar with back-and-forth "ball"
  IMP_WIDGET_TYPE_PROGRESS_BAR,       // dynamic-width bar that fills from left to %
  IMP_WIDGET_TYPE_PROGRESS_FRACTION,  // "X/Y" formatted progress values with units
  IMP_WIDGET_TYPE_PROGRESS_LABEL,     // text chosen dynamically from array by % or range
  IMP_WIDGET_TYPE_PROGRESS_PERCENT,   // dynamic progress %
  IMP_WIDGET_TYPE_PROGRESS_SCALAR,    // dynamic progress value rendered with unit
  IMP_WIDGET_TYPE_SCALAR,             // dynamic number with unit
  IMP_WIDGET_TYPE_SPINNER,            // animated label flipbook
  IMP_WIDGET_TYPE_STRING,             // dynamic string
  IMP_WIDGET_TYPE_STOPWATCH,          // elapsed time counting up from 0
} imp_widget_type_t;

typedef enum imp_unit {
  IMP_UNIT_NONE,
  IMP_UNIT_SIZE_B,
  IMP_UNIT_SIZE_KB,
  IMP_UNIT_SIZE_MB,
  IMP_UNIT_SIZE_GB,
  IMP_UNIT_SIZE_DYNAMIC,
} imp_unit_t;

struct imp_widget_def;

#define IMP_ARRAY(...) { __VA_ARGS__ }

typedef struct imp_widget_label {
  char const *s;
} imp_widget_label_t;

#define IMP_WIDGET_LABEL(STRING) \
  { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = STRING } } }

typedef struct imp_widget_string {
  int field_width; // -1 for natural length
  int max_len; // -1 for natural length
} imp_widget_string_t;

#define IMP_WIDGET_STRING(FIELD_WIDTH, MAX_LEN) \
  { .type = IMP_WIDGET_TYPE_STRING, .w = { \
    .str = { .field_width = (FIELD_WIDTH), .max_len = (MAX_LEN) } } }

typedef struct imp_widget_scalar {
  imp_unit_t unit;
  int field_width;
  int precision;
} imp_widget_scalar_t;

#define IMP_WIDGET_SCALAR_UNIT(FIELD_WIDTH, PRECISION, UNIT) \
  { .type = IMP_WIDGET_TYPE_SCALAR, .w = { .scalar = { \
    .precision = (PRECISION), .field_width = (FIELD_WIDTH), .unit = (UNIT) } } }

#define IMP_WIDGET_SCALAR(FIELD_WIDTH, PRECISION) \
  IMP_WIDGET_SCALAR_UNIT((FIELD_WIDTH), (PRECISION), IMP_UNIT_NONE)

typedef struct imp_widget_spinner {
  char const *const *frames;
  unsigned frame_count;
  unsigned speed_msec;
} imp_widget_spinner_t;

#define IMP_WIDGET_SPINNER(SPEED_MSEC, FRAME_COUNT, FRAME_ARRAY) \
  { .type = IMP_WIDGET_TYPE_SPINNER, .w = { .spinner = { \
    .speed_msec = (SPEED_MSEC), .frame_count = (FRAME_COUNT), \
    .frames = (char const * const[]) FRAME_ARRAY } } }

typedef struct imp_widget_progress_fraction {
  imp_unit_t unit;
  int field_width; // -1 for natural length
  int precision;
} imp_widget_progress_fraction_t;

#define IMP_WIDGET_PROGRESS_FRACTION(FIELD_WIDTH, PRECISION, UNIT) \
  { .type = IMP_WIDGET_TYPE_PROGRESS_FRACTION, .w = { .progress_fraction = { \
    .precision = (PRECISION), .field_width = (FIELD_WIDTH), .unit = (UNIT) } } }

typedef struct imp_widget_progress_percent {
  int field_width; // -1 for natural length
  int precision;
} imp_widget_progress_percent_t;

#define IMP_WIDGET_PROGRESS_PERCENT(FIELD_WIDTH, PRECISION) \
  { .type = IMP_WIDGET_TYPE_PROGRESS_PERCENT, .w = { \
    .progress_percent = { .precision = (PRECISION), .field_width = (FIELD_WIDTH) } } }

typedef struct imp_widget_progress_scalar {
  imp_unit_t unit;
  int field_width;
  int precision;
} imp_widget_progress_scalar_t;

#define IMP_WIDGET_PROGRESS_SCALAR(FIELD_WIDTH, PRECISION, UNIT) \
  { .type = IMP_WIDGET_TYPE_PROGRESS_SCALAR, .w = { .progress_scalar = { \
    .precision = (PRECISION), .field_width = (FIELD_WIDTH), .unit = (UNIT) } } }

typedef struct imp_widget_progress_label_entry {
  float threshold; // upper bound, non-inclusive
  char const *s;
} imp_widget_progress_label_entry_t;

#define IMP_WIDGET_PROGRESS_LABEL_ENTRY(THRESHOLD, STRING) \
  { .threshold = (THRESHOLD), .s = STRING }

typedef struct imp_widget_progress_label {
  imp_widget_progress_label_entry_t const *labels;
  int label_count;
  int field_width; // -1 for natural length
} imp_widget_progress_label_t;

#define IMP_WIDGET_PROGRESS_LABEL(FIELD_WIDTH, LABEL_COUNT, LABEL_ARRAY) \
  { .type = IMP_WIDGET_TYPE_PROGRESS_LABEL, .w = { .progress_label = { \
    .field_width = (FIELD_WIDTH), .label_count = (LABEL_COUNT), \
    .labels = (imp_widget_progress_label_entry_t[]) LABEL_ARRAY } } }

typedef struct imp_widget_progress_bar {
  int field_width; // -1 for space-filling
  char const *left_end;
  char const *right_end;
  char const *full_fill; // single-column grapheme to paint the filled portion with
  char const *empty_fill; // single-column grapheme to paint the empty portion with
  struct imp_widget_def const *edge_fill; // widget to paint between empty + full
} imp_widget_progress_bar_t;

#define IMP_WIDGET_PROGRESS_BAR( \
  FIELD_WIDTH, LEFT_END, RIGHT_END, FULL_FILL, EMPTY_FILL, EDGE_FILL) \
  { .type = IMP_WIDGET_TYPE_PROGRESS_BAR, .w = { .progress_bar = { \
    .field_width = (FIELD_WIDTH), .left_end = LEFT_END, .right_end = RIGHT_END, \
    .full_fill = FULL_FILL, .empty_fill = EMPTY_FILL, .edge_fill = EDGE_FILL } } }

typedef struct imp_widget_ping_pong_bar {
  int field_width; // -1 for space-filling
  char const *left_end;
  char const *right_end;
  struct imp_widget_def const *bouncer; // spinner / % / stopwatch
  char const *fill; // single-column grapheme to paint the background with
} imp_widget_ping_pong_bar_t;

typedef struct imp_widget_def {
  union {
    imp_widget_label_t label;
    imp_widget_scalar_t scalar;
    imp_widget_string_t str;
    imp_widget_spinner_t spinner;
    imp_widget_progress_fraction_t progress_fraction;
    imp_widget_progress_percent_t progress_percent;
    imp_widget_progress_label_t progress_label;
    imp_widget_progress_bar_t progress_bar;
    imp_widget_progress_scalar_t progress_scalar;
    imp_widget_ping_pong_bar_t ping_pong_bar;
  } w;
  imp_widget_type_t type;
} imp_widget_def_t;

typedef enum {
  IMP_VALUE_TYPE_INT,
  IMP_VALUE_TYPE_DOUBLE,
  IMP_VALUE_TYPE_STRING,
  IMP_VALUE_TYPE_COMPOSITE,
} imp_value_type_t;

typedef struct imp_value_composite {
  struct imp_value const *const *values;
  int value_count;
} imp_value_composite_t;

typedef struct imp_value {
  union {
    int64_t i;
    double d;
    char const *s;
    imp_value_composite_t c;
  } v;
  imp_value_type_t type;
} imp_value_t;

#define IMP_VALUE_INT(I) { .type = IMP_VALUE_TYPE_INT, .v = { .i = (int64_t)(I) } }
#define IMP_VALUE_DOUBLE(D) { .type = IMP_VALUE_TYPE_DOUBLE, .v = { .d = (double)(D) } }
#define IMP_VALUE_STRING(S) { .type = IMP_VALUE_TYPE_STRING, .v = { .s = (S) } }

typedef void (*imp_print_cb_t)(void *ctx, char const *s);

typedef struct imp_ctx { // mutable, stateful across one set of lines
  imp_print_cb_t print_cb;
  void *print_cb_ctx;
  unsigned terminal_width;
  unsigned last_frame_line_count;
  unsigned cur_frame_line_count;
  unsigned ttl_elapsed_msec; // elapsed time since init()
  unsigned dt_msec; // elapsed time since last begin()
} imp_ctx_t;

imp_ret_t imp_init(imp_ctx_t *ctx, imp_print_cb_t print_cb, void *print_cb_ctx);
imp_ret_t imp_begin(imp_ctx_t *ctx, unsigned terminal_width, unsigned dt_msec);
imp_ret_t imp_draw_line(imp_ctx_t *ctx,
                        imp_value_t const *progress_cur,
                        imp_value_t const *progress_max,
                        int widget_count,
                        imp_widget_def_t const *widgets,
                        imp_value_t const * const values[]);
imp_ret_t imp_end(imp_ctx_t *ctx, bool done);

// Utility stuff, helpers

bool imp_util_get_terminal_width(unsigned *out_term_width);
int imp_util_get_display_width(char const *utf8_str);
bool imp_util_isatty(void);

// https://en.wikipedia.org/wiki/ANSI_escape_code#CSI_sequences
#define IMP_PREVLINE "\033[%dF"
#define IMP_HIDE_CURSOR "\033[?25l"
#define IMP_SHOW_CURSOR "\033[?25h"
#define IMP_ERASE_CURSOR_TO_LINE_END "\033[0K"
#define IMP_ERASE_CURSOR_TO_SCREEN_END "\033[0J"
#define IMP_AUTO_WRAP_DISABLE "\033[?7l"
#define IMP_AUTO_WRAP_ENABLE "\033[?7h"

#endif
