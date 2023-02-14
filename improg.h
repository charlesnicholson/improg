// ImProg: An immediate-mode UTF-8 progress bar system for ANSI terminals.
#ifndef IMPROG_H
#define IMPROG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  IMP_RET_SUCCESS = 0,
  IMP_RET_ERR_ARGS,
  IMP_RET_ERR_EXHAUSTED
} imp_ret_t;

typedef enum {
  IMP_WIDGET_TYPE_LABEL,            // constant text
  IMP_WIDGET_TYPE_SCALAR,           // dynamic number with unit
  IMP_WIDGET_TYPE_STRING,           // dynamic string
  IMP_WIDGET_TYPE_SPINNER,          // animated fixed-position string
  IMP_WIDGET_TYPE_FRACTION,         // "X/Y" with printf-formatted numbers with units
  IMP_WIDGET_TYPE_STOPWATCH,        // elapsed time counting up from 0
  IMP_WIDGET_TYPE_PROGRESS_PERCENT, // dynamic progres %
  IMP_WIDGET_TYPE_PROGRESS_LABEL,   // text chosen dynamically from array by % or range
  IMP_WIDGET_TYPE_PROGRESS_BAR,     // dynamic-width bar that fills from left to %
  IMP_WIDGET_TYPE_PING_PONG_BAR,    // dynamic-width bar with back-and-forth "ball"
} imp_widget_type_t;

struct imp_widget_def;

typedef struct {
  char const *s;
} imp_widget_label_t;

typedef struct {
  int field_width; // -1 for natural length
} imp_widget_string_t;

typedef struct {
  // imp_unit_t unit; // todo: figure out unit conversion (min+sec/sec/msec, b/kb/mb)
  int unused;
} imp_widget_scalar_t;

typedef struct {
  char const *const *frames;
  unsigned frame_count;
  unsigned speed_msec;
} imp_widget_spinner_t;

typedef struct {
  int field_width; // -1 for natural length
  int precision;
} imp_widget_progress_percent_t;

typedef struct {
  int field_width; // -1 for space-filling
  char const *left_end;
  char const *right_end;
  char const *full_fill; // single-column grapheme to paint the filled portion with
  char const *empty_fill; // single-column grapheme to paint the empty portion with
  struct imp_widget_def const *threshold; // widget to paint between empty + full
} imp_widget_progress_bar_t;

typedef struct {
  int field_width; // -1 for space-filling
  char const *left_end;
  char const *right_end;
  struct imp_widget_def const *bouncer; // spinner / % / stopwatch
  char const *fill; // single-column grapheme to paint the background with
} imp_widget_ping_pong_bar_t;

typedef struct imp_widget_def {
  imp_widget_type_t type;
  union {
    imp_widget_label_t label;
    imp_widget_scalar_t scalar;
    imp_widget_string_t str;
    imp_widget_spinner_t spinner;
    imp_widget_progress_percent_t percent;
    imp_widget_progress_bar_t progress_bar;
    imp_widget_ping_pong_bar_t ping_pong_bar;
  } w;
} imp_widget_def_t;

typedef enum {
  IMP_VALUE_TYPE_INT,
  IMP_VALUE_TYPE_DOUBLE,
  IMP_VALUE_TYPE_STR,
} imp_value_type_t;

typedef struct imp_value {
  imp_value_type_t type;
  union {
    int64_t i;
    double d;
    char const *s;
  } v;
} imp_value_t;

typedef int (*imp_print_cb_t)(void *ctx, char const *fmt, va_list args);

typedef struct imp_ctx { // mutable, stateful across one set of lines
  imp_print_cb_t print_cb;
  void *print_cb_ctx;
  unsigned line_count;
  unsigned terminal_width;
  unsigned ttl_elapsed_msec; // elapsed time since init()
  unsigned dt_msec; // elapsed time since last begin()
} imp_ctx_t;

