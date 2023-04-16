// ImProg: An immediate-mode UTF-8 progress bar system for ANSI terminals.
#ifndef IMPROG_H
#define IMPROG_H

#include <stdbool.h>
#include <stdint.h>

typedef enum imp_ret {
  IMP_RET_SUCCESS = 0,
  // -1 reserved for "unused" in some fields
  IMP_RET_ERR_ARGS = -2,
  IMP_RET_ERR_WRONG_VALUE_TYPE = -3,
  IMP_RET_ERR_AMBIGUOUS_WIDTH = -4,
  IMP_RET_ERR_EXHAUSTED = -5,
} imp_ret_t;

typedef struct imp_value imp_value_t;
typedef struct imp_widget_def imp_widget_def_t;
typedef struct imp_ctx imp_ctx_t;

typedef void (*imp_print_cb_t)(void *ctx, char const *s);

imp_ret_t imp_init(imp_ctx_t *ctx, imp_print_cb_t print_cb, void *print_cb_ctx);
imp_ret_t imp_begin(imp_ctx_t *ctx, uint16_t terminal_width);
imp_ret_t imp_draw_line(imp_ctx_t *ctx,
                        imp_value_t const *progress_cur,
                        imp_value_t const *progress_max,
                        imp_widget_def_t const *widget,
                        imp_value_t const *value);
imp_ret_t imp_end(imp_ctx_t *ctx, bool done);

// Widgets

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
  IMP_WIDGET_TYPE_COMPOSITE,          // list of sub-widgets
} imp_widget_type_t;

typedef enum imp_unit {
  IMP_UNIT_NONE,
  IMP_UNIT_SIZE_B,
  IMP_UNIT_SIZE_KB,
  IMP_UNIT_SIZE_MB,
  IMP_UNIT_SIZE_GB,
  IMP_UNIT_SIZE_DYNAMIC,
  IMP_UNIT_TIME_SEC,          // 8424s
  IMP_UNIT_TIME_HMS_LETTERS,  // 2h20m24s
  IMP_UNIT_TIME_HMS_COLONS    // 02:20:24
} imp_unit_t;

typedef struct imp_widget_label {
  char const *s;
} imp_widget_label_t;

#define IMP_WIDGET_LABEL(STRING) \
  { .type = IMP_WIDGET_TYPE_LABEL, .w = { .label = { .s = STRING } } }

typedef struct imp_widget_string {
  char const *custom_trim; // NULL ok
  int16_t field_width; // -1 for natural length
  int16_t max_len; // -1 for natural length
  bool trim_left; // true if beginning of string is trimmed
} imp_widget_string_t;

#define IMP_WIDGET_STRING(FIELD_WIDTH, MAX_LEN) \
  { .type = IMP_WIDGET_TYPE_STRING, .w = { .str = { .field_width = (int16_t)(FIELD_WIDTH), \
    .max_len = (int16_t)(MAX_LEN), .custom_trim = NULL, .trim_left = false } } }

#define IMP_WIDGET_STRING_CUSTOM_TRIM(FIELD_WIDTH, MAX_LEN, TRIM_STR, TRIM_LEFT) \
  { .type = IMP_WIDGET_TYPE_STRING, .w = { .str = { .field_width = (FIELD_WIDTH), \
    .max_len = (MAX_LEN), .custom_trim = TRIM_STR, .trim_left = (TRIM_LEFT) } } }

typedef struct imp_widget_scalar {
  imp_unit_t unit;
  int16_t field_width;
  int16_t precision;
} imp_widget_scalar_t;

#define IMP_WIDGET_SCALAR_UNIT(FIELD_WIDTH, PRECISION, UNIT) \
  { .type = IMP_WIDGET_TYPE_SCALAR, .w = { .scalar = { \
    .precision = (PRECISION), .field_width = (FIELD_WIDTH), .unit = (UNIT) } } }

#define IMP_WIDGET_SCALAR(FIELD_WIDTH, PRECISION) \
  IMP_WIDGET_SCALAR_UNIT((FIELD_WIDTH), (PRECISION), IMP_UNIT_NONE)

typedef struct imp_widget_spinner {
  char const *const *frames;
  uint16_t frame_count;
  uint16_t speed_msec;
} imp_widget_spinner_t;

#define IMP_WIDGET_SPINNER(SPEED_MSEC, FRAME_COUNT, FRAME_ARRAY) \
  { .type = IMP_WIDGET_TYPE_SPINNER, .w = { .spinner = { \
    .speed_msec = (SPEED_MSEC), .frame_count = (FRAME_COUNT), \
    .frames = (char const * const[]) FRAME_ARRAY } } }

typedef struct imp_widget_progress_fraction {
  imp_unit_t unit;
  int16_t field_width; // -1 for natural length
  int16_t precision;
} imp_widget_progress_fraction_t;

