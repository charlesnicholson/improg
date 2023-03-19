#include "improg.h"

#include <inttypes.h>
#include <stdio.h>
#include <wchar.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

static int imp_util__wchar_display_width(wchar_t wc);
static int imp_util__wchar_from_utf8(unsigned char const *s, wchar_t *out);

static int imp__max(int a, int b) { return a > b ? a : b; }
static int imp__min(int a, int b) { return a < b ? a : b; }
static int imp__clamp(int lo, int x, int hi) { return (x < lo) ? lo : (x > hi) ? hi : x; }

static float imp__clampf(float lo, float x, float hi) {
  return (x < lo) ? lo : (x > hi) ? hi : x;
}

static void imp__default_print_cb(void *ctx, char const *s) {
  (void)ctx; s ? printf("%s", s) : fflush(stdout);
}

static void imp__print(imp_ctx_t *ctx, char const *s, int *dw) {
  ctx->print_cb(ctx->print_cb_ctx, s);
  if (s && dw) { *dw += imp_util_get_display_width(s); }
}

static bool imp__value_type_is_scalar(imp_value_t const *v) {
  if (!v) { return false; }
  return (v->type == IMP_VALUE_TYPE_DOUBLE) || (v->type == IMP_VALUE_TYPE_INT);
}

static bool imp__value_to_int(imp_value_t const *v, imp_value_t *out_iv) {
  if (!imp__value_type_is_scalar(v)) { return false; }
  if (v->type == IMP_VALUE_TYPE_INT) { *out_iv = *v; return true; }
  *out_iv = (imp_value_t) { .type = IMP_VALUE_TYPE_INT, .v = { .i = (int64_t)v->v.d } };
  return true;
}

static bool imp__value_to_float(imp_value_t const *v, imp_value_t *out_fv) {
  if (!imp__value_type_is_scalar(v)) { return false; }
  if (v->type == IMP_VALUE_TYPE_DOUBLE) { *out_fv = *v; return true; }
  *out_fv = (imp_value_t) { .type = IMP_VALUE_TYPE_DOUBLE, .v = { .d = (double)v->v.i } };
  return true;
}

static char const *imp__progress_label_get_string(imp_widget_progress_label_t const *pl,
                                                  float progress) {
  for (int li = 0; li < pl->label_count; ++li) {
    if (progress < pl->labels[li].threshold) { return pl->labels[li].s; }
  }
  return NULL;
}

static char const *imp__spinner_get_string(imp_widget_spinner_t const *s, unsigned msec) {
  unsigned const idx = (unsigned)((float)msec / (float)s->speed_msec);
  return s->frames[idx % (unsigned)s->frame_count];
}

