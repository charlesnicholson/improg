#include "improg.h"

#include <sys/ioctl.h>

#include <stdio.h>
#include <unistd.h>
#include <wchar.h>

static int imp__default_print_cb(void *ctx, char const *fmt, va_list args) {
  (void)ctx;
  if (!fmt) {
    fflush(stdout);
    return 0;
  }
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

  imp__print(ctx, IMP_FULL_HIDE_CURSOR IMP_FULL_AUTO_WRAP_DISABLE "\r");
  if (ctx->line_count > 1) {
    imp__print(ctx, IMP_FULL_PREVLINE, ctx->line_count - 1);
  }

  ctx->line_count = 0;
  return IMP_RET_SUCCESS;
}

imp_ret_t imp_draw_line(imp_ctx_t *ctx,
                        imp_value_t const *progress_cur,
                        imp_value_t const *progress_max,
                        imp_widget_def_t const *widgets,
                        int widget_count,
                        imp_value_t const *values,
                        int value_count) {
  if (!ctx) { return IMP_RET_ERR_ARGS; }
  if ((bool)progress_max ^ (bool)progress_cur) {
    return IMP_RET_ERR_ARGS;
  }
  if (progress_cur && (progress_cur->type == IMP_VALUE_TYPE_STR)) {
    return IMP_RET_ERR_ARGS;
  }
  if (progress_cur && (progress_cur->type != progress_max->type)) {
    return IMP_RET_ERR_ARGS;
  }

  double progress = -1.;
  if (progress_cur) {
    if (progress_cur->type == IMP_VALUE_TYPE_DOUBLE) {
      progress = progress_cur->v.d / progress_max->v.d;
      if (progress >= 1.) {
        progress = 1.;
      }
    } else {
      if (progress_cur->v.i >= progress_max->v.i) {
        progress = 1.;
      } else {
        progress = (double)progress_cur->v.i / (double)progress_max->v.i;
      }
    }
  }

  if (ctx->line_count) { imp__print(ctx, "\n"); }

  int coff = 0;
  for (int i = 0, val = 0; i < widget_count; ++i) {
    imp_widget_def_t const *w = &widgets[i];
    switch (w->type) {
      case IMP_WIDGET_TYPE_LABEL:
        imp__print(ctx, "%s", w->w.label.s);
        coff += (int)imp_util_get_display_width(w->w.label.s);
        break;

      case IMP_WIDGET_TYPE_STRING: {
        if (val >= value_count) { return IMP_RET_ERR_ARGS; }
        imp_value_t const *v = &values[val++];
        switch (v->type) {
          case IMP_VALUE_TYPE_STR:
            imp__print(ctx, "%s", v->v.s);
            coff += imp_util_get_display_width(v->v.s);
            break;
          case IMP_VALUE_TYPE_INT: imp__print(ctx, "%lld", v->v.i); break;
          case IMP_VALUE_TYPE_DOUBLE: imp__print(ctx, "%f", v->v.d); break;
        }
      } break;

      case IMP_WIDGET_TYPE_PROGRESS_PERCENT: {
        imp__print(ctx, "%6.2f%%", progress * 100.);
      } break;

      case IMP_WIDGET_TYPE_PROGRESS_BAR: {
        imp_widget_progress_bar_t const *pb = &w->w.progress_bar;
        imp__print(ctx, "%s", pb->left_end);
        coff += imp_util_get_display_width(pb->left_end);
        int const bar_w = (pb->field_width == -1) ?
          ((int)ctx->terminal_width - coff - imp_util_get_display_width(pb->right_end)) :
          pb->field_width;
        int const full_w = (int)(bar_w * progress);
        for (int fi = 0; fi < full_w; ++fi) { imp__print(ctx, "%s", pb->full_fill); }
        int const empty_w = (bar_w - full_w) < 0 ? 0 : (bar_w - full_w);
        for (int ei = 0; ei < empty_w; ++ei) { imp__print(ctx, "%s", pb->empty_fill); }
        coff += bar_w;
        imp__print(ctx, "%s", pb->right_end);
        coff += imp_util_get_display_width(pb->right_end);
      } break;

      case IMP_WIDGET_TYPE_SCALAR: break;

      case IMP_WIDGET_TYPE_SPINNER: {
        imp_widget_spinner_t const *s = &w->w.spinner;
        unsigned const frame = (ctx->ttl_elapsed_msec / s->speed_msec) % s->frame_count;
        imp__print(ctx, "%s", s->frames[frame]);
        coff += imp_util_get_display_width(s->frames[frame]);
      } break;

      case IMP_WIDGET_TYPE_FRACTION: break;
      case IMP_WIDGET_TYPE_STOPWATCH: break;
      case IMP_WIDGET_TYPE_PROGRESS_LABEL: break;
      case IMP_WIDGET_TYPE_PING_PONG_BAR: break;
      default: break;
    }
  }

  imp__print(ctx, IMP_FULL_ERASE_CURSOR_TO_END);
  ++ctx->line_count;
  return IMP_RET_SUCCESS;
}