#define IMP_WIDGET_PROGRESS_FRACTION(FIELD_WIDTH, PRECISION, UNIT) \
  { .type = IMP_WIDGET_TYPE_PROGRESS_FRACTION, .w = { .progress_fraction = { \
    .precision = (PRECISION), .field_width = (FIELD_WIDTH), .unit = (UNIT) } } }

typedef struct imp_widget_progress_percent {
  int16_t field_width; // -1 for natural length
  int16_t precision;
} imp_widget_progress_percent_t;

#define IMP_WIDGET_PROGRESS_PERCENT(FIELD_WIDTH, PRECISION) \
  { .type = IMP_WIDGET_TYPE_PROGRESS_PERCENT, .w = { \
    .progress_percent = { .precision = (PRECISION), .field_width = (FIELD_WIDTH) } } }

typedef struct imp_widget_progress_scalar {
  imp_unit_t unit;
  int16_t field_width;
  int16_t precision;
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
  int16_t label_count;
  int16_t field_width; // -1 for natural length
} imp_widget_progress_label_t;

#define IMP_WIDGET_PROGRESS_LABEL(FIELD_WIDTH, LABEL_COUNT, LABEL_ARRAY) \
  { .type = IMP_WIDGET_TYPE_PROGRESS_LABEL, .w = { .progress_label = { \
    .field_width = (FIELD_WIDTH), .label_count = (LABEL_COUNT), \
    .labels = (imp_widget_progress_label_entry_t[]) LABEL_ARRAY } } }

typedef struct imp_widget_progress_bar {
  char const *left_end;
  char const *right_end;
  char const *full_fill; // single-column grapheme to paint the filled portion with
  char const *empty_fill; // single-column grapheme to paint the empty portion with
  struct imp_widget_def const *edge_fill; // widget to paint between empty + full
  int16_t field_width; // -1 for space-filling
  bool scale_fill; // if true, prog_cur + prox_max represent % of edge_fill
} imp_widget_progress_bar_t;

#define IMP_WIDGET_PROGRESS_BAR( \
  FIELD_WIDTH, LEFT_END, RIGHT_END, FULL_FILL, EMPTY_FILL, EDGE_FILL) \
  { .type = IMP_WIDGET_TYPE_PROGRESS_BAR, .w = { .progress_bar = { .scale_fill = false, \
    .field_width = (FIELD_WIDTH), .left_end = LEFT_END, .right_end = RIGHT_END, \
    .full_fill = FULL_FILL, .empty_fill = EMPTY_FILL, .edge_fill = EDGE_FILL } } }

#define IMP_WIDGET_PROGRESS_BAR_SCALE_EDGE_FILL( \
  FIELD_WIDTH, LEFT_END, RIGHT_END, FULL_FILL, EMPTY_FILL, EDGE_FILL) \
  { .type = IMP_WIDGET_TYPE_PROGRESS_BAR, .w = { .progress_bar = { .scale_fill = true, \
    .field_width = (FIELD_WIDTH), .left_end = LEFT_END, .right_end = RIGHT_END, \
    .full_fill = FULL_FILL, .empty_fill = EMPTY_FILL, .edge_fill = EDGE_FILL } } }

typedef struct imp_widget_ping_pong_bar {
  int16_t field_width; // -1 for space-filling
  char const *left_end;
  char const *right_end;
  struct imp_widget_def const *bouncer; // spinner / % / etc
  char const *fill; // single-column grapheme to paint the background with
} imp_widget_ping_pong_bar_t;

typedef struct imp_widget_composite {
  struct imp_widget_def const *widgets;
  int16_t widget_count;
  int16_t max_len;
} imp_widget_composite_t;

#define IMP_WIDGET_COMPOSITE(MAX_LENGTH, WIDGET_COUNT, WIDGET_ARRAY) \
  { .type = IMP_WIDGET_TYPE_COMPOSITE, .w = { .composite = { .max_len = MAX_LENGTH, \
    .widget_count = (WIDGET_COUNT), .widgets = (imp_widget_def_t const[]) WIDGET_ARRAY } } }

struct imp_widget_def {
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
    imp_widget_composite_t composite;
  } w;
  imp_widget_type_t type;
};

// Values

typedef enum {
  IMP_VALUE_TYPE_NULL,
  IMP_VALUE_TYPE_INT,
  IMP_VALUE_TYPE_DOUBLE,
  IMP_VALUE_TYPE_STRING,
  IMP_VALUE_TYPE_COMPOSITE,
} imp_value_type_t;

typedef struct imp_value_composite {
  struct imp_value const *values;
  int16_t value_count;
} imp_value_composite_t;