static int imp__value_write(int field_width,
                            int precision,
                            imp_unit_t unit,
                            imp_value_t const *v,
                            char *out_buf,
                            unsigned buf_len) {
  imp_value_t conv_v = *v;
  imp_unit_t conv_u = unit;

  switch (unit) {
    case IMP_UNIT_NONE: break;

    case IMP_UNIT_SIZE_B:
      // TODO: validate v type
      imp__value_to_int(v, &conv_v);
      break;

    case IMP_UNIT_SIZE_KB:
      // TODO: validate v type
      imp__value_to_float(v, &conv_v);
      conv_v.v.d /= 1024.;
      break;

    case IMP_UNIT_SIZE_MB:
      // TODO: validate v type
      imp__value_to_float(v, &conv_v);
      conv_v.v.d /= (1024. * 1024.);
      break;

    case IMP_UNIT_SIZE_GB:
      // TODO: validate v type
      imp__value_to_float(v, &conv_v);
      conv_v.v.d /= (1024. * 1024. * 1024.);
      break;

    case IMP_UNIT_SIZE_DYNAMIC:
      // TODO: validate v type
      if (v->v.i < 1024) {
        imp__value_to_int(v, &conv_v);
        conv_u = IMP_UNIT_SIZE_B;
      } else {
        imp__value_to_float(v, &conv_v);
        if (v->v.i < (1024 * 1024)) {
          conv_v.v.d /= 1024.;
          conv_u = IMP_UNIT_SIZE_KB;
        } else if (v->v.i < (1024LL * 1024 * 1024)) {
          conv_v.v.d /= (1024. * 1024.);
          conv_u = IMP_UNIT_SIZE_MB;
        } else {
          conv_v.v.d /= (1024. * 1024. * 1024.);
          conv_u = IMP_UNIT_SIZE_GB;
        }
      }
      break;

    case IMP_UNIT_TIME_SEC:
    case IMP_UNIT_TIME_HMS_LETTERS:
    case IMP_UNIT_TIME_HMS_COLONS:
      imp__value_to_int(v, &conv_v);
      break;
  }

  bool const have_fw = field_width != -1;
  bool const have_pr = precision != -1;

  if ((unit == IMP_UNIT_TIME_HMS_LETTERS) || (unit == IMP_UNIT_TIME_HMS_COLONS)) {
    int const sec = (int)(conv_v.v.i % 60LL);
    int const min = (int)((conv_v.v.i / 60LL) % 60);
    int const hours = (int)(conv_v.v.i / 60LL / 60LL);
    int fw_pad = 0;
    if (have_fw) {
      char const *fmt = (unit == IMP_UNIT_TIME_HMS_LETTERS) ? "%dh%dm%ds" : "%02d:%02d:%02d";
      fw_pad = imp__max(0, field_width - snprintf(NULL, 0, fmt, hours, min, sec));
      if (!fw_pad) { return snprintf(out_buf, buf_len, fmt, hours, min, sec); }
    }
    char const *fmt =
      (unit == IMP_UNIT_TIME_HMS_LETTERS) ? "%*s%dh%dm%ds" : "%*s%02d:%02d:%02d";
    return snprintf(out_buf, buf_len, fmt, fw_pad, "", hours, min, sec);
  }

  static char const *s_unit_suffixes[] = {
    [IMP_UNIT_NONE] = "",
    [IMP_UNIT_SIZE_B] = "B",
    [IMP_UNIT_SIZE_KB] = "KB",
    [IMP_UNIT_SIZE_MB] = "MB",
    [IMP_UNIT_SIZE_GB] = "GB",
    [IMP_UNIT_TIME_SEC] = "s",
  };
  char const *us = s_unit_suffixes[conv_u];
  int us_len = 0;
  for (char const *src = us; *src; ++src, ++us_len);
  int const fw = imp__max(0, field_width - us_len);

  switch (conv_v.type) {
    case IMP_VALUE_TYPE_INT:
      if (!have_fw) { return snprintf(out_buf, buf_len, "%" PRIi64 "%s", conv_v.v.i, us); }
      return snprintf(out_buf, buf_len, "%*" PRIi64 "%s", fw, conv_v.v.i, us);

    case IMP_VALUE_TYPE_DOUBLE: {
      double const d = conv_v.v.d;
      int const pr = precision;
      if (!have_fw && !have_pr) { return snprintf(out_buf, buf_len, "%f%s", d, us); }
      if (have_fw && !have_pr) { return snprintf(out_buf, buf_len, "%*f%s", fw, d, us); }
      if (!have_fw && have_pr) { return snprintf(out_buf, buf_len, "%.*f%s", pr, d, us); }
      return snprintf(out_buf, buf_len, "%*.*f%s", fw, pr, d, us);
    }

    default: break;
  }

  return -1;
}

static int imp__scalar_write(imp_widget_scalar_t const *s,
                             imp_value_t const *v,
                             char *out_buf,
                             unsigned buf_len) {
  return imp__value_write(s->field_width, s->precision, s->unit, v, out_buf, buf_len);
}

static int imp__progress_scalar_write(imp_widget_progress_scalar_t const *s,
                                      imp_value_t const *v,
                                      char *out_buf,
                                      unsigned buf_len) {
  return imp__value_write(s->field_width, s->precision, s->unit, v, out_buf, buf_len);
}

// TODO: delete this, combine it with get_display_width
static int imp__clipped_str_len(char const *str, int max_len) {
  int i = 0;
  unsigned char const *cur = (unsigned char const *)str;

  while (*cur) {
    wchar_t wc;
    cur += imp_util__wchar_from_utf8(cur, &wc);
    int const dw = imp_util__wchar_display_width(wc);
    if (dw < 0) { return dw; }
    if ((max_len >= 0) && ((i + dw) > max_len)) { break; }
    i += dw;
  }

  return i;
}

