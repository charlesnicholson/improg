#include "improg.h"

#include <sys/ioctl.h>

#include <stdio.h>
#include <unistd.h>
#include <wchar.h>

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

imp_ret_t imp_drawline(imp_ctx_t *ctx,
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

  for (int i = 0, val = 0; i < widget_count; ++i) {
    imp_widget_def_t const *w = &widgets[i];
    switch (w->type) {
      case IMP_WIDGET_TYPE_LABEL:
        imp__print(ctx, "%s", w->w.label.s);
        break;

      case IMP_WIDGET_TYPE_STRING: {
        if (val >= value_count) { return IMP_RET_ERR_ARGS; }
        imp_value_t const *v = &values[val++];
        switch (v->type) {
          case IMP_VALUE_TYPE_STR: imp__print(ctx, "%s", v->v.s); break;
          case IMP_VALUE_TYPE_INT: imp__print(ctx, "%lld", v->v.i); break;
          case IMP_VALUE_TYPE_DOUBLE: imp__print(ctx, "%f", v->v.d); break;
        }
      } break;

      case IMP_WIDGET_TYPE_PROGRESS_PERCENT: {
        imp__print(ctx, "%3.2f%%", progress * 100.);
      } break;

      case IMP_WIDGET_TYPE_PROGRESS_BAR: {
        imp__print(ctx, w->w.progress_bar.left_end);
        int const full_w = w->w.progress_bar.field_width * progress;
        for (int i = 0; i < full_w; ++i) {
          imp__print(ctx, "%s", w->w.progress_bar.full_fill);
        }
        int empty_w = w->w.progress_bar.field_width - full_w;
        if (empty_w < 0) { empty_w = 0; }
        for (int i = 0; i < empty_w; ++i) {
          imp__print(ctx, "%s", w->w.progress_bar.empty_fill);
        }
        imp__print(ctx, w->w.progress_bar.right_end);
      } break;

      default:
        break;
    }
  }

  imp__print(ctx, IMP_FULL_ERASE_CURSOR_TO_END "\n");
  ++ctx->line_count;
  return IMP_RET_SUCCESS;
}

