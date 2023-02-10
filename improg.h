// ImProg: An immediate-mode progress bar system for ANSI terminals.
#ifndef IMPROG_H
#define IMPROG_H

#include <stdbool.h>

typedef enum {
  IMP_RET_SUCCESS = 0,
  IMP_RET_ERR_ARGS,
  IMP_RET_ERR_EXHAUSTED
} imp_ret_t;

typedef enum {
  IMP_WIDGET_TYPE_LABEL,         // constant text
  IMP_WIDGET_TYPE_SCALAR,        // dynamic number with unit
  IMP_WIDGET_TYPE_STRING,        // dynamic string
  IMP_WIDGET_TYPE_SPINNER,       // animated fixed-position string
  IMP_WIDGET_TYPE_FRACTION,      // "X/Y" with printf-formatted numbers with units
  IMP_WIDGET_TYPE_STOPWATCH,     // elapsed time counting up from 0
  IMP_WIDGET_TYPE_PROGRESS_BAR,  // dynamic-width bar that fills from left to %
  IMP_WIDGET_TYPE_PING_PONG_BAR, // dynamic-width bar with back-and-forth "ball"
} imp_widget_type_t;

typedef struct {
  char const *s;
} imp_widget_label_t;

typedef struct {
} imp_widget_string_t;

typedef struct {
  // imp_unit_t unit; // todo: figure out unit conversion (min+sec/sec/msec, b/kb/mb)
} imp_widget_scalar_t;

typedef struct {
  char const *left_end;
  char const *right_end;
  char empty;
  char full;
  char threshold; // todo: make a widget (spinner / % text as the "arrowhead")
} imp_widget_progress_bar_t;

typedef struct {
  char const *left_end;
  char const *right_end;
  char const *bouncer; // todo: make a widget (elapsed time etc)
  char fill;
} imp_widget_ping_pong_bar_t;

typedef struct {
  imp_widget_type_t type;
  union {
    imp_widget_label_t label;
    imp_widget_string_t str;
    imp_widget_scalar_t scalar;
    imp_widget_progress_bar_t progress_bar;
    imp_widget_ping_pong_bar_t ping_pong_bar;
  } w;
} imp_widget_def_t;

typedef struct {
  char *buf;
  unsigned len;
} imp_buf_t;

typedef int (*imp_print_cb_t)(void *ctx, char const *str);

imp_ret_t imp_begin();
imp_ret_t imp_draw(/* const array of widgets, dynamic values */);
imp_ret_t imp_end(bool reset_cursor);

#endif