static int imp__string_write(imp_ctx_t *ctx,
                             imp_widget_string_t const *s,
                             imp_value_t const *v) {
  bool const have_v = v && v->v.s;
  bool const have_fw = (s->field_width != -1);
  bool const have_ml = (s->max_len != -1);
  bool const have_ct = (s->custom_trim != NULL);

  // s = string, fwp = field width pad, ml = max length, ct = custom trim
  int const s_len = have_v ? imp_util_get_display_width(v->v.s) : 0;
  int const sml_len = (have_v && have_ml) ? imp__clipped_str_len(v->v.s, s->max_len) : s_len;
  int const fwp_len = have_fw ? imp__max(0, s->field_width - sml_len) : 0;
  int const ct_len = have_ct ? imp_util_get_display_width(s->custom_trim) : 0;

  bool const need_ct = have_ct && ct_len && (sml_len < s_len) && (sml_len > ct_len);
  bool const need_ltrim = (sml_len < s_len) && s->trim_left;

  int const sctml_len = need_ct ? (sml_len - ct_len) : sml_len;

  for (int i = 0; i < fwp_len; ++i) { imp__print(ctx, " ", NULL); }
  if (!have_v) { return fwp_len; }

  if (!need_ct && (sml_len == s_len)) { // No trim, string fits in len
    imp__print(ctx, v->v.s, NULL);
    return fwp_len + s_len;
  }

  unsigned char const *cur = (unsigned char const *)v->v.s;
  if (need_ltrim) { // ltrim pre-advances through string
    if (need_ct) { imp__print(ctx, s->custom_trim, NULL); }
    int i = 0;
    while (*cur) {
      wchar_t wc;
      int const wc_l = imp_util__wchar_from_utf8(cur, &wc);
      int const wc_w = imp_util__wchar_display_width(wc);
      if (wc_w < 0) { return -1; }
      if (s_len - (i + wc_w) < sctml_len) { break; }
      i += wc_w;
      cur += wc_l;
    }
  }

  int i = 0;
  while (i < sctml_len) {
    char cp[5];
    int const cp_len = imp_util__wchar_from_utf8(cur, NULL);
    for (int cpi = 0; cpi < cp_len; ++cpi) { cp[cpi] = (char)cur[cpi]; }
    cp[cp_len] = '\0';
    imp__print(ctx, cp, &i);
    cur += cp_len;
  }

  if (need_ct && !s->trim_left) { imp__print(ctx, s->custom_trim, NULL); }
  return sml_len + fwp_len;
}

static int imp__progress_percent_write(imp_widget_progress_percent_t const *p,
                                       float progress,
                                       char *out_buf,
                                       unsigned buf_len) {
  double const p_pct = (double)(progress * 100.f);
  bool const have_fw = (p->field_width >= 0);
  bool const have_pr = (p->precision >= 0);
  int const fw = imp__max(0, p->field_width - 1);
  int const pr = p->precision;

  if (!have_fw && !have_pr) { return snprintf(out_buf, buf_len, "%f%%", p_pct); }
  if (!have_fw && have_pr) { return snprintf(out_buf, buf_len, "%.*f%%", pr, p_pct); }
  if (have_fw && !have_pr) { return snprintf(out_buf, buf_len, "%*f%%", fw, p_pct); }
  return snprintf(out_buf, buf_len, "%*.*f%%", fw, pr, p_pct);
}

static int imp__progress_fraction_write(imp_widget_progress_fraction_t const *f,
                                        imp_value_t const *prog_cur,
                                        imp_value_t const *prog_max,
                                        char *out_buf,
                                        unsigned buf_len) {
  int const fw = f->field_width, prec = f->precision;
  imp_unit_t const u = f->unit;
  int const num_len = imp__value_write(-1, prec, u, prog_cur, NULL, 0);
  int const den_len = imp__value_write(-1, prec, u, prog_max, NULL, 0);
  if ((num_len == -1) || (den_len == -1)) { return -1; }

  int const frac_len = num_len + den_len + 1;
  int const fw_pad = (fw > frac_len) ? (fw - frac_len) : 0;
  int const ttl_len = frac_len + fw_pad;

  if (buf_len) {
    int off = 0;
    for (; off < imp__min((int)buf_len, fw_pad); ++off) { out_buf[off] = ' '; }
    off += imp__value_write(-1, prec, u, prog_cur, &out_buf[off], buf_len - (unsigned)off);
    if (off < (int)buf_len) { out_buf[off++] = '/'; }
    off += imp__value_write(-1, prec, u, prog_max, &out_buf[off], buf_len - (unsigned)off);
  }

  return ttl_len;
}