struct imp_value {
  union {
    int64_t i;
    double d;
    char const *s;
    imp_value_composite_t c;
  } v;
  imp_value_type_t type;
};

#define IMP_VALUE_NULL() { .type = IMP_VALUE_TYPE_NULL }
#define IMP_VALUE_INT(I) { .type = IMP_VALUE_TYPE_INT, .v = { .i = (int64_t)(I) } }
#define IMP_VALUE_DOUBLE(D) { .type = IMP_VALUE_TYPE_DOUBLE, .v = { .d = (double)(D) } }
#define IMP_VALUE_STRING(S) { .type = IMP_VALUE_TYPE_STRING, .v = { .s = (S) } }
#define IMP_VALUE_COMPOSITE(COUNT, VALUES) { .type = IMP_VALUE_TYPE_COMPOSITE, .v = { \
  .c = { .value_count = (COUNT), .values = (imp_value_t const[])VALUES } } }


struct imp_ctx { // mutable, stateful across one set of lines
  imp_print_cb_t print_cb;
  void *print_cb_ctx;
  uint16_t terminal_width;
  uint16_t last_frame_line_count;
  uint16_t cur_frame_line_count;
};

// Utility stuff, helpers

#define IMP_ARRAY(...) { __VA_ARGS__ }

void imp_util_enable_utf8(void);
bool imp_util_get_terminal_width(uint16_t *out_term_width);
int imp_util_get_display_width(char const *utf8_str);
bool imp_util_isatty(void);

// https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
#define IMP_COLOR_RESET             "\033[0m"
#define IMP_COLOR_FG_BLACK          "\033[30m"
#define IMP_COLOR_FG_RED            "\033[31m"
#define IMP_COLOR_FG_GREEN          "\033[32m"
#define IMP_COLOR_FG_YELLOW         "\033[33m"
#define IMP_COLOR_FG_BLUE           "\033[34m"
#define IMP_COLOR_FG_MAGENTA        "\033[35m"
#define IMP_COLOR_FG_CYAN           "\033[36m"
#define IMP_COLOR_FG_WHITE          "\033[37m"
#define IMP_COLOR_FG_BLACK_BRIGHT   "\033[90m"
#define IMP_COLOR_FG_RED_BRIGHT     "\033[91m"
#define IMP_COLOR_FG_GREEN_BRIGHT   "\033[92m"
#define IMP_COLOR_FG_YELLOW_BRIGHT  "\033[93m"
#define IMP_COLOR_FG_BLUE_BRIGHT    "\033[94m"
#define IMP_COLOR_FG_MAGENTA_BRIGHT "\033[95m"
#define IMP_COLOR_FG_CYAN_BRIGHT    "\033[96m"
#define IMP_COLOR_FG_WHITE_BRIGHT   "\033[97m"
#define IMP_COLOR_BG_BLACK          "\033[40m"
#define IMP_COLOR_BG_RED            "\033[41m"
#define IMP_COLOR_BG_GREEN          "\033[42m"
#define IMP_COLOR_BG_YELLOW         "\033[43m"
#define IMP_COLOR_BG_BLUE           "\033[44m"
#define IMP_COLOR_BG_MAGENTA        "\033[45m"
#define IMP_COLOR_BG_CYAN           "\033[46m"
#define IMP_COLOR_BG_WHITE          "\033[47m"
#define IMP_COLOR_BG_BLACK_BRIGHT   "\033[100m"
#define IMP_COLOR_BG_RED_BRIGHT     "\033[101m"
#define IMP_COLOR_BG_GREEN_BRIGHT   "\033[102m"
#define IMP_COLOR_BG_YELLOW_BRIGHT  "\033[103m"
#define IMP_COLOR_BG_BLUE_BRIGHT    "\033[104m"
#define IMP_COLOR_BG_MAGENTA_BRIGHT "\033[105m"
#define IMP_COLOR_BG_CYAN_BRIGHT    "\033[106m"
#define IMP_COLOR_BG_WHITE_BRIGHT   "\033[107m"

// Compile-time 8-bit color construction
#define IMP_COLOR_FG_256(VAL)       "\033[38;5;" #VAL "m"
#define IMP_COLOR_BG_256(VAL)       "\033[48;5;" #VAL "m"

// https://en.wikipedia.org/wiki/ANSI_escape_code#CSI_sequences
#define IMP_PREVLINE "\033[%dF"
#define IMP_HIDE_CURSOR "\033[?25l"
#define IMP_SHOW_CURSOR "\033[?25h"
#define IMP_ERASE_CURSOR_TO_LINE_END "\033[0K"
#define IMP_ERASE_CURSOR_TO_SCREEN_END "\033[0J"
#define IMP_AUTO_WRAP_DISABLE "\033[?7l"
#define IMP_AUTO_WRAP_ENABLE "\033[?7h"

#endif