imp_ret_t imp_end(imp_ctx_t *ctx, bool done) {
  if (!ctx) { return IMP_RET_ERR_ARGS; }
  ctx->ttl_elapsed_msec += ctx->dt_msec;
  ctx->dt_msec = 0;
  if (done) {
    imp__print(ctx, "\n" IMP_FULL_AUTO_WRAP_ENABLE IMP_FULL_SHOW_CURSOR);
  } else {
    imp__print(ctx, IMP_FULL_ERASE_CURSOR_TO_END);
  }
  imp__print(ctx, NULL);
  return IMP_RET_SUCCESS;
}

// ---------------- imp_util routines

bool imp_util_isatty(void) {
  return isatty(fileno(stdout));
}

bool imp_util_get_terminal_width(unsigned *out_term_width) {
  if (!out_term_width || !imp_util_isatty()) { return false; }
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)) {
    return false;
  }
  *out_term_width = w.ws_col;
  return true;
}

// from https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c, updated to unicode 6.0
typedef struct unicode_non_spacing_char_interval_16 {
  uint16_t first, last;
} unicode_non_spacing_char_interval_16_t;

static unicode_non_spacing_char_interval_16_t const s_non_spacing_char_ranges_16[] = {
  { 0x0300u, 0x036Fu },   { 0x0483u, 0x0486u },   { 0x0488u, 0x0489u },
  { 0x0591u, 0x05BDu },   { 0x05BFu, 0x05BFu },   { 0x05C1u, 0x05C2u },
  { 0x05C4u, 0x05C5u },   { 0x05C7u, 0x05C7u },   { 0x0600u, 0x0603u },
  { 0x0610u, 0x0615u },   { 0x064Bu, 0x065Eu },   { 0x0670u, 0x0670u },
  { 0x06D6u, 0x06E4u },   { 0x06E7u, 0x06E8u },   { 0x06EAu, 0x06EDu },
  { 0x070Fu, 0x070Fu },   { 0x0711u, 0x0711u },   { 0x0730u, 0x074Au },
  { 0x07A6u, 0x07B0u },   { 0x07EBu, 0x07F3u },   { 0x0901u, 0x0902u },
  { 0x093Cu, 0x093Cu },   { 0x0941u, 0x0948u },   { 0x094Du, 0x094Du },
  { 0x0951u, 0x0954u },   { 0x0962u, 0x0963u },   { 0x0981u, 0x0981u },
  { 0x09BCu, 0x09BCu },   { 0x09C1u, 0x09C4u },   { 0x09CDu, 0x09CDu },
  { 0x09E2u, 0x09E3u },   { 0x0A01u, 0x0A02u },   { 0x0A3Cu, 0x0A3Cu },
  { 0x0A41u, 0x0A42u },   { 0x0A47u, 0x0A48u },   { 0x0A4Bu, 0x0A4Du },
  { 0x0A70u, 0x0A71u },   { 0x0A81u, 0x0A82u },   { 0x0ABCu, 0x0ABCu },
  { 0x0AC1u, 0x0AC5u },   { 0x0AC7u, 0x0AC8u },   { 0x0ACDu, 0x0ACDu },
  { 0x0AE2u, 0x0AE3u },   { 0x0B01u, 0x0B01u },   { 0x0B3Cu, 0x0B3Cu },
  { 0x0B3Fu, 0x0B3Fu },   { 0x0B41u, 0x0B43u },   { 0x0B4Du, 0x0B4Du },
  { 0x0B56u, 0x0B56u },   { 0x0B82u, 0x0B82u },   { 0x0BC0u, 0x0BC0u },
  { 0x0BCDu, 0x0BCDu },   { 0x0C3Eu, 0x0C40u },   { 0x0C46u, 0x0C48u },
  { 0x0C4Au, 0x0C4Du },   { 0x0C55u, 0x0C56u },   { 0x0CBCu, 0x0CBCu },
  { 0x0CBFu, 0x0CBFu },   { 0x0CC6u, 0x0CC6u },   { 0x0CCCu, 0x0CCDu },
  { 0x0CE2u, 0x0CE3u },   { 0x0D41u, 0x0D43u },   { 0x0D4Du, 0x0D4Du },
  { 0x0DCAu, 0x0DCAu },   { 0x0DD2u, 0x0DD4u },   { 0x0DD6u, 0x0DD6u },
  { 0x0E31u, 0x0E31u },   { 0x0E34u, 0x0E3Au },   { 0x0E47u, 0x0E4Eu },
  { 0x0EB1u, 0x0EB1u },   { 0x0EB4u, 0x0EB9u },   { 0x0EBBu, 0x0EBCu },
  { 0x0EC8u, 0x0ECDu },   { 0x0F18u, 0x0F19u },   { 0x0F35u, 0x0F35u },
  { 0x0F37u, 0x0F37u },   { 0x0F39u, 0x0F39u },   { 0x0F71u, 0x0F7Eu },
  { 0x0F80u, 0x0F84u },   { 0x0F86u, 0x0F87u },   { 0x0F90u, 0x0F97u },
  { 0x0F99u, 0x0FBCu },   { 0x0FC6u, 0x0FC6u },   { 0x102Du, 0x1030u },
  { 0x1032u, 0x1032u },   { 0x1036u, 0x1037u },   { 0x1039u, 0x1039u },
  { 0x1058u, 0x1059u },   { 0x1160u, 0x11FFu },   { 0x135Fu, 0x135Fu },
  { 0x1712u, 0x1714u },   { 0x1732u, 0x1734u },   { 0x1752u, 0x1753u },
  { 0x1772u, 0x1773u },   { 0x17B4u, 0x17B5u },   { 0x17B7u, 0x17BDu },
  { 0x17C6u, 0x17C6u },   { 0x17C9u, 0x17D3u },   { 0x17DDu, 0x17DDu },
  { 0x180Bu, 0x180Du },   { 0x18A9u, 0x18A9u },   { 0x1920u, 0x1922u },
  { 0x1927u, 0x1928u },   { 0x1932u, 0x1932u },   { 0x1939u, 0x193Bu },
  { 0x1A17u, 0x1A18u },   { 0x1B00u, 0x1B03u },   { 0x1B34u, 0x1B34u },
  { 0x1B36u, 0x1B3Au },   { 0x1B3Cu, 0x1B3Cu },   { 0x1B42u, 0x1B42u },
  { 0x1B6Bu, 0x1B73u },   { 0x1DC0u, 0x1DCAu },   { 0x1DFEu, 0x1DFFu },
  { 0x200Bu, 0x200Fu },   { 0x202Au, 0x202Eu },   { 0x2060u, 0x2063u },
  { 0x206Au, 0x206Fu },   { 0x20D0u, 0x20EFu },   { 0x302Au, 0x302Fu },
  { 0x3099u, 0x309Au },   { 0xA806u, 0xA806u },   { 0xA80Bu, 0xA80Bu },
  { 0xA825u, 0xA826u },   { 0xFB1Eu, 0xFB1Eu },   { 0xFE00u, 0xFE0Fu },
  { 0xFE20u, 0xFE23u },   { 0xFEFFu, 0xFEFFu },   { 0xFFF9u, 0xFFFBu },
};