static int imp_widget_display_width(imp_widget_def_t const *w,
                                    imp_value_t const *v,
                                    float prog_pct,
                                    imp_value_t const *prog_cur,
                                    imp_value_t const *prog_max,
                                    unsigned msec) {
  switch (w->type) {
    case IMP_WIDGET_TYPE_LABEL: return imp_util_get_display_width(w->w.label.s);
    case IMP_WIDGET_TYPE_SCALAR: return imp__scalar_write(&w->w.scalar, v, NULL, 0);

    case IMP_WIDGET_TYPE_STRING: {
      imp_widget_string_t const *s = &w->w.str;
      int const str_len = (v && v->v.s) ? imp__clipped_str_len(v->v.s, s->max_len) : 0;
      return imp__max(s->field_width, str_len);
    }

    case IMP_WIDGET_TYPE_SPINNER:
      return imp_util_get_display_width(imp__spinner_get_string(&w->w.spinner, msec));

    case IMP_WIDGET_TYPE_PROGRESS_FRACTION:
      return imp__progress_fraction_write(
        &w->w.progress_fraction, prog_cur, prog_max, NULL, 0);

    case IMP_WIDGET_TYPE_PROGRESS_PERCENT:
      return imp__progress_percent_write(&w->w.progress_percent, prog_pct, NULL, 0);

    case IMP_WIDGET_TYPE_PROGRESS_LABEL: {
      imp_widget_progress_label_t const *p = &w->w.progress_label;
      char const *s = imp__progress_label_get_string(p, prog_pct);
      int const dw = s ? imp_util_get_display_width(s) : 0;
      return imp__max(p->field_width, dw);
    }

    case IMP_WIDGET_TYPE_PROGRESS_SCALAR:
      return imp__progress_scalar_write(&w->w.progress_scalar, prog_cur, NULL, 0);

    case IMP_WIDGET_TYPE_PROGRESS_BAR: return w->w.progress_bar.field_width;
    case IMP_WIDGET_TYPE_PING_PONG_BAR: return w->w.ping_pong_bar.field_width;
    default: break;
  }
  return 0;
}

static imp_ret_t imp__draw_widget(imp_ctx_t *ctx,
                                  float prog_pct,
                                  imp_value_t const *prog_cur,
                                  imp_value_t const *prog_max,
                                  int wi,
                                  int widget_count,
                                  imp_widget_def_t const *widgets,
                                  imp_value_t const * const values[],
                                  int *cx) {
  unsigned const msec = ctx->ttl_elapsed_msec, tw = ctx->terminal_width;
  imp_widget_def_t const *w = &widgets[wi];
  imp_value_t const *v = values[wi];
  char buf[64];

  switch (w->type) {
    case IMP_WIDGET_TYPE_LABEL: imp__print(ctx, w->w.label.s, cx); break;

    case IMP_WIDGET_TYPE_STRING: {
      if (!v || (v->type != IMP_VALUE_TYPE_STRING)) { return IMP_RET_ERR_WRONG_VALUE_TYPE; }
      int const n = imp__string_write(ctx, &w->w.str, v);
      if (cx) { *cx += n; }
    } break;

    case IMP_WIDGET_TYPE_PROGRESS_PERCENT: {
      imp__progress_percent_write(&w->w.progress_percent, prog_pct, buf, sizeof(buf));
      imp__print(ctx, buf, cx);
    } break;

    case IMP_WIDGET_TYPE_PROGRESS_LABEL: {
      imp_widget_progress_label_t const *p = &w->w.progress_label;
      char const *s = imp__progress_label_get_string(p, prog_pct);
      int const dw = s ? imp_util_get_display_width(s) : 0;
      int const fw_pad = imp__max(0, p->field_width - dw);
      for (int i = 0; i < fw_pad; ++i) { imp__print(ctx, " ", NULL); }
      if (s) { imp__print(ctx, s, NULL); }
      if (cx) { *cx += (dw + fw_pad); }
    } break;

    case IMP_WIDGET_TYPE_PROGRESS_BAR: {
      imp_widget_progress_bar_t const *pb = &w->w.progress_bar;
      imp__print(ctx, pb->left_end, cx);

      int bar_w = pb->field_width;
      if (bar_w == -1) {
        int rhs = 0;
        for (int wj = wi + 1; wj < widget_count; ++wj) {
          imp_widget_def_t const *cur_w = &widgets[wj];
          int const cur_ww =
            imp_widget_display_width(cur_w, values[wj], prog_pct, prog_cur, prog_max, msec);
          if (cur_ww < 0) { return IMP_RET_ERR_AMBIGUOUS_WIDTH; }
          rhs += cur_ww;
        }
        bar_w = (int)tw - *cx - imp_util_get_display_width(pb->right_end) - rhs;
      }

      int const edge_w =
        imp_widget_display_width(pb->edge_fill, v, prog_pct, prog_cur, prog_max, msec);
      bool const draw_edge = (edge_w <= bar_w) && (prog_pct > 0.f) && (prog_pct < 1.f);
      int const prog_w = (int)((float)bar_w * prog_pct);
      int const edge_off = imp__clamp(0, prog_w - (edge_w / 2), bar_w - edge_w);
      int const full_w = draw_edge ? edge_off : prog_w;
      int const empty_w = draw_edge ? bar_w - (full_w + edge_w) : (bar_w - full_w);

      for (int fi = 0; fi < full_w; ++fi) { imp__print(ctx, pb->full_fill, NULL); }
      if (draw_edge) {
        if (pb->scale_fill) {
          float const sub_pct =
            (prog_pct - ((float)full_w * ((float)edge_w / (float)bar_w))) * (float)bar_w;
          imp__draw_widget(
            ctx, sub_pct, &(imp_value_t)IMP_VALUE_DOUBLE(sub_pct),
            &(imp_value_t)IMP_VALUE_DOUBLE(1.), 0, 1, pb->edge_fill, &v, NULL);
        } else {
          imp__draw_widget(ctx, prog_pct, prog_cur, prog_max, 0, 1, pb->edge_fill, &v, NULL);
        }
      }
      for (int ei = 0; ei < empty_w; ++ei) { imp__print(ctx, pb->empty_fill, NULL); }

      if (cx) { *cx += bar_w; }
      imp__print(ctx, pb->right_end, cx);
    } break;

    case IMP_WIDGET_TYPE_PROGRESS_FRACTION: {
      int const len = imp__progress_fraction_write(
        &w->w.progress_fraction, prog_cur, prog_max, buf, sizeof(buf));
      if (len == -1) { return IMP_RET_ERR_WRONG_VALUE_TYPE; }
      if (cx) { *cx += len; }
      imp__print(ctx, buf, NULL);
    } break;

    case IMP_WIDGET_TYPE_PROGRESS_SCALAR: {
      int const len =
        imp__progress_scalar_write(&w->w.progress_scalar, prog_cur, buf, sizeof(buf));
      if (len == -1) { return IMP_RET_ERR_WRONG_VALUE_TYPE; }
      if (cx) { *cx += len; }
      imp__print(ctx, buf, NULL);
    } break;

    case IMP_WIDGET_TYPE_SCALAR: {
      if (!imp__value_type_is_scalar(v)) { return IMP_RET_ERR_WRONG_VALUE_TYPE; }
      int const len = imp__scalar_write(&w->w.scalar, v, buf, sizeof(buf));
      if (len == -1) { return IMP_RET_ERR_WRONG_VALUE_TYPE; }
      if (cx) { *cx += len; }
      imp__print(ctx, buf, NULL);
    } break;

    case IMP_WIDGET_TYPE_SPINNER:
      imp__print(ctx, imp__spinner_get_string(&w->w.spinner, msec), cx);
      break;

    case IMP_WIDGET_TYPE_PING_PONG_BAR: break;
    default: break;
  }

  return IMP_RET_SUCCESS;
}

