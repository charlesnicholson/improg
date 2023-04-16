// RemProg: A stateful retained-mode wrapper for ImProg.
#ifndef REMPROG_H
#define REMPROG_H

#include "improg.h"

typedef struct remp_cfg {
  int max_lines;
  int max_values_per_line;
  int max_terminal_width;
  int reqd_seat_size;
} remp_cfg_t;

void remp_cfg(int max_lines,
              int max_values_per_line,
              int max_terminal_width,
              remp_cfg_t *out_cfg);

typedef struct remp_line {
  imp_widget_def_t const *w;
} remp_line_t;

typedef struct remp_ctx {
  remp_cfg_t cfg;
  remp_line_t *lines;
  int num_lines;
} remp_ctx_t;

void remp_init(remp_cfg_t const *cfg, void *seat, remp_ctx_t **out_ctx);
void remp_add_line(remp_ctx_t *ctx, imp_widget_def_t const *def, int *out_line_id);
void remp_remove_line(remp_ctx_t *ctx, int line_id);
void remp_draw_lines(remp_ctx_t *ctx, bool done);

#endif
