// ImProg: An immediate-mode UTF-8 progress bar system for ANSI terminals.
#ifndef IMPROG_H
#define IMPROG_H

#include <stdarg.h>
#include <stdbool.h>

typedef enum {
  IMP_RET_SUCCESS = 0,
  IMP_RET_ERR_ARGS,
  IMP_RET_ERR_EXHAUSTED
} imp_ret_t;

typedef enum {
  IMP_WIDGET_TYPE_LABEL,          // constant text
  IMP_WIDGET_TYPE_SCALAR,         // dynamic number with unit
  IMP_WIDGET_TYPE_STRING,         // dynamic string
  IMP_WIDGET_TYPE_SPINNER,        // animated fixed-position string
  IMP_WIDGET_TYPE_FRACTION,       // "X/Y" with printf-formatted numbers with units
  IMP_WIDGET_TYPE_STOPWATCH,      // elapsed time counting up from 0
  IMP_WIDGET_TYPE_PROGRESS_LABEL, // text chosen dynamically from array by % or range
  IMP_WIDGET_TYPE_PROGRESS_BAR,   // dynamic-width bar that fills from left to %
  IMP_WIDGET_TYPE_PING_PONG_BAR,  // dynamic-width bar with back-and-forth "ball"
} imp_widget_type_t;

struct imp_widget_def;

typedef struct {
  char const *s;
  int display_width; // -1 = "use strlen" (set explicitly for emoji)
} imp_widget_label_t;

typedef struct {
  int field_width; // -1 for natural length
} imp_widget_string_t;

typedef struct {
  // imp_unit_t unit; // todo: figure out unit conversion (min+sec/sec/msec, b/kb/mb)
} imp_widget_scalar_t;

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
    imp_widget_string_t str;
    imp_widget_scalar_t scalar;
    imp_widget_progress_bar_t progress_bar;
    imp_widget_ping_pong_bar_t ping_pong_bar;
  } w;
} imp_widget_def_t;

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
imp_ret_t imp_drawline(imp_ctx_t *ctx, imp_widget_def_t const *widgets, int widget_count);
imp_ret_t imp_end(imp_ctx_t *ctx);

// Utility methods

int imp_util_get_display_width(char const *s); // TODO: lol
unsigned imp_util_get_terminal_width(void);
bool imp_util_isatty(void);

#endif