imp_ret_t imp_init(imp_ctx_t *ctx, imp_print_cb_t print_cb, void *print_cb_ctx) {
  if (!ctx) { return IMP_RET_ERR_ARGS; }
  ctx->print_cb_ctx = print_cb_ctx;
  ctx->print_cb = print_cb ? print_cb : imp__default_print_cb;
  ctx->terminal_width = 0;
  ctx->cur_frame_line_count = 0;
  ctx->last_frame_line_count = 0;
  ctx->ttl_elapsed_msec = 0;
  ctx->dt_msec = 0;
  return IMP_RET_SUCCESS;
}

imp_ret_t imp_begin(imp_ctx_t *ctx, unsigned terminal_width, unsigned dt_msec) {
  if (!ctx) { return IMP_RET_ERR_ARGS; }
  ctx->terminal_width = terminal_width;
  ctx->dt_msec = dt_msec;

  imp__print(ctx, IMP_HIDE_CURSOR IMP_AUTO_WRAP_DISABLE "\r", NULL);
  if (ctx->cur_frame_line_count > 1) {
    char cmd[16];
    snprintf(cmd, sizeof(cmd), IMP_PREVLINE, ctx->cur_frame_line_count - 1);
    cmd[sizeof(cmd)-1] = 0;
    imp__print(ctx, cmd, NULL);
  }

  ctx->last_frame_line_count = ctx->cur_frame_line_count;
  ctx->cur_frame_line_count = 0;
  return IMP_RET_SUCCESS;
}

imp_ret_t imp_end(imp_ctx_t *ctx, bool done) {
  if (!ctx) { return IMP_RET_ERR_ARGS; }
  ctx->ttl_elapsed_msec += ctx->dt_msec;
  ctx->dt_msec = 0;
  if (done) {
    imp__print(ctx, "\n" IMP_AUTO_WRAP_ENABLE IMP_SHOW_CURSOR, NULL);
  } else {
    if (ctx->cur_frame_line_count < ctx->last_frame_line_count) {
      imp__print(ctx, "\n" IMP_ERASE_CURSOR_TO_SCREEN_END, NULL);
      ++ctx->cur_frame_line_count;
    }
  }
  imp__print(ctx, NULL, NULL);
  return IMP_RET_SUCCESS;
}