typedef struct unicode_non_spacing_char_interval_32 {
  int first, last;
} unicode_non_spacing_char_interval_32_t;

static unicode_non_spacing_char_interval_32_t const s_non_spacing_char_ranges_32[] = {
  { 0x10A01, 0x10A03 }, { 0x10A05, 0x10A06 }, { 0x10A0C, 0x10A0F },
  { 0x10A38, 0x10A3A }, { 0x10A3F, 0x10A3F }, { 0x1D167, 0x1D169 },
  { 0x1D173, 0x1D182 }, { 0x1D185, 0x1D18B }, { 0x1D1AA, 0x1D1AD },
  { 0x1D242, 0x1D244 }, { 0xE0001, 0xE0001 }, { 0xE0020, 0xE007F },
  { 0xE0100, 0xE01EF }
};

static int imp_util__wchar_is_non_spacing_char(wchar_t wc) {
  unicode_non_spacing_char_interval_16_t const *t16 = s_non_spacing_char_ranges_16;
  int max_idx = (sizeof(s_non_spacing_char_ranges_16) / sizeof(*t16)) - 1;

  if ((wc < t16[0].first) || (wc > t16[max_idx].last)) { return 0; }

  int min_idx = 0;
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

  unicode_non_spacing_char_interval_32_t const *t32 = s_non_spacing_char_ranges_32;
  int const n32 = sizeof(s_non_spacing_char_ranges_32) / sizeof(*t32);

  for (int i = 0; i < n32; ++i) {
    if ((wc >= t32[i].first) && (wc <= t32[i].last)) { return 1; }
  }

  return 0;
}

