# ImProg Issues and Improvements

This document tracks correctness issues and potential improvements found in `improg.c` and `improg.h`.

## Critical Issues

### 1. Missing validation in `imp__value_write` for `IMP_UNIT_SIZE_DYNAMIC`
**Location:** `improg.c:104-122`

**Issue:** Lines 106, 111, 114 access `v->v.i` without first validating that `v->type == IMP_VALUE_TYPE_INT`. If a `DOUBLE` value is passed, it will read from the wrong union field, causing undefined behavior.

**Fix:** Add type validation and convert to int first before accessing union field.

```c
case IMP_UNIT_SIZE_DYNAMIC:
  // Add validation
  if (!imp__value_type_is_scalar(v)) { return IMP_RET_ERR_WRONG_VALUE_TYPE; }
  imp_value_t v_int;
  imp__value_to_int(v, &v_int);
  if (v_int.v.i < 1024) {
    conv_v = v_int;
    // ... rest of logic using v_int.v.i instead of v->v.i
  }
```

---

### 2. Buffer overflow risk in `imp_begin`
**Location:** `improg.c:567-570`

**Issue:** `snprintf(cmd, sizeof(cmd), IMP_PREVLINE, ctx->cur_frame_line_count - 1)` followed by manual null termination at line 569. If the formatted string is exactly 16 bytes, the null terminator at position 15 would be overwritten.

**Fix:** Use a larger buffer or check `snprintf` return value properly.

```c
char cmd[32];  // Increase buffer size
int len = snprintf(cmd, sizeof(cmd), IMP_PREVLINE, ctx->cur_frame_line_count - 1);
if (len >= (int)sizeof(cmd)) {
  // Handle error - shouldn't happen with larger buffer
}
```

---

### 3. Potential integer overflow in `imp__progress_bar` edge calculation
**Location:** `improg.c:478`

**Issue:** `imp__clamp(0, prog_w - (edge_w / 2), bar_w - edge_w)` - If `edge_w > bar_w`, the expression `bar_w - edge_w` underflows (negative result in int context, undefined for unsigned).

**Fix:** Ensure `edge_w` is validated before subtraction.

```c
int const edge_off = (edge_w > bar_w) ? 0 :
  imp__clamp(0, prog_w - (edge_w / 2), bar_w - edge_w);
```

---

### 4. Missing bounds check in `imp__string_write`
**Location:** `improg.c:296-299`

**Issue:** Character copy loop assumes `cp_len` is valid (0-4 bytes for UTF-8) but doesn't explicitly check array bounds for the `cp[5]` buffer.

**Fix:** Add assertion or explicit check that `cp_len < 5`.

```c
int const cp_len = imp_util__wchar_from_utf8(cur, NULL);
if (cp_len < 0 || cp_len > 4) { return -1; }  // Add validation
for (int cpi = 0; cpi < cp_len; ++cpi) { cp[cpi] = (char)cur[cpi]; }
```

---

## Logic Issues

### 5. Inconsistent field width handling in `imp__progress_percent_write`
**Location:** `improg.c:308-322`

**Issue:** Line 315 subtracts 1 from field_width to account for '%' character, but this is only applied when `have_fw && !have_pr` (line 320). The '%' character width is not consistently handled across all format branches.

**Fix:** Apply the field width adjustment consistently for all paths.

```c
// Calculate fw once, accounting for '%' suffix
int const fw = have_fw ? imp__max(0, p->field_width - 1) : 0;

if (!have_fw && !have_pr) { return snprintf(out_buf, buf_len, "%f%%", p_pct); }
if (!have_fw && have_pr) { return snprintf(out_buf, buf_len, "%.*f%%", pr, p_pct); }
if (have_fw && !have_pr) { return snprintf(out_buf, buf_len, "%*f%%", fw, p_pct); }
return snprintf(out_buf, buf_len, "%*.*f%%", fw, pr, p_pct);
```

---

### 6. Missing error handling in `imp_widget_display_width`
**Location:** `improg.c:359-366`

**Issue:** Line 363 returns `false` (0) on error from `imp__clipped_str_len`, but caller expects negative error codes for actual errors. This inconsistency could mask errors.

**Fix:** Return proper negative error code.

```c
case IMP_WIDGET_TYPE_STRING: {
  imp_widget_string_t const *s = &w->w.str;
  int str_len = 0;
  if (v && v->v.s) {
    if (!imp__clipped_str_len(v->v.s, s->max_len, 0, &str_len, NULL)) {
      return IMP_RET_ERR_ARGS;  // Return error code instead of false/0
    }
  }
  return imp__max(s->field_width, str_len);
}
```

---

### 7. Redundant double negation in `imp_draw_line`
**Location:** `improg.c:599`

**Issue:** `if ((bool)!!prog_max ^ (bool)!!prog_cur)` - The double negation `!!` is redundant since casting to bool already normalizes to 0/1.

**Fix:** Simplify to `(bool)prog_max ^ (bool)prog_cur`.

```c
if ((bool)prog_max ^ (bool)prog_cur) { return IMP_RET_ERR_ARGS; }
```

---

### 8. Division by zero risk in `imp_draw_line`
**Location:** `improg.c:606, 611`

**Issue:** If `prog_max->v.d` is 0.0 (line 606) or `prog_max->v.i` is 0 (line 611), division by zero occurs.

**Fix:** Add zero checks before division.

```c
float p = 0.f;
if (prog_cur) {
  if (prog_cur->type == IMP_VALUE_TYPE_DOUBLE) {
    if (prog_max->v.d != 0.0) {
      p = imp__clampf(0.f, (float)(prog_cur->v.d / prog_max->v.d), 1.f);
    }
  } else {
    if (prog_max->v.i != 0) {
      if (prog_cur->v.i >= prog_max->v.i) {
        p = 1.f;
      } else {
        p = imp__clampf(0.f, (float)prog_cur->v.i / (float)prog_max->v.i, 1.f);
      }
    }
  }
}
```