imp_ret_t imp_draw_line(imp_ctx_t *ctx,
                        imp_value_t const *prog_cur,
                        imp_value_t const *prog_max,
                        int widget_count,
                        imp_widget_def_t const *widgets,
                        imp_value_t const * const values[]) {
  if (!ctx) { return IMP_RET_ERR_ARGS; }
  if ((bool)!!prog_max ^ (bool)!!prog_cur) { return IMP_RET_ERR_ARGS; }
  if (prog_cur && (prog_cur->type == IMP_VALUE_TYPE_STRING)) { return IMP_RET_ERR_ARGS; }
  if (prog_cur && (prog_cur->type != prog_max->type)) { return IMP_RET_ERR_ARGS; }

  float p = 0.f;
  if (prog_cur) {
    if (prog_cur->type == IMP_VALUE_TYPE_DOUBLE) {
      p = imp__clampf(0.f, (float)(prog_cur->v.d / prog_max->v.d), 1.f);
    } else {
      if (prog_cur->v.i >= prog_max->v.i) {
        p = 1.f;
      } else {
        p = imp__clampf(0.f, (float)prog_cur->v.i / (float)prog_max->v.i, 1.f);
      }
    }
  }

  if (ctx->cur_frame_line_count) { imp__print(ctx, "\n", NULL); }

  int cx = 0;
  for (int i = 0; i < widget_count; ++i) {
    imp_ret_t const ret =
      imp__draw_widget(ctx, p, prog_cur, prog_max, i, widget_count, widgets, values, &cx);
    if (ret != IMP_RET_SUCCESS) { return ret; }
  }

  if (cx < (int)ctx->terminal_width) {
    imp__print(ctx, IMP_ERASE_CURSOR_TO_LINE_END, NULL);
  }

  ++ctx->cur_frame_line_count;
  return IMP_RET_SUCCESS;
}

// ---------------- imp_util routines

#ifdef _WIN32
bool imp_util_isatty(void) { return _isatty(_fileno(stdout)); }

bool imp_util_get_terminal_width(unsigned *out_term_width) {
  if (!out_term_width || !imp_util_isatty()) { return false; }
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  *out_term_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  return true;
}
#else
bool imp_util_isatty(void) { return isatty(fileno(stdout)); }

bool imp_util_get_terminal_width(unsigned *out_term_width) {
  if (!out_term_width || !imp_util_isatty()) { return false; }
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)) { return false; }
  *out_term_width = w.ws_col;
  return true;
}
#endif

// from https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c, updated to some of unicode 6.0
typedef struct unicode_codepoint_interval_16 {
  uint16_t first, last;
} unicode_codepoint_interval_16_t;

