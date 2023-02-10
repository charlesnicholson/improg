// ImProg: An immediate-mode progress bar system for ANSI terminals.
#ifndef IMPROG_H
#define IMPROG_H

typedef enum {
  IMP_RET_SUCCESS = 0,
  IMP_RET_ERR_ARGS,
  IMP_RET_ERR_EXHAUSTED
} imp_ret_t;

typedef enum {
  IMP_WIDGET_TYPE_CONST_TEXT,
  IMP_WIDGET_TYPE_SCALAR,
  IMP_WIDGET_TYPE_STRING,
  IMP_WIDGET_TYPE_SPINNER,
  IMP_WIDGET_TYPE_FRACTION,
  IMP_WIDGET_TYPE_STOPWATCH,
  IMP_WIDGET_TYPE_PROGRESS_BAR,
} imp_widget_type_t;

typedef struct {
  char const *text;
} imp_widget_const_text_t;

typedef struct {
} imp_widget_string_t;

typedef struct {
  // imp_unit_t unit;
} imp_widget_scalar_t;

typedef struct {
  imp_widget_type_t type;
  union {
    imp_widget_const_text_t const_text;
    imp_widget_string_t str;
    imp_widget_scalar_t scalar;
  } w;
} imp_widget_def_t;

typedef struct {
} imp_linedef_t;

imp_ret_t imp_begin();
imp_ret_t imp_draw();
imp_ret_t imp_end();

#endif