imp_ret_t imp_init(imp_ctx_t *ctx, imp_print_cb_t print_cb, void *print_cb_ctx);
imp_ret_t imp_begin(imp_ctx_t *ctx, unsigned terminal_width, unsigned dt_msec);
imp_ret_t imp_draw_line(imp_ctx_t *ctx,
                        imp_value_t const *progress_cur,
                        imp_value_t const *progress_max,
                        imp_widget_def_t const *widgets,
                        int widget_count,
                        imp_value_t const *values,
                        int value_count);
imp_ret_t imp_end(imp_ctx_t *ctx, bool done);

// Utility stuff, helpers

bool imp_util_get_terminal_width(unsigned *out_term_width);
int imp_util_get_display_width(char const *utf8_str);
bool imp_util_isatty(void);


// https://en.wikipedia.org/wiki/ANSI_escape_code#CSI_sequences
#define IMP_ESC "\033"
#define IMP_CSI "["

// "CSI n F" "CPL" Cursor previous line (n = # of lines)
#define IMP_PREVLINE "F"
#define IMP_FULL_PREVLINE IMP_ESC IMP_CSI "%d" IMP_PREVLINE

// "CSI ? 25 x" "DECTCEM" Hide (x = 'l') or show (x = 'h') cursor
#define IMP_DECTCEM "?25"
#define IMP_HIDECURSOR "l"
#define IMP_SHOWCURSOR "h"
#define IMP_FULL_HIDE_CURSOR IMP_ESC IMP_CSI IMP_DECTCEM IMP_HIDECURSOR
#define IMP_FULL_SHOW_CURSOR IMP_ESC IMP_CSI IMP_DECTCEM IMP_SHOWCURSOR

// "CSI n K" "EL" Erase in Line
#define IMP_ERASE_IN_LINE_CMD "K"
#define IMP_ERASE_IN_LINE_CURSOR_TO_END "0"
#define IMP_ERASE_IN_LINE_CURSOR_TO_BEGINNING "1"
#define IMP_ERASE_IN_LINE_ENTIRE "2"
#define IMP_FULL_ERASE_CURSOR_TO_END \
  IMP_ESC IMP_CSI IMP_ERASE_IN_LINE_CURSOR_TO_END IMP_ERASE_IN_LINE_CMD

// "CSI n J" "ED" Erase In Display
#define IMP_ERASE_IN_DISPLAY "J"
#define IMP_ERASE_IN_DISPLAY_CURSOR_TO_END "0"
#define IMP_ERASE_IN_DISPLAY_CURSOR_TO_BEGINNING "1"
#define IMP_ERASE_IN_DISPLAY_ENTIRE_SCREEN "2"
#define IMP_ERASE_IN_DISPLAY_ENTIRE_SCREEN_AND_HISTORY "3"
#define IMP_FULL_ERASE_IN_DISPLAY_CURSOR_TO_END IMP_ESC IMP_CSI IMP_ERASE_IN_DISPLAY

// "CSI s" "SCP" Save Cursor Position
#define IMP_SAVE_CURSOR_POSITION "s"
#define IMP_FULL_SAVE_CURSOR_POSITION IMP_ESC IMP_CSI IMP_SAVE_CURSOR_POSITION

// "CSI u" "RCP" Restore Cursor Position
#define IMP_RESTORE_CURSOR_POSITION "u"
#define IMP_FULL_RESTORE_CURSOR_POSITION IMP_ESC IMP_CSI IMP_RESTORE_CURSOR_POSITION

// "CSI ? 7 n" Auto-wrap https://vt100.net/docs/vt510-rm/DECAWM.html
#define IMP_AUTO_WRAP "?7"
#define IMP_AUTO_WRAP_DISABLE "l"
#define IMP_AUTO_WRAP_ENABLE "h"
#define IMP_FULL_AUTO_WRAP_DISABLE IMP_ESC IMP_CSI IMP_AUTO_WRAP IMP_AUTO_WRAP_DISABLE
#define IMP_FULL_AUTO_WRAP_ENABLE IMP_ESC IMP_CSI IMP_AUTO_WRAP IMP_AUTO_WRAP_ENABLE

#endif