static unicode_codepoint_interval_16_t const s_non_spacing_char_ranges_16[] = {
  { 0x0300u, 0x036Fu }, { 0x0483u, 0x0486u }, { 0x0488u, 0x0489u }, { 0x0591u, 0x05BDu },
  { 0x05BFu, 0x05BFu }, { 0x05C1u, 0x05C2u }, { 0x05C4u, 0x05C5u }, { 0x05C7u, 0x05C7u },
  { 0x0600u, 0x0603u }, { 0x0610u, 0x0615u }, { 0x064Bu, 0x065Eu }, { 0x0670u, 0x0670u },
  { 0x06D6u, 0x06E4u }, { 0x06E7u, 0x06E8u }, { 0x06EAu, 0x06EDu }, { 0x070Fu, 0x070Fu },
  { 0x0711u, 0x0711u }, { 0x0730u, 0x074Au }, { 0x07A6u, 0x07B0u }, { 0x07EBu, 0x07F3u },
  { 0x0901u, 0x0902u }, { 0x093Cu, 0x093Cu }, { 0x0941u, 0x0948u }, { 0x094Du, 0x094Du },
  { 0x0951u, 0x0954u }, { 0x0962u, 0x0963u }, { 0x0981u, 0x0981u }, { 0x09BCu, 0x09BCu },
  { 0x09C1u, 0x09C4u }, { 0x09CDu, 0x09CDu }, { 0x09E2u, 0x09E3u }, { 0x0A01u, 0x0A02u },
  { 0x0A3Cu, 0x0A3Cu }, { 0x0A41u, 0x0A42u }, { 0x0A47u, 0x0A48u }, { 0x0A4Bu, 0x0A4Du },
  { 0x0A70u, 0x0A71u }, { 0x0A81u, 0x0A82u }, { 0x0ABCu, 0x0ABCu }, { 0x0AC1u, 0x0AC5u },
  { 0x0AC7u, 0x0AC8u }, { 0x0ACDu, 0x0ACDu }, { 0x0AE2u, 0x0AE3u }, { 0x0B01u, 0x0B01u },
  { 0x0B3Cu, 0x0B3Cu }, { 0x0B3Fu, 0x0B3Fu }, { 0x0B41u, 0x0B43u }, { 0x0B4Du, 0x0B4Du },
  { 0x0B56u, 0x0B56u }, { 0x0B82u, 0x0B82u }, { 0x0BC0u, 0x0BC0u }, { 0x0BCDu, 0x0BCDu },
  { 0x0C3Eu, 0x0C40u }, { 0x0C46u, 0x0C48u }, { 0x0C4Au, 0x0C4Du }, { 0x0C55u, 0x0C56u },
  { 0x0CBCu, 0x0CBCu }, { 0x0CBFu, 0x0CBFu }, { 0x0CC6u, 0x0CC6u }, { 0x0CCCu, 0x0CCDu },
  { 0x0CE2u, 0x0CE3u }, { 0x0D41u, 0x0D43u }, { 0x0D4Du, 0x0D4Du }, { 0x0DCAu, 0x0DCAu },
  { 0x0DD2u, 0x0DD4u }, { 0x0DD6u, 0x0DD6u }, { 0x0E31u, 0x0E31u }, { 0x0E34u, 0x0E3Au },
  { 0x0E47u, 0x0E4Eu }, { 0x0EB1u, 0x0EB1u }, { 0x0EB4u, 0x0EB9u }, { 0x0EBBu, 0x0EBCu },
  { 0x0EC8u, 0x0ECDu }, { 0x0F18u, 0x0F19u }, { 0x0F35u, 0x0F35u }, { 0x0F37u, 0x0F37u },
  { 0x0F39u, 0x0F39u }, { 0x0F71u, 0x0F7Eu }, { 0x0F80u, 0x0F84u }, { 0x0F86u, 0x0F87u },
  { 0x0F90u, 0x0F97u }, { 0x0F99u, 0x0FBCu }, { 0x0FC6u, 0x0FC6u }, { 0x102Du, 0x1030u },
  { 0x1032u, 0x1032u }, { 0x1036u, 0x1037u }, { 0x1039u, 0x1039u }, { 0x1058u, 0x1059u },
  { 0x1160u, 0x11FFu }, { 0x135Fu, 0x135Fu }, { 0x1712u, 0x1714u }, { 0x1732u, 0x1734u },
  { 0x1752u, 0x1753u }, { 0x1772u, 0x1773u }, { 0x17B4u, 0x17B5u }, { 0x17B7u, 0x17BDu },
  { 0x17C6u, 0x17C6u }, { 0x17C9u, 0x17D3u }, { 0x17DDu, 0x17DDu }, { 0x180Bu, 0x180Du },
  { 0x18A9u, 0x18A9u }, { 0x1920u, 0x1922u }, { 0x1927u, 0x1928u }, { 0x1932u, 0x1932u },
  { 0x1939u, 0x193Bu }, { 0x1A17u, 0x1A18u }, { 0x1B00u, 0x1B03u }, { 0x1B34u, 0x1B34u },
  { 0x1B36u, 0x1B3Au }, { 0x1B3Cu, 0x1B3Cu }, { 0x1B42u, 0x1B42u }, { 0x1B6Bu, 0x1B73u },
  { 0x1DC0u, 0x1DCAu }, { 0x1DFEu, 0x1DFFu }, { 0x200Bu, 0x200Fu }, { 0x202Au, 0x202Eu },
  { 0x2060u, 0x2063u }, { 0x206Au, 0x206Fu }, { 0x20D0u, 0x20EFu }, { 0x302Au, 0x302Fu },
  { 0x3099u, 0x309Au }, { 0xA806u, 0xA806u }, { 0xA80Bu, 0xA80Bu }, { 0xA825u, 0xA826u },
  { 0xFB1Eu, 0xFB1Eu }, { 0xFE00u, 0xFE0Fu }, { 0xFE20u, 0xFE23u }, { 0xFEFFu, 0xFEFFu },
  { 0xFFF9u, 0xFFFBu },
};

typedef struct unicode_codepoint_interval_32 {
  uint32_t first, last;
} unicode_codepoint_interval_32_t;

static unicode_codepoint_interval_32_t const s_non_spacing_char_ranges_32[] = {
  { 0x10A01, 0x10A03 }, { 0x10A05, 0x10A06 }, { 0x10A0C, 0x10A0F }, { 0x10A38, 0x10A3A },
  { 0x10A3F, 0x10A3F }, { 0x1D167, 0x1D169 }, { 0x1D173, 0x1D182 }, { 0x1D185, 0x1D18B },
  { 0x1D1AA, 0x1D1AD }, { 0x1D242, 0x1D244 }, { 0xE0001, 0xE0001 }, { 0xE0020, 0xE007F },
  { 0xE0100, 0xE01EF }
};