imp_ret_t imp_end(imp_ctx_t *ctx) {
  if (!ctx) { return IMP_RET_ERR_ARGS; }
  ctx->ttl_elapsed_msec += ctx->dt_msec;
  ctx->dt_msec = 0;
  if (ctx->line_count) {
    imp__print(ctx, IMP_FULL_PREVLINE, ctx->line_count);
  }
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
typedef struct unicode_non_spacing_char_interval {
  int first, last;
} unicode_non_spacing_char_interval_t;

static unicode_non_spacing_char_interval_t const s_non_spacing_char_ranges[] = {
  { 0x0300, 0x036F },   { 0x0483, 0x0486 },   { 0x0488, 0x0489 },
  { 0x0591, 0x05BD },   { 0x05BF, 0x05BF },   { 0x05C1, 0x05C2 },
  { 0x05C4, 0x05C5 },   { 0x05C7, 0x05C7 },   { 0x0600, 0x0603 },
  { 0x0610, 0x0615 },   { 0x064B, 0x065E },   { 0x0670, 0x0670 },
  { 0x06D6, 0x06E4 },   { 0x06E7, 0x06E8 },   { 0x06EA, 0x06ED },
  { 0x070F, 0x070F },   { 0x0711, 0x0711 },   { 0x0730, 0x074A },
  { 0x07A6, 0x07B0 },   { 0x07EB, 0x07F3 },   { 0x0901, 0x0902 },
  { 0x093C, 0x093C },   { 0x0941, 0x0948 },   { 0x094D, 0x094D },
  { 0x0951, 0x0954 },   { 0x0962, 0x0963 },   { 0x0981, 0x0981 },
  { 0x09BC, 0x09BC },   { 0x09C1, 0x09C4 },   { 0x09CD, 0x09CD },
  { 0x09E2, 0x09E3 },   { 0x0A01, 0x0A02 },   { 0x0A3C, 0x0A3C },
  { 0x0A41, 0x0A42 },   { 0x0A47, 0x0A48 },   { 0x0A4B, 0x0A4D },
  { 0x0A70, 0x0A71 },   { 0x0A81, 0x0A82 },   { 0x0ABC, 0x0ABC },
  { 0x0AC1, 0x0AC5 },   { 0x0AC7, 0x0AC8 },   { 0x0ACD, 0x0ACD },
  { 0x0AE2, 0x0AE3 },   { 0x0B01, 0x0B01 },   { 0x0B3C, 0x0B3C },
  { 0x0B3F, 0x0B3F },   { 0x0B41, 0x0B43 },   { 0x0B4D, 0x0B4D },
  { 0x0B56, 0x0B56 },   { 0x0B82, 0x0B82 },   { 0x0BC0, 0x0BC0 },
  { 0x0BCD, 0x0BCD },   { 0x0C3E, 0x0C40 },   { 0x0C46, 0x0C48 },
  { 0x0C4A, 0x0C4D },   { 0x0C55, 0x0C56 },   { 0x0CBC, 0x0CBC },
  { 0x0CBF, 0x0CBF },   { 0x0CC6, 0x0CC6 },   { 0x0CCC, 0x0CCD },
  { 0x0CE2, 0x0CE3 },   { 0x0D41, 0x0D43 },   { 0x0D4D, 0x0D4D },
  { 0x0DCA, 0x0DCA },   { 0x0DD2, 0x0DD4 },   { 0x0DD6, 0x0DD6 },
  { 0x0E31, 0x0E31 },   { 0x0E34, 0x0E3A },   { 0x0E47, 0x0E4E },
  { 0x0EB1, 0x0EB1 },   { 0x0EB4, 0x0EB9 },   { 0x0EBB, 0x0EBC },
  { 0x0EC8, 0x0ECD },   { 0x0F18, 0x0F19 },   { 0x0F35, 0x0F35 },
  { 0x0F37, 0x0F37 },   { 0x0F39, 0x0F39 },   { 0x0F71, 0x0F7E },
  { 0x0F80, 0x0F84 },   { 0x0F86, 0x0F87 },   { 0x0F90, 0x0F97 },
  { 0x0F99, 0x0FBC },   { 0x0FC6, 0x0FC6 },   { 0x102D, 0x1030 },
  { 0x1032, 0x1032 },   { 0x1036, 0x1037 },   { 0x1039, 0x1039 },
  { 0x1058, 0x1059 },   { 0x1160, 0x11FF },   { 0x135F, 0x135F },
  { 0x1712, 0x1714 },   { 0x1732, 0x1734 },   { 0x1752, 0x1753 },
  { 0x1772, 0x1773 },   { 0x17B4, 0x17B5 },   { 0x17B7, 0x17BD },
  { 0x17C6, 0x17C6 },   { 0x17C9, 0x17D3 },   { 0x17DD, 0x17DD },
  { 0x180B, 0x180D },   { 0x18A9, 0x18A9 },   { 0x1920, 0x1922 },
  { 0x1927, 0x1928 },   { 0x1932, 0x1932 },   { 0x1939, 0x193B },
  { 0x1A17, 0x1A18 },   { 0x1B00, 0x1B03 },   { 0x1B34, 0x1B34 },
  { 0x1B36, 0x1B3A },   { 0x1B3C, 0x1B3C },   { 0x1B42, 0x1B42 },
  { 0x1B6B, 0x1B73 },   { 0x1DC0, 0x1DCA },   { 0x1DFE, 0x1DFF },
  { 0x200B, 0x200F },   { 0x202A, 0x202E },   { 0x2060, 0x2063 },
  { 0x206A, 0x206F },   { 0x20D0, 0x20EF },   { 0x302A, 0x302F },
  { 0x3099, 0x309A },   { 0xA806, 0xA806 },   { 0xA80B, 0xA80B },
  { 0xA825, 0xA826 },   { 0xFB1E, 0xFB1E },   { 0xFE00, 0xFE0F },
  { 0xFE20, 0xFE23 },   { 0xFEFF, 0xFEFF },   { 0xFFF9, 0xFFFB },
  { 0x10A01, 0x10A03 }, { 0x10A05, 0x10A06 }, { 0x10A0C, 0x10A0F },
  { 0x10A38, 0x10A3A }, { 0x10A3F, 0x10A3F }, { 0x1D167, 0x1D169 },
  { 0x1D173, 0x1D182 }, { 0x1D185, 0x1D18B }, { 0x1D1AA, 0x1D1AD },
  { 0x1D242, 0x1D244 }, { 0xE0001, 0xE0001 }, { 0xE0020, 0xE007F },
  { 0xE0100, 0xE01EF }
};

static int imp_util__wchar_is_non_spacing_char(wchar_t wc) {
  unicode_non_spacing_char_interval_t const *table = s_non_spacing_char_ranges;
  int max_idx =
    (sizeof(s_non_spacing_char_ranges) / sizeof(unicode_non_spacing_char_interval_t)) - 1;

  if ((wc < table[0].first) || (wc > table[max_idx].last)) { return 0; }

  int min_idx = 0;
  while (max_idx >= min_idx) {
    int const mid_idx = (min_idx + max_idx) / 2;
    if (wc > table[mid_idx].last) {
      min_idx = mid_idx + 1;
    } else if (wc < table[mid_idx].first) {
      max_idx = mid_idx - 1;
    } else {
      return 1;
    }
  }

  return 0;
}

static int imp_util__wchar_display_width(wchar_t wc) {
  if (wc == 0) { return 0; }
  if ((wc < 32) || ((wc >= 0x7f) && (wc < 0xa0))) { return -1; }
  if (imp_util__wchar_is_non_spacing_char(wc)) { return 0; }

  // wc is not a combining or C0/C1 control character

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

  return src - (unsigned char const *)s;
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

