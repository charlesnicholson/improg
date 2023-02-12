#include "improg.h"
#include <stdio.h>


int imp__default_print_cb(void *ctx, char const *fmt, va_list args) {
  (void)ctx;
  return vprintf(fmt, args);
}

static int imp__print(imp_ctx_t *ctx, char const *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int const result = ctx->print_cb(ctx->print_cb_ctx, fmt, args);
  va_end(args);
  return result;
}

imp_ret_t imp_init(imp_ctx_t *ctx, imp_print_cb_t print_cb, void *print_cb_ctx) {
  if (!ctx) { return IMP_RET_ERR_ARGS; }
  ctx->print_cb_ctx = print_cb_ctx;
  ctx->print_cb = print_cb ? print_cb : imp__default_print_cb;
  ctx->terminal_width = 0;
  ctx->line_count = 0;
  ctx->ttl_elapsed_msec = 0;
  ctx->dt_msec = 0;
  return IMP_RET_SUCCESS;
}

imp_ret_t imp_begin(imp_ctx_t *ctx, unsigned terminal_width, unsigned dt_msec) {
  if (!ctx) { return IMP_RET_ERR_ARGS; }
  ctx->terminal_width = terminal_width;
  ctx->dt_msec = dt_msec;
  ctx->line_count = 0;
  imp__print(ctx, IMP_FULL_HIDE_CURSOR);
  return IMP_RET_SUCCESS;
}

imp_ret_t imp_draw(imp_ctx_t *ctx, imp_widget_def_t const *widgets, int widget_count);

imp_ret_t imp_end(imp_ctx_t *ctx) {
  if (!ctx) { return IMP_RET_ERR_ARGS; }
  ctx->ttl_elapsed_msec += ctx->dt_msec;
  ctx->dt_msec = 0;
  imp__print(ctx, IMP_FULL_PREVLINE, ctx->line_count);
  return IMP_RET_SUCCESS;
}
