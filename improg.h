// ImProg: An immediate-mode progress bar system for ANSI terminals.
#ifndef IMPROG_H
#define IMPROG_H

typedef enum {
  IMP_RET_SUCCESS = 0,
  IMP_RET_ERR_ARGS,
  IMP_RET_ERR_EXHAUSTED
} imp_ret_t;

typedef enum {
  IMP_WIDGET_TYPE_TEXT,
  IMP_WIDGET_TYPE_PROGRESS_BAR,
  IMP_WIDGET_TYPE_SPINNER,
  IMP_WIDGET_TYPE_SCALAR,
  IMP_WIDGET_TYPE_FRACTION,
  IMP_WIDGET_TYPE_STOPWATCH,
} imp_widget_type_t;

typedef struct {
  imp_widget_type_t type;
} imp_widget_def_t;

typedef struct {
} imp_linedef_t;

imp_ret_t imp_begin();
imp_ret_t imp_draw();
imp_ret_t imp_end();

#endif