static int imp_util__wchar_is_non_spacing_char(wchar_t wc) {
  // binary search through the big 16-bit table
  unicode_codepoint_interval_16_t const *t16 = s_non_spacing_char_ranges_16;
  int min_idx = 0, max_idx = (sizeof(s_non_spacing_char_ranges_16) / sizeof(*t16)) - 1;
  if ((wc < t16[0].first) || (wc > t16[max_idx].last)) { return 0; }
  while (max_idx >= min_idx) {
    int const mid_idx = (min_idx + max_idx) / 2;
    if (wc > t16[mid_idx].last) {
      min_idx = mid_idx + 1;
    } else if (wc < t16[mid_idx].first) {
      max_idx = mid_idx - 1;
    } else {
      return 1;
    }
  }

  // linear scan w/early-out through the small 32-bit table
  unicode_codepoint_interval_32_t const *t32 = s_non_spacing_char_ranges_32;
  for (int i = 0, n = sizeof(s_non_spacing_char_ranges_32) / sizeof(*t32); i < n; ++i) {
    if (wc < (wchar_t)t32[i].first) { break; }
    if ((wc >= (wchar_t)t32[i].first) && (wc <= (wchar_t)t32[i].last)) { return 1; }
  }
  return 0;
}

static unicode_codepoint_interval_16_t const s_two_colum_fixed_width_cps[] = {
  {0x2460, 0x24ff},  // Enclosed Alphanumerics
  {0x2600, 0x26ff},  // Miscellaneous Symbols
  {0x2b00, 0x2bff},  // Miscellaneous Symbols and Arrows
  {0xac00, 0xd7a3},  // Hangul Syllables
  {0xf900, 0xfaff},  // CJK Compatibility Ideographs
  {0xfe10, 0xfe19},  // Vertical forms
  {0xfe30, 0xfe6f},  // CJK Compatibility Forms
  {0xff00, 0xff60},  // Fullwidth Forms
  {0xffe0, 0xffe6},
};

static int imp_util__wchar_display_width(wchar_t wc) {
  if (wc == 0) { return 0; }
  if ((wc < 32) || ((wc >= 0x7f) && (wc < 0xa0))) { return -1; }
  if (imp_util__wchar_is_non_spacing_char(wc)) { return 0; }

  // wc is not a combining or C0/C1 control character
  // todo: ZWJ here, lookahead on emoji plane

  unicode_codepoint_interval_16_t const *t16 = s_two_colum_fixed_width_cps;
  for (int i = 0, n = sizeof(s_two_colum_fixed_width_cps) / sizeof(*t16); i < n; ++i) {
    if (wc < t16[i].first) { break; }
    if ((wc >= t16[i].first) && (wc <= t16[i].last)) { return 2; }
  }

  if (wc < 0x1100) { return 1; }

  bool const two_col =
    (wc <= 0x115f || // Hangul Jamo init. consonants
     wc == 0x2329 || wc == 0x232a ||
     (wc >= 0x2e80 && wc <= 0xa4cf && wc != 0x303f) || // CJK ... Yi
     (wc >= 0x1f300 && wc <= 0x1f6ff) || // Misc symbols + emoticons + dingbats
     (wc >= 0x20000 && wc <= 0x2fffd) ||
     (wc >= 0x30000 && wc <= 0x3fffd));

  return two_col ? 2 : 1;
}

static int imp_util__wchar_from_utf8(unsigned char const *s, wchar_t *out) {
  unsigned char const *src = s;
  uint32_t cp = 0;
  while (*src) {
    unsigned char cur = *src;
    do {
      if (cur <= 0x7f) { cp = cur; break; }
      if (cur <= 0xbf) { cp = (cp << 6) | (cur & 0x3f); break; }
      if (cur <= 0xdf) { cp = cur & 0x1f; break; }
      if (cur <= 0xef) { cp = cur & 0x0f; break; }
      cp = cur & 0x07;
    } while (0);

    ++src;

    if (((*src & 0xc0) != 0x80) && (cp <= 0x10ffff)) {
      if (out) { *out = (wchar_t)cp; }
      break;
    }
  }
  return (int)((uintptr_t)src - (uintptr_t)s);
}

int imp_util_get_display_width(char const *utf8_str) {
  unsigned char const *src = (unsigned char const *)utf8_str;
  int w = 0;
  while (*src) {
    if (*src <= 0x7f) { ++src; ++w; continue; }
    wchar_t wc;
    src += imp_util__wchar_from_utf8(src, &wc);
    w += imp_util__wchar_display_width(wc);
  }
  return w;
}