static int imp_util__wchar_display_width(wchar_t wc) {
  if (wc == 0) { return 0; }
  if ((wc < 32) || ((wc >= 0x7f) && (wc < 0xa0))) { return -1; }
  if (imp_util__wchar_is_non_spacing_char(wc)) { return 0; }

  // wc is not a combining or C0/C1 control character

  // todo: ZWJ here, lookahead on emoji plane

  return 1 +
    (wc >= 0x1100 &&
      (wc <= 0x115f || // Hangul Jamo init. consonants
       wc == 0x2329 || wc == 0x232a ||
       (wc >= 0x2e80 && wc <= 0xa4cf && wc != 0x303f) || // CJK ... Yi
       (wc >= 0xac00 && wc <= 0xd7a3) ||   // Hangul Syllables
       (wc >= 0xf900 && wc <= 0xfaff) ||   // CJK Compatibility Ideographs
       (wc >= 0xfe10 && wc <= 0xfe19) ||   // Vertical forms
       (wc >= 0xfe30 && wc <= 0xfe6f) ||   // CJK Compatibility Forms
       (wc >= 0xff00 && wc <= 0xff60) ||   // Fullwidth Forms
       (wc >= 0xffe0 && wc <= 0xffe6) ||
       (wc >= 0x1f300 && wc <= 0x1f6ff) || // Misc symbols + emoticons + dingbats
       (wc >= 0x20000 && wc <= 0x2fffd) ||
       (wc >= 0x30000 && wc <= 0x3fffd)));
}

static int wchar_from_utf8(unsigned char const *s, wchar_t *out) {
  unsigned char const *src = s;
  uint32_t cp = 0;

  while (*src) {
    unsigned char cur = *src;
    if (cur <= 0x7f) {
      cp = cur;
    } else if (cur <= 0xbf) {
      cp = (cp << 6) | (cur & 0x3f);
    } else if (cur <= 0xdf) {
      cp = cur & 0x1f;
    } else if (cur <= 0xef) {
      cp = cur & 0x0f;
    } else {
      cp = cur & 0x07;
    }

    ++src;

    if (((*src & 0xc0) != 0x80) && (cp <= 0x10ffff)) {
      *out = (wchar_t)cp;
      break;
    }
  }

  return (int)(src - (unsigned char const *)s);
}

int imp_util_get_display_width(char const *utf8_str) {
  unsigned char const *src = (unsigned char const *)utf8_str;
  int w = 0;
  while (*src) {
    if (*src <= 0x7f) { // ascii fast path
      ++src;
      ++w;
    } else { // convert 1-4 UTF-8 bytes to wchar_t, get display width.
      wchar_t wc;
      src += wchar_from_utf8(src, &wc);
      w += imp_util__wchar_display_width(wc);
    }
  }

  return w;
}