---

### 9. Missing division by zero check in `imp__spinner_get_string`
**Location:** `improg.c:64-66`

**Issue:** If `s->speed_msec` is 0, division by zero occurs on line 65.

**Fix:** Add validation that `speed_msec > 0`.

```c
static char const *imp__spinner_get_string(imp_widget_spinner_t const *s, unsigned msec) {
  if (s->speed_msec == 0) { return s->frames[0]; }  // Fallback to first frame
  unsigned const idx = (unsigned)((float)msec / (float)s->speed_msec);
  return s->frames[idx % (unsigned)s->frame_count];
}
```

---

## Code Quality Issues

### 10. Typo in variable name
**Location:** `improg.c:739`

**Issue:** Variable `s_two_colum_fixed_width_cps` should be `s_two_column_fixed_width_cps` (missing 'n' in "column").

**Fix:** Rename variable and all references.

---

### 11. Missing const qualifier
**Location:** `improg.c:169-177`

**Issue:** The `s_unit_suffixes` array is declared as non-const but should be const since it's never modified.

**Fix:** Add const qualifier.

```c
static char const * const s_unit_suffixes[] = {
  [IMP_UNIT_NONE] = "",
  [IMP_UNIT_SIZE_B] = "B",
  // ... rest
};
```

---

### 12. Manual string length calculation
**Location:** `improg.c:178-179`

**Issue:** Lines 178-179 implement a hand-rolled `strlen` instead of using standard library function.

**Fix:** Use `strlen()` for clarity (though current implementation avoids a dependency, so this is optional).

```c
char const *us = s_unit_suffixes[conv_u];
int const us_len = (int)strlen(us);
```

---

### 13. Potential NULL dereference in `imp__print`
**Location:** `improg.c:32-35`

**Issue:** Line 34 calls `imp_util_get_display_width(s)` without checking if `s` is NULL first. The function is called with NULL in some places (e.g., line 29 from default print callback, line 589).

**Fix:** Check `s != NULL` before calling `imp_util_get_display_width`.

```c
static void imp__print(imp_ctx_t *ctx, char const *s, int *dw) {
  ctx->print_cb(ctx->print_cb_ctx, s);
  if (s && dw) { *dw += imp_util_get_display_width(s); }
}
```

**Note:** This is already correctly implemented! Mark as verified.

---

### 14. Inconsistent error code returns
**Location:** Multiple functions

**Issue:** Some functions return `-1` on error (e.g., `imp__value_write`, `imp__scalar_write`), others return `IMP_RET_ERR_*` codes. This makes error handling inconsistent.

**Fix:** Consider standardizing on using the enum error codes throughout, or clearly document which functions use which convention.

---

## Potential Improvements

### 15. Missing return value checks for `snprintf`
**Location:** Multiple locations throughout `improg.c`

**Issue:** Many `snprintf` calls don't check return values. While generally safe due to buffer sizes, checking helps catch buffer size issues during development.

**Consider:** Adding assertions or debug checks on return values in key locations.

---

### 16. Magic number for UTF-8 buffer size
**Location:** `improg.c:296`

**Issue:** The array size `char cp[5]` assumes UTF-8 max of 4 bytes + null terminator, but this is not documented.

**Consider:** Adding a `#define UTF8_MAX_BYTES 4` or `#define UTF8_MAX_BUFFER 5` for clarity.

```c
#define IMP_UTF8_MAX_CODEPOINT_BYTES 4
// ...
char cp[IMP_UTF8_MAX_CODEPOINT_BYTES + 1];
```

---

### 17. Redundant conditional branches in `imp__value_write`
**Location:** `improg.c:191-194`

**Issue:** Four separate branches for combinations of `have_fw` and `have_pr` when formatting doubles. Similar pattern in other formatting functions.

**Consider:** Constructing format string dynamically to reduce code duplication (though current approach is more explicit and potentially more efficient).

---

## Status

- [ ] Issue 1: Missing validation in UNIT_SIZE_DYNAMIC
- [ ] Issue 2: Buffer overflow risk in imp_begin
- [ ] Issue 3: Integer overflow in progress_bar edge calculation
- [ ] Issue 4: Missing bounds check in string_write
- [ ] Issue 5: Inconsistent field width in progress_percent_write
- [ ] Issue 6: Missing error handling in widget_display_width
- [ ] Issue 7: Redundant double negation
- [ ] Issue 8: Division by zero in imp_draw_line
- [ ] Issue 9: Division by zero in spinner
- [ ] Issue 10: Typo in variable name
- [ ] Issue 11: Missing const qualifier
- [ ] Issue 12: Manual string length calculation
- [x] Issue 13: NULL check in imp__print (already correct)
- [ ] Issue 14: Inconsistent error codes
- [ ] Issue 15: Missing snprintf return checks
- [ ] Issue 16: Magic number for UTF-8 buffer
- [ ] Issue 17: Redundant conditional branches

## Priority

**High Priority (Fix First):**
- Issue 1 (Critical: Undefined behavior)
- Issue 2 (Critical: Buffer overflow)
- Issue 3 (Critical: Integer overflow)
- Issue 8 (Critical: Division by zero)
- Issue 9 (Critical: Division by zero)

**Medium Priority:**
- Issue 4 (Logic: Bounds checking)
- Issue 5 (Logic: Incorrect output)
- Issue 6 (Logic: Error handling)

**Low Priority (Code Quality):**
- Issues 7, 10, 11, 12, 14

**Optional Improvements:**
- Issues 15, 16, 17
