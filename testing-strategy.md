# ImProg Unit Testing Strategy

This document outlines a comprehensive testing strategy for the ImProg library, which requires careful validation due to its subtle UTF-8 width calculations, precise layout math, and terminal control sequences.

## Testing Philosophy

**Key Challenges:**
1. **Precise display width calculations** - Unicode graphemes have varying widths (0, 1, or 2 columns)
2. **Terminal escape sequences** - Must validate correct ANSI codes without a real terminal
3. **Edge cases in formatting** - Field widths, precision, truncation, alignment
4. **Progress bar math** - Pixel-perfect layout with edge fills and dynamic widths
5. **Value conversions** - Type coercion between int/double with units

**Approach:**
- **Capture and parse output** instead of rendering to terminal
- **Mock print callback** to intercept all output strings
- **Golden file testing** for complex visual layouts
- **Property-based testing** for math invariants
- **Boundary testing** for edge cases

---

## Test Infrastructure

### 1. Mock Print Callback

Create a test harness that captures output instead of printing:

```c
typedef struct {
  char buffer[4096];
  size_t offset;
  bool overflow;
} test_capture_t;

void test_print_cb(void *ctx, char const *s) {
  test_capture_t *cap = (test_capture_t *)ctx;
  if (!s) { return; }  // flush

  size_t len = strlen(s);
  if (cap->offset + len >= sizeof(cap->buffer)) {
    cap->overflow = true;
    return;
  }

  memcpy(cap->buffer + cap->offset, s, len);
  cap->offset += len;
  cap->buffer[cap->offset] = '\0';
}
```

### 2. Output Parser

Parse captured output to extract:
- Display width (visible characters, excluding ANSI codes)
- ANSI escape sequences
- Actual text content

```c
typedef struct {
  char *text;           // Visible text only
  char **ansi_codes;    // Array of escape sequences
  int ansi_count;
  int display_width;    // Calculated display width
} parsed_output_t;

parsed_output_t parse_output(char const *raw);
```

### 3. Assertion Helpers

```c
// Assert display width matches expected
void assert_display_width(char const *output, int expected);

// Assert output contains specific ANSI codes
void assert_has_ansi(char const *output, char const *code);

// Assert output matches visual golden file
void assert_matches_golden(char const *output, char const *golden_file);

// Assert numeric output within tolerance
void assert_value_near(char const *output, double expected, double epsilon);
```

---

## Test Categories

## Category 1: UTF-8 Display Width Calculation

These tests validate `imp_util_get_display_width()` and the underlying Unicode width logic.

### Test Cases:

```c
void test_utf8_ascii_width(void) {
  // ASCII characters are 1 column each
  assert_eq(imp_util_get_display_width("hello"), 5);
  assert_eq(imp_util_get_display_width(""), 0);
  assert_eq(imp_util_get_display_width("a"), 1);
}

void test_utf8_wide_chars(void) {
  // CJK characters are 2 columns
  assert_eq(imp_util_get_display_width("你好"), 4);  // 2 chars × 2 columns
  assert_eq(imp_util_get_display_width("😀"), 2);    // Emoji are 2 columns
  assert_eq(imp_util_get_display_width("日本語"), 6); // 3 chars × 2 columns
}

void test_utf8_combining_chars(void) {
  // Combining characters are 0 columns
  assert_eq(imp_util_get_display_width("é"), 1);      // e + combining acute
  assert_eq(imp_util_get_display_width("a\u0301"), 1); // a + combining acute
}

void test_utf8_mixed_width(void) {
  assert_eq(imp_util_get_display_width("Hello世界"), 9); // 5 + 4
  assert_eq(imp_util_get_display_width("a😀b"), 4);      // 1 + 2 + 1
}

void test_utf8_zero_width(void) {
  // Zero-width characters
  assert_eq(imp_util_get_display_width("a\u200Bb"), 2);  // a + ZWS + b
  assert_eq(imp_util_get_display_width("\uFEFF"), 0);    // Zero-width no-break space
}

void test_utf8_hangul(void) {
  // Hangul syllables are 2 columns
  assert_eq(imp_util_get_display_width("한글"), 4);
}

void test_utf8_control_chars(void) {
  // Control characters should return -1 for width
  // (Need to check if imp_util__wchar_display_width is exposed or test indirectly)
}
```

**Test Data Sources:**
- Unicode 15.0 East Asian Width property data
- Common emoji sequences (including ZWJ sequences)
- Combining diacritical marks (U+0300 - U+036F)
- Arabic/Hebrew text (RTL considerations if relevant)

---

## Category 2: Value Formatting and Unit Conversion

Test `imp__value_write()` and related formatting functions.

### Test Cases:

```c
void test_format_int_basic(void) {
  char buf[64];
  imp_value_t v = IMP_VALUE_INT(42);
  int len = imp__value_write(-1, -1, IMP_UNIT_NONE, &v, buf, sizeof(buf));
  assert_str_eq(buf, "42");
  assert_eq(len, 2);
}

void test_format_double_precision(void) {
  char buf[64];
  imp_value_t v = IMP_VALUE_DOUBLE(3.14159);

  // Default precision
  imp__value_write(-1, -1, IMP_UNIT_NONE, &v, buf, sizeof(buf));
  assert_str_eq(buf, "3.141590");  // Default %f precision

  // Custom precision
  imp__value_write(-1, 2, IMP_UNIT_NONE, &v, buf, sizeof(buf));
  assert_str_eq(buf, "3.14");
}

void test_format_field_width(void) {
  char buf[64];
  imp_value_t v = IMP_VALUE_INT(42);

  // Right-aligned with padding
  imp__value_write(5, -1, IMP_UNIT_NONE, &v, buf, sizeof(buf));
  assert_str_eq(buf, "   42");

  // Width smaller than value
  imp__value_write(1, -1, IMP_UNIT_NONE, &v, buf, sizeof(buf));
  assert_str_eq(buf, "42");  // Should not truncate
}

void test_format_size_units(void) {
  char buf[64];

  // Bytes
  imp_value_t v1 = IMP_VALUE_INT(1024);
  imp__value_write(-1, -1, IMP_UNIT_SIZE_B, &v1, buf, sizeof(buf));
  assert_str_eq(buf, "1024B");

  // Kilobytes
  imp__value_write(-1, 2, IMP_UNIT_SIZE_KB, &v1, buf, sizeof(buf));
  assert_str_eq(buf, "1.00KB");

  // Megabytes
  imp_value_t v2 = IMP_VALUE_INT(1048576);
  imp__value_write(-1, 2, IMP_UNIT_SIZE_MB, &v2, buf, sizeof(buf));
  assert_str_eq(buf, "1.00MB");
}

void test_format_size_dynamic(void) {
  char buf[64];

  // Should choose B
  imp_value_t v1 = IMP_VALUE_INT(512);
  imp__value_write(-1, -1, IMP_UNIT_SIZE_DYNAMIC, &v1, buf, sizeof(buf));
  assert_str_eq(buf, "512B");

  // Should choose KB
  imp_value_t v2 = IMP_VALUE_INT(2048);
  imp__value_write(-1, 2, IMP_UNIT_SIZE_DYNAMIC, &v2, buf, sizeof(buf));
  assert_str_eq(buf, "2.00KB");

  // Should choose MB
  imp_value_t v3 = IMP_VALUE_INT(5242880);
  imp__value_write(-1, 2, IMP_UNIT_SIZE_DYNAMIC, &v3, buf, sizeof(buf));
  assert_str_eq(buf, "5.00MB");

  // Should choose GB
  imp_value_t v4 = IMP_VALUE_INT(2147483648LL);
  imp__value_write(-1, 2, IMP_UNIT_SIZE_DYNAMIC, &v4, buf, sizeof(buf));
  assert_str_eq(buf, "2.00GB");
}

void test_format_time_seconds(void) {
  char buf[64];
  imp_value_t v = IMP_VALUE_INT(125);
  imp__value_write(-1, -1, IMP_UNIT_TIME_SEC, &v, buf, sizeof(buf));
  assert_str_eq(buf, "125s");
}

void test_format_time_hms_letters(void) {
  char buf[64];

  // Just seconds
  imp_value_t v1 = IMP_VALUE_INT(45);
  imp__value_write(-1, -1, IMP_UNIT_TIME_HMS_LETTERS, &v1, buf, sizeof(buf));
  assert_str_eq(buf, "45s");

  // Minutes and seconds
  imp_value_t v2 = IMP_VALUE_INT(125);
  imp__value_write(-1, -1, IMP_UNIT_TIME_HMS_LETTERS, &v2, buf, sizeof(buf));
  assert_str_eq(buf, "2m5s");

  // Hours, minutes, seconds
  imp_value_t v3 = IMP_VALUE_INT(3665);
  imp__value_write(-1, -1, IMP_UNIT_TIME_HMS_LETTERS, &v3, buf, sizeof(buf));
  assert_str_eq(buf, "1h1m5s");

  // Zero values
  imp_value_t v4 = IMP_VALUE_INT(3600);
  imp__value_write(-1, -1, IMP_UNIT_TIME_HMS_LETTERS, &v4, buf, sizeof(buf));
  assert_str_eq(buf, "1h0m0s");
}

void test_format_time_hms_colons(void) {
  char buf[64];

  imp_value_t v1 = IMP_VALUE_INT(45);
  imp__value_write(-1, -1, IMP_UNIT_TIME_HMS_COLONS, &v1, buf, sizeof(buf));
  assert_str_eq(buf, "00:00:45");

  imp_value_t v2 = IMP_VALUE_INT(3665);
  imp__value_write(-1, -1, IMP_UNIT_TIME_HMS_COLONS, &v2, buf, sizeof(buf));
  assert_str_eq(buf, "01:01:05");

  // With field width
  imp__value_write(10, -1, IMP_UNIT_TIME_HMS_COLONS, &v2, buf, sizeof(buf));
  assert_str_eq(buf, "  01:01:05");
}

void test_format_type_conversion(void) {
  char buf[64];

  // Double to int conversion for byte sizes
  imp_value_t v1 = IMP_VALUE_DOUBLE(1024.7);
  imp__value_write(-1, -1, IMP_UNIT_SIZE_B, &v1, buf, sizeof(buf));
  assert_str_eq(buf, "1024B");  // Should truncate to int

  // Int to double conversion for KB
  imp_value_t v2 = IMP_VALUE_INT(2048);
  imp__value_write(-1, 2, IMP_UNIT_SIZE_KB, &v2, buf, sizeof(buf));
  assert_str_eq(buf, "2.00KB");
}
```

**Edge Cases to Test:**
- Very large numbers (INT64_MAX, near-overflow)
- Very small numbers (0, negative if applicable)
- Precision edge cases (0 precision, very high precision)
- Field width edge cases (0, 1, very large)
- Buffer size edge cases (write with buf_len = 0, small buffers)

---

## Category 3: String Widget Trimming and Clipping

Test `imp__string_write()` and `imp__clipped_str_len()`.

### Test Cases:

```c
void test_string_no_trim(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);

  imp_widget_def_t w = IMP_WIDGET_STRING(-1, -1);
  imp_value_t v = IMP_VALUE_STRING("hello");

  imp__string_write(&ctx, &w.w.str, &v);
  assert_str_eq(cap.buffer, "hello");
}

void test_string_max_length_ascii(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);

  imp_widget_def_t w = IMP_WIDGET_STRING(-1, 3);
  imp_value_t v = IMP_VALUE_STRING("hello");

  imp__string_write(&ctx, &w.w.str, &v);
  assert_str_eq(cap.buffer, "hel");
}

void test_string_max_length_unicode(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);

  // Max length 3 columns, "你" is 2 columns, "好" is 2 columns
  // Should fit "你" (2 cols) + partial fit? or just "你"?
  imp_widget_def_t w = IMP_WIDGET_STRING(-1, 3);
  imp_value_t v = IMP_VALUE_STRING("你好");

  imp__string_write(&ctx, &w.w.str, &v);
  // Should contain only "你" if it stops at 2 columns before exceeding 3
  // Need to verify exact behavior
}

void test_string_custom_trim_right(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);

  imp_widget_def_t w = IMP_WIDGET_STRING_CUSTOM_TRIM(-1, 5, "...", false);
  imp_value_t v = IMP_VALUE_STRING("hello world");

  imp__string_write(&ctx, &w.w.str, &v);
  assert_str_eq(cap.buffer, "he...");  // 2 chars + 3 char trim
}

void test_string_custom_trim_left(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);

  imp_widget_def_t w = IMP_WIDGET_STRING_CUSTOM_TRIM(-1, 5, "...", true);
  imp_value_t v = IMP_VALUE_STRING("hello world");

  imp__string_write(&ctx, &w.w.str, &v);
  assert_str_eq(cap.buffer, "...ld");  // 3 char trim + 2 chars
}

void test_string_field_width_padding(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);

  imp_widget_def_t w = IMP_WIDGET_STRING(10, -1);
  imp_value_t v = IMP_VALUE_STRING("hi");

  imp__string_write(&ctx, &w.w.str, &v);
  assert_str_eq(cap.buffer, "        hi");  // 8 spaces + "hi"
}

void test_string_trim_with_combining_chars(void) {
  // Test that combining characters don't break trimming
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);

  imp_widget_def_t w = IMP_WIDGET_STRING(-1, 3);
  imp_value_t v = IMP_VALUE_STRING("café");  // e + combining accent

  imp__string_write(&ctx, &w.w.str, &v);
  // Should trim to "caf" (3 display columns)
}
```

**Property-Based Tests:**
- For any string and max_len, output display width ≤ max_len
- For any string and field_width, output display width ≥ field_width (with padding)
- Custom trim should always fit within max_len

---

## Category 4: Progress Bar Layout Math

Test `imp__draw_widget()` for progress bars - the most complex layout logic.

### Test Cases:

```c
void test_progress_bar_empty(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  imp_widget_def_t w = IMP_WIDGET_PROGRESS_BAR(20, "[", "]", "=", " ", NULL);
  imp_value_t v = IMP_VALUE_NULL();

  imp_draw_line(&ctx,
                &(imp_value_t)IMP_VALUE_INT(0),
                &(imp_value_t)IMP_VALUE_INT(100),
                &w, &v);

  parsed_output_t out = parse_output(cap.buffer);
  assert_str_eq(out.text, "[                    ]");  // 20 spaces
}

void test_progress_bar_full(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  imp_widget_def_t w = IMP_WIDGET_PROGRESS_BAR(20, "[", "]", "=", " ", NULL);
  imp_value_t v = IMP_VALUE_NULL();

  imp_draw_line(&ctx,
                &(imp_value_t)IMP_VALUE_INT(100),
                &(imp_value_t)IMP_VALUE_INT(100),
                &w, &v);

  parsed_output_t out = parse_output(cap.buffer);
  assert_str_eq(out.text, "[====================]");  // 20 '='
}

void test_progress_bar_partial(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  imp_widget_def_t w = IMP_WIDGET_PROGRESS_BAR(20, "[", "]", "=", " ", NULL);
  imp_value_t v = IMP_VALUE_NULL();

  imp_draw_line(&ctx,
                &(imp_value_t)IMP_VALUE_INT(50),
                &(imp_value_t)IMP_VALUE_INT(100),
                &w, &v);

  parsed_output_t out = parse_output(cap.buffer);
  assert_str_eq(out.text, "[==========          ]");  // 10 '=' + 10 spaces
}

void test_progress_bar_edge_fill(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  imp_widget_def_t edge = IMP_WIDGET_LABEL(">");
  imp_widget_def_t w = IMP_WIDGET_PROGRESS_BAR(20, "[", "]", "=", " ", &edge);
  imp_value_t v = IMP_VALUE_NULL();

  imp_draw_line(&ctx,
                &(imp_value_t)IMP_VALUE_INT(50),
                &(imp_value_t)IMP_VALUE_INT(100),
                &w, &v);

  parsed_output_t out = parse_output(cap.buffer);
  assert_str_eq(out.text, "[=========>          ]");  // Edge at 50%
}

void test_progress_bar_scale_edge_fill(void) {
  // Test scale_fill feature with a multi-character edge
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  imp_widget_def_t edge = IMP_WIDGET_SPINNER(100, 4, {"⠁", "⠂", "⠄", "⡀"});
  imp_widget_def_t w = IMP_WIDGET_PROGRESS_BAR_SCALE_EDGE_FILL(
    20, "[", "]", "=", " ", &edge);
  imp_value_t edge_val = IMP_VALUE_INT(150);  // 150ms = frame 1

  imp_draw_line(&ctx,
                &(imp_value_t)IMP_VALUE_INT(50),
                &(imp_value_t)IMP_VALUE_INT(100),
                &w, &edge_val);

  // Should show second spinner frame
  assert_str_contains(cap.buffer, "⠂");
}

void test_progress_bar_dynamic_width(void) {
  // Test field_width = -1 (fill remaining space)
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  imp_widget_def_t widgets[] = {
    IMP_WIDGET_LABEL("Progress: "),
    IMP_WIDGET_PROGRESS_BAR(-1, "[", "]", "=", " ", NULL),
    IMP_WIDGET_PROGRESS_PERCENT(6, 1),
  };

  // "Progress: " = 10 cols, " XXX.X%" = 7 cols, bar should be 80 - 10 - 7 = 63 cols
  // Actually need to test the width calculation
}

void test_progress_bar_wide_chars_in_fill(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  // Use wide character as fill (2 columns)
  imp_widget_def_t w = IMP_WIDGET_PROGRESS_BAR(10, "[", "]", "█", " ", NULL);
  imp_value_t v = IMP_VALUE_NULL();

  imp_draw_line(&ctx,
                &(imp_value_t)IMP_VALUE_INT(50),
                &(imp_value_t)IMP_VALUE_INT(100),
                &w, &v);

  // Should handle wide chars correctly in layout
  parsed_output_t out = parse_output(cap.buffer);
  assert_eq(out.display_width, 12);  // [ + 10 cols + ]
}
```

**Property-Based Tests:**
- For any progress 0-100%, full_width + edge_width + empty_width = bar_width
- Edge position should be proportional to progress
- Total bar width (including ends) should equal field_width + len(left_end) + len(right_end)

---

## Category 5: Composite Widgets

Test nested widget rendering and layout.

```c
void test_composite_simple(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  imp_widget_def_t w = IMP_WIDGET_COMPOSITE(-1, 2, {
    IMP_WIDGET_LABEL("Hello "),
    IMP_WIDGET_LABEL("World"),
  });

  imp_value_t v = IMP_VALUE_COMPOSITE(2, {
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
  });

  imp_draw_line(&ctx, NULL, NULL, &w, &v);

  parsed_output_t out = parse_output(cap.buffer);
  assert_str_eq(out.text, "Hello World");
}

void test_composite_with_values(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  imp_widget_def_t w = IMP_WIDGET_COMPOSITE(-1, 3, {
    IMP_WIDGET_LABEL("Count: "),
    IMP_WIDGET_SCALAR(3, 0),
    IMP_WIDGET_LABEL(" items"),
  });

  imp_value_t v = IMP_VALUE_COMPOSITE(3, {
    IMP_VALUE_NULL(),
    IMP_VALUE_INT(42),
    IMP_VALUE_NULL(),
  });

  imp_draw_line(&ctx, NULL, NULL, &w, &v);

  parsed_output_t out = parse_output(cap.buffer);
  assert_str_eq(out.text, "Count:  42 items");
}

void test_composite_max_length(void) {
  // Test that max_len properly clips composite output
  // This is tricky - need to verify exact behavior
}
```

---

## Category 6: Frame Management and ANSI Codes

Test `imp_begin()`, `imp_end()`, and cursor manipulation.

```c
void test_begin_emits_cursor_codes(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);

  imp_begin(&ctx, 80);

  assert_has_ansi(cap.buffer, IMP_HIDE_CURSOR);
  assert_has_ansi(cap.buffer, IMP_AUTO_WRAP_DISABLE);
}

void test_end_done_shows_cursor(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  cap.offset = 0;  // Reset capture
  imp_end(&ctx, true);

  assert_has_ansi(cap.buffer, IMP_SHOW_CURSOR);
  assert_has_ansi(cap.buffer, IMP_AUTO_WRAP_ENABLE);
  assert_has_ansi(cap.buffer, IMP_ERASE_CURSOR_TO_SCREEN_END);
}

void test_multiline_cursor_positioning(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);

  // First frame
  imp_begin(&ctx, 80);
  imp_draw_line(&ctx, NULL, NULL,
                &(imp_widget_def_t)IMP_WIDGET_LABEL("Line 1"),
                &(imp_value_t)IMP_VALUE_NULL());
  imp_draw_line(&ctx, NULL, NULL,
                &(imp_widget_def_t)IMP_WIDGET_LABEL("Line 2"),
                &(imp_value_t)IMP_VALUE_NULL());
  imp_end(&ctx, false);

  // Second frame - should move cursor back up
  cap.offset = 0;
  imp_begin(&ctx, 80);

  // Should contain cursor up command for 1 line (2 lines - 1)
  assert_has_ansi(cap.buffer, "\033[1F");
}
```

---

## Category 7: Edge Cases and Error Handling

Test boundary conditions and error paths.

```c
void test_division_by_zero_progress(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  // prog_max = 0 should not crash
  imp_ret_t ret = imp_draw_line(&ctx,
                                &(imp_value_t)IMP_VALUE_INT(0),
                                &(imp_value_t)IMP_VALUE_INT(0),
                                &(imp_widget_def_t)IMP_WIDGET_PROGRESS_PERCENT(5, 1),
                                &(imp_value_t)IMP_VALUE_NULL());

  // Should handle gracefully (show 0% or return error)
  assert(ret == IMP_RET_SUCCESS || ret == IMP_RET_ERR_ARGS);
}

void test_spinner_zero_speed(void) {
  // speed_msec = 0 should not crash
  imp_widget_spinner_t s = {
    .frames = (char const *[]){"a", "b"},
    .frame_count = 2,
    .speed_msec = 0,
  };

  char const *result = imp__spinner_get_string(&s, 100);
  assert(result != NULL);  // Should return first frame or similar
}

void test_value_type_mismatch(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  // Pass string value to scalar widget
  imp_ret_t ret = imp_draw_line(&ctx, NULL, NULL,
                                &(imp_widget_def_t)IMP_WIDGET_SCALAR(5, 2),
                                &(imp_value_t)IMP_VALUE_STRING("hello"));

  assert_eq(ret, IMP_RET_ERR_WRONG_VALUE_TYPE);
}

void test_terminal_width_zero(void) {
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 0);

  // Should not crash with 0 width terminal
  imp_ret_t ret = imp_draw_line(&ctx, NULL, NULL,
                                &(imp_widget_def_t)IMP_WIDGET_LABEL("test"),
                                &(imp_value_t)IMP_VALUE_NULL());

  assert_eq(ret, IMP_RET_SUCCESS);
}

void test_very_long_string(void) {
  char long_str[10000];
  memset(long_str, 'a', sizeof(long_str) - 1);
  long_str[sizeof(long_str) - 1] = '\0';

  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  imp_ret_t ret = imp_draw_line(&ctx, NULL, NULL,
                                &(imp_widget_def_t)IMP_WIDGET_STRING(-1, 50),
                                &(imp_value_t)IMP_VALUE_STRING(long_str));

  assert_eq(ret, IMP_RET_SUCCESS);
  parsed_output_t out = parse_output(cap.buffer);
  assert_eq(out.display_width, 50);
}

void test_null_context(void) {
  // All functions should handle NULL ctx gracefully
  assert_eq(imp_init(NULL, NULL, NULL), IMP_RET_ERR_ARGS);
  assert_eq(imp_begin(NULL, 80), IMP_RET_ERR_ARGS);
  assert_eq(imp_draw_line(NULL, NULL, NULL, NULL, NULL), IMP_RET_ERR_ARGS);
  assert_eq(imp_end(NULL, false), IMP_RET_ERR_ARGS);
}

void test_int64_overflow(void) {
  char buf[64];

  // Test INT64_MAX
  imp_value_t v1 = IMP_VALUE_INT(INT64_MAX);
  int len = imp__value_write(-1, -1, IMP_UNIT_NONE, &v1, buf, sizeof(buf));
  assert(len > 0);

  // Test very large size conversions
  imp_value_t v2 = IMP_VALUE_INT(9223372036854775807LL);
  len = imp__value_write(-1, 2, IMP_UNIT_SIZE_DYNAMIC, &v2, buf, sizeof(buf));
  assert(len > 0);
  assert_str_contains(buf, "GB");
}
```

---

## Category 8: Regression Tests

Golden file tests for complex, realistic scenarios.

### Approach:

1. Create `.golden` files with expected output for various progress bar configurations
2. Run actual code and compare output (ignoring timestamp-specific values)
3. Use visual diffing tools to catch regressions

```c
void test_golden_download_progress(void) {
  // Simulate a file download progress bar
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);
  imp_begin(&ctx, 80);

  imp_widget_def_t w = IMP_WIDGET_COMPOSITE(-1, 5, {
    IMP_WIDGET_PROGRESS_BAR(-1, "[", "]", "=", " ",
                            &(imp_widget_def_t)IMP_WIDGET_LABEL(">")),
    IMP_WIDGET_LABEL(" "),
    IMP_WIDGET_PROGRESS_FRACTION(10, 1, IMP_UNIT_SIZE_DYNAMIC),
    IMP_WIDGET_LABEL(" "),
    IMP_WIDGET_PROGRESS_PERCENT(6, 1),
  });

  // Values for 50% progress, 512KB of 1024KB
  imp_value_t v = IMP_VALUE_COMPOSITE(5, {
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
    IMP_VALUE_NULL(),
  });

  imp_draw_line(&ctx,
                &(imp_value_t)IMP_VALUE_INT(524288),
                &(imp_value_t)IMP_VALUE_INT(1048576),
                &w, &v);

  assert_matches_golden(cap.buffer, "testdata/download_progress.golden");
}
```

---

## Category 9: Performance and Stress Tests

```c
void test_rapid_updates(void) {
  // Simulate 1000 rapid frame updates
  imp_ctx_t ctx;
  test_capture_t cap = {0};
  imp_init(&ctx, test_print_cb, &cap);

  for (int i = 0; i < 1000; i++) {
    cap.offset = 0;
    imp_begin(&ctx, 80);
    imp_draw_line(&ctx,
                  &(imp_value_t)IMP_VALUE_INT(i),
                  &(imp_value_t)IMP_VALUE_INT(1000),
                  &(imp_widget_def_t)IMP_WIDGET_PROGRESS_BAR(50, "[", "]", "=", " ", NULL),
                  &(imp_value_t)IMP_VALUE_NULL());
    imp_end(&ctx, false);
  }

  // Should complete without crashes
}

void test_complex_nested_composites(void) {
  // Test deeply nested composite widgets
  // Ensures no stack overflow or performance issues
}
```

---

## Category 10: Feature × Widget Test Matrix

This section maps out the combinatorial test space to ensure every widget type is tested with every applicable feature/configuration.

### Widget Types (from improg.h:34-46)

1. `IMP_WIDGET_TYPE_LABEL` - Static text
2. `IMP_WIDGET_TYPE_STRING` - Dynamic string with trimming
3. `IMP_WIDGET_TYPE_SCALAR` - Dynamic number with units
4. `IMP_WIDGET_TYPE_SPINNER` - Animated frames
5. `IMP_WIDGET_TYPE_PROGRESS_BAR` - Progress bar with fills
6. `IMP_WIDGET_TYPE_PROGRESS_FRACTION` - "X/Y" fraction display
7. `IMP_WIDGET_TYPE_PROGRESS_PERCENT` - Progress percentage
8. `IMP_WIDGET_TYPE_PROGRESS_LABEL` - Text selected by threshold
9. `IMP_WIDGET_TYPE_PROGRESS_SCALAR` - Progress value with units
10. `IMP_WIDGET_TYPE_PING_PONG_BAR` - Back-and-forth animation (unimplemented)
11. `IMP_WIDGET_TYPE_COMPOSITE` - Container of widgets

### Features to Test

**Core Features:**
- A. Field width (`-1` = natural, `0`, `1`, `10`, `100`)
- B. Precision (`-1` = default, `0`, `2`, `6`)
- C. Max length (for strings/composites)
- D. Units (NONE, SIZE_*, TIME_*)
- E. UTF-8 content (ASCII, wide chars, combining chars, emoji)
- F. Progress values (0%, 1%, 50%, 99%, 100%, >100%)
- G. NULL/empty values
- H. Terminal width constraints (narrow: 40, normal: 80, wide: 200)
- I. Value types (NULL, INT, DOUBLE, STRING, COMPOSITE)
- J. Custom trim (left, right, various lengths)
- K. Edge fills (NULL, simple, composite widget)
- L. Dynamic width (field_width = -1, space-filling)

### Test Matrix

Below is the combinatorial matrix showing which features apply to which widgets. ✓ = Must test, ○ = Optional/edge case, — = Not applicable.

| Widget Type | A<br>Field<br>Width | B<br>Preci-<br>sion | C<br>Max<br>Length | D<br>Units | E<br>UTF-8 | F<br>Progress<br>Values | G<br>NULL<br>Values | H<br>Term<br>Width | I<br>Value<br>Types | J<br>Custom<br>Trim | K<br>Edge<br>Fills | L<br>Dynamic<br>Width |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| **LABEL** | — | — | — | — | ✓ | — | — | ✓ | — | — | — | — |
| **STRING** | ✓ | — | ✓ | — | ✓ | — | ✓ | ✓ | ✓<br>(STRING) | ✓ | — | — |
| **SCALAR** | ✓ | ✓ | — | ✓ | — | — | ✓ | ✓ | ✓<br>(INT/DBL) | — | — | — |
| **SPINNER** | — | — | — | — | ✓ | — | ○ | ✓ | ✓<br>(INT/DBL) | — | — | — |
| **PROGRESS_BAR** | ✓ | — | — | — | ✓ | ✓ | ✓ | ✓ | ✓<br>(NULL) | — | ✓ | ✓ |
| **PROGRESS_FRACTION** | ✓ | ✓ | — | ✓ | — | ✓ | ○ | ✓ | — | — | — | — |
| **PROGRESS_PERCENT** | ✓ | ✓ | — | — | — | ✓ | — | ✓ | — | — | — | — |
| **PROGRESS_LABEL** | ✓ | — | — | — | ✓ | ✓ | ○ | ✓ | — | — | — | — |
| **PROGRESS_SCALAR** | ✓ | ✓ | — | ✓ | — | ✓ | ○ | ✓ | — | — | — | — |
| **PING_PONG_BAR** | ✓ | — | — | — | ✓ | — | ○ | ✓ | ○ | — | ○ | ✓ |
| **COMPOSITE** | — | — | ✓ | — | ✓ | ✓ | ✓ | ✓ | ✓<br>(COMP) | — | — | — |

### Detailed Test Case Breakdown

#### LABEL Widget (Static Text)
**Must Test:**
- E: UTF-8 content variations
  - ASCII only: `"Progress:"`
  - Wide chars: `"進捗："`
  - Emoji: `"📊 Stats:"`
  - Mixed: `"Progress 進捗 📊"`
- H: Terminal width constraints
  - Label longer than terminal width
  - Label at exact terminal width

**Test Count:** ~6 cases

---

#### STRING Widget (Dynamic String)
**Must Test:**
- A: Field widths: -1 (natural), 0, 5, 20
- C: Max lengths: -1 (unlimited), 0, 5, 10, 50
- E: UTF-8 variations (ASCII, wide, combining, emoji)
- G: NULL and empty string values
- H: Terminal width constraints
- I: Value type validation (should accept STRING, reject others)
- J: Custom trim configurations
  - No trim (NULL)
  - Right trim: `"..."`, `">>"`
  - Left trim: `"..."`, `"<<"`
  - Wide char trim: `"…"`, `"》"`

**Combinations to Test:**
- field_width × max_length: 4 × 5 = 20 combinations
- UTF-8 types: 4 types
- Custom trim: 6 variations (none, 2×right, 2×left, wide)
- NULL/empty: 2 cases

**Critical Cases:**
- max_length = 5 with wide char string `"你好世界"` (should fit "你好" = 4 cols)
- max_length = 5 with custom_trim = "..." (3 cols), string = "hello world" (should show "he...")
- field_width = 10, max_length = 5 (padding + truncation)
- Combining chars at trim boundary

**Test Count:** ~80 cases (prioritize critical cases)

---

#### SCALAR Widget (Numeric Value)
**Must Test:**
- A: Field widths: -1, 0, 5, 10
- B: Precision: -1, 0, 2, 6
- D: All unit types
  - IMP_UNIT_NONE
  - IMP_UNIT_SIZE_B, KB, MB, GB, DYNAMIC
  - IMP_UNIT_TIME_SEC, HMS_LETTERS, HMS_COLONS
- G: NULL value handling
- H: Terminal width constraints
- I: Value types (INT, DOUBLE, should reject STRING/COMPOSITE)

**Combinations to Test:**
- field_width × precision: 4 × 4 = 16 combinations
- Units: 10 unit types
- Value types: INT, DOUBLE (2 types)

**Critical Cases:**
- INT64_MAX with each unit type
- INT64_MIN (if signed values supported)
- 0 with SIZE_DYNAMIC (should show "0B")
- 1023 with SIZE_DYNAMIC (should show "1023B")
- 1024 with SIZE_DYNAMIC (should show "1.00KB" with precision=2)
- 3665 seconds with HMS_LETTERS (should show "1h1m5s")
- 3665 seconds with HMS_COLONS (should show "01:01:05")
- field_width accounting for unit suffix length
- Precision = 0 with units (no decimal point)
- Very large values with TIME units (days worth of seconds)

**Test Count:** ~100 cases

---

#### SPINNER Widget (Animation)
**Must Test:**
- E: UTF-8 in frames
  - ASCII frames: `{"-", "\\", "|", "/"}`
  - Wide char frames: `{"⠁", "⠂", "⠄", "⡀"}`
  - Emoji frames: `{"🌑", "🌒", "🌓", "🌔"}`
- F: Time values (0ms, 100ms, 1000ms, UINT_MAX)
- G: NULL value (or 0 msec)
- H: Terminal width
- I: Value types (should accept INT/DOUBLE as milliseconds)

**Critical Cases:**
- speed_msec = 0 (division by zero)
- speed_msec = 1, very large msec value (overflow check)
- frame_count = 1 (degenerate case)
- Wide char frames with narrow terminal width
- Millisecond values that wrap around frame array

**Test Count:** ~20 cases

---

#### PROGRESS_BAR Widget (Most Complex)
**Must Test:**
- A: Field widths: -1 (dynamic), 0, 10, 50
- E: UTF-8 in fills and ends
  - ASCII fills: `"=", " "`, ends: `"[", "]"`
  - Wide char fills: `"█", "░"`, ends: `"【", "】"`
  - Emoji fills: `"🟩", "⬜"`
- F: Progress values: 0%, 1%, 25%, 50%, 75%, 99%, 100%, 150% (overflow)
- G: NULL value for edge_fill
- H: Terminal width constraints (especially with dynamic width)
- K: Edge fill variations
  - NULL (no edge)
  - Simple widget: LABEL
  - Composite widget
  - SPINNER widget (with scale_fill)
- L: Dynamic width (field_width = -1)
  - Alone on line
  - With left-side widgets
  - With right-side widgets
  - With both sides

**Critical Cases:**
- field_width = 20, progress = 50% → should show exactly 10 filled, 10 empty
- field_width = 10, edge_fill width = 3, progress = 50% → edge positioning
- edge_fill wider than bar (should handle gracefully)
- scale_fill = true with composite edge widget
- Dynamic width calculation with complex surrounding widgets
- Wide char fills (2-column) vs ASCII fills (1-column) layout math
- Progress exactly at 0% and 100% (edge fill should not draw)
- Progress = 0.5% (very small, edge should still position correctly)
- Very wide progress bar (200 columns)

**Combinations to Test:**
- field_width × progress: 5 × 8 = 40 combinations
- Fill types: 3 types
- Edge fills: 4 variations
- Dynamic width scenarios: 4 scenarios

**Test Count:** ~120 cases

---

#### PROGRESS_FRACTION Widget (X/Y Display)
**Must Test:**
- A: Field widths: -1, 0, 10, 20
- B: Precision: -1, 0, 2
- D: Units (all types, especially SIZE_DYNAMIC with different scales)
- F: Progress values with different scales
  - Same unit: 50/100, 512/1024
  - Different magnitudes: 1/1000000
- H: Terminal width

**Critical Cases:**
- Numerator > denominator (150/100)
- Denominator = 0 (division by zero check)
- Very large values (INT64_MAX / INT64_MAX)
- Mixed unit rendering (512B / 2KB → display as fraction)
- field_width with SIZE_DYNAMIC where num and den have different unit scales
  - Example: 512B/2MB should show "0.50KB/2.00MB" or similar
- Precision = 0 with fractional values

**Test Count:** ~40 cases

---

#### PROGRESS_PERCENT Widget
**Must Test:**
- A: Field widths: -1, 0, 6, 10
- B: Precision: -1, 0, 1, 3
- F: Progress values: 0%, 1%, 50%, 99%, 99.9%, 100%, 150%
- H: Terminal width

**Critical Cases:**
- Precision = 0 (should show "50%", no decimal)
- Precision = 3 (should show "50.000%")
- field_width accounting for '%' character (field_width=6 should fit "100.0%")
- Progress > 100% (should show "150%" or clamp to "100%")
- Very small progress (0.001%)
- field_width smaller than needed for value (e.g., field_width=3, value="100.5%")

**Test Count:** ~30 cases

---

#### PROGRESS_LABEL Widget (Threshold-Based Text)
**Must Test:**
- A: Field widths: -1, 0, 10
- E: UTF-8 in label strings
- F: Progress values crossing thresholds
  - Array: `[{0.33, "Low"}, {0.66, "Medium"}, {1.0, "High"}]`
  - Test: 0%, 20%, 35%, 50%, 70%, 100%
- H: Terminal width

**Critical Cases:**
- Progress exactly at threshold boundary (0.33 → should show "Low" or "Medium"?)
- Progress > 1.0 with no threshold match (fallback behavior)
- Empty label array (0 labels)
- Single label (1 threshold)
- Labels with varying widths and field_width padding
- Wide char labels with field_width

**Test Count:** ~25 cases

---

#### PROGRESS_SCALAR Widget
**Must Test:**
- A: Field widths: -1, 0, 5, 10
- B: Precision: -1, 0, 2
- D: All unit types
- F: Progress values (uses prog_cur, not percentage)
- H: Terminal width

**Critical Cases:**
- Similar to SCALAR widget, but uses prog_cur value from context
- Should display prog_cur, NOT prog_max (distinction from PROGRESS_FRACTION)
- All unit conversions (especially SIZE_DYNAMIC)
- Large values with different units

**Test Count:** ~60 cases

---

#### PING_PONG_BAR Widget (Unimplemented)
**Document Expected Behavior:**
- A: Field widths: -1 (dynamic), 0, 10, 50
- E: UTF-8 in bouncer widget and fill
- H: Terminal width
- K: Bouncer widget variations (LABEL, SPINNER, PROGRESS_PERCENT)
- L: Dynamic width

**Test Count:** Deferred until implementation (~40 cases expected)

---

#### COMPOSITE Widget (Container)
**Must Test:**
- C: Max length: -1 (unlimited), 10, 50
- E: UTF-8 in child widgets
- F: Progress values passed to children
- G: NULL values in child value array
- H: Terminal width (especially with max_length)
- I: Composite value type with matching child count

**Nesting Scenarios:**
- Flat composite (2-5 children, all simple widgets)
- Nested composite (composite containing composite)
- Deep nesting (3+ levels)
- Mixed widget types (LABEL + SCALAR + PROGRESS_BAR)
- Composite with dynamic width child (PROGRESS_BAR with -1)

**Critical Cases:**
- Widget count mismatch (3 widgets, 2 values → error)
- Value type mismatch for children
- max_length truncation with UTF-8 children
- max_length = 10, children total width = 50 (should truncate)
- Empty composite (0 widgets)
- Very wide composite (200 columns of children)
- Composite containing PROGRESS_BAR with dynamic width

**Test Count:** ~50 cases

---

### Cross-Widget Integration Tests

Beyond individual widget testing, test interactions:

1. **Multiple widgets on same line with dynamic width bar**
   - `[LABEL] [DYNAMIC_PROGRESS_BAR] [PERCENT]`
   - Ensure math: label_width + bar_width + percent_width = terminal_width

2. **Composite containing all widget types**
   - Validate each child renders correctly within composite

3. **Progress bars with composite edge fills**
   - Edge fill itself contains SPINNER + LABEL

4. **Nested composites with progress propagation**
   - Outer composite passes progress to inner composite's progress widgets

5. **UTF-8 mixing across widgets**
   - Line with ASCII, wide chars, emoji from different widgets

6. **Terminal width edge cases**
   - Line with total width exactly = terminal width
   - Line with total width > terminal width (should truncate or wrap?)
   - Dynamic bar with 0 space remaining

---

### Priority Test Cases

Given the issues found, prioritize these:

**P0 (Critical - Must Pass):**
- PROGRESS_BAR: edge_fill_width > bar_width (Issue #3)
- SCALAR: SIZE_DYNAMIC with DOUBLE value (Issue #1)
- PROGRESS_BAR/FRACTION/PERCENT: prog_max = 0 (Issue #8)
- SPINNER: speed_msec = 0 (Issue #9)
- STRING: Wide char at max_length boundary
- All widgets: NULL context, NULL values
- PROGRESS_PERCENT: field_width = -1 with precision (Issue #5)

**P1 (High - Should Pass):**
- SCALAR: All unit types with INT64_MAX
- STRING: Custom trim at UTF-8 boundaries
- PROGRESS_BAR: Dynamic width calculation with complex neighbors
- COMPOSITE: Nested composites with progress values
- All widgets: Terminal width = 0, 1, 40, 80, 200

**P2 (Medium - Nice to Have):**
- All widgets: Very long content
- SPINNER: Emoji frames with high time values
- UTF-8: Combining chars, RTL, ZWJ sequences
- PROGRESS_LABEL: Empty or single-entry threshold array

**P3 (Low - Edge Cases):**
- Negative values (if applicable)
- Very high precision values (precision = 20)
- Degenerate cases (empty strings, 0-length bars)

---

### Test Count Summary

| Widget Type | Estimated Test Cases |
|-------------|---------------------|
| LABEL | 6 |
| STRING | 80 |
| SCALAR | 100 |
| SPINNER | 20 |
| PROGRESS_BAR | 120 |
| PROGRESS_FRACTION | 40 |
| PROGRESS_PERCENT | 30 |
| PROGRESS_LABEL | 25 |
| PROGRESS_SCALAR | 60 |
| PING_PONG_BAR | 0 (unimplemented) |
| COMPOSITE | 50 |
| **Widget Subtotal** | **531** |
| Cross-widget integration | 30 |
| UTF-8 edge cases | 20 |
| Error handling | 25 |
| **Grand Total** | **~606 test cases** |

This can be reduced by focusing on P0/P1 priorities (~250 cases) for initial implementation.

---

## Test Organization

### Directory Structure:

```
tests/
├── test_utf8_width.c          # UTF-8 display width tests
├── test_value_format.c        # Value formatting and units
├── test_string_widget.c       # String trimming/clipping
├── test_progress_bar.c        # Progress bar layout math
├── test_composite.c           # Composite widgets
├── test_frame_management.c    # ANSI codes and cursor
├── test_edge_cases.c          # Error handling and boundaries
├── test_golden.c              # Golden file regression tests
├── test_performance.c         # Stress tests
├── test_helpers.c             # Test infrastructure
├── test_helpers.h
└── testdata/
    ├── download_progress.golden
    ├── multiline_complex.golden
    └── unicode_mixed.golden
```

### Test Runner:

```c
int main(void) {
  int passed = 0, failed = 0;

  RUN_TEST(test_utf8_ascii_width);
  RUN_TEST(test_utf8_wide_chars);
  // ... all tests

  printf("\n%d passed, %d failed\n", passed, failed);
  return failed > 0 ? 1 : 0;
}
```

---

## Continuous Integration

### CI Pipeline:

1. **Compiler matrix**: Test with gcc, clang, msvc
2. **Platform matrix**: Linux, macOS, Windows
3. **Sanitizers**:
   - AddressSanitizer (memory errors)
   - UndefinedBehaviorSanitizer (UB detection)
   - MemorySanitizer (uninitialized reads)
4. **Valgrind**: Memory leak detection
5. **Coverage**: Aim for >95% line coverage
6. **Fuzzing**: Use libFuzzer for input fuzzing (especially UTF-8 parsing)

---

## Manual Testing Checklist

Since this is a visual library, manual testing in real terminals is essential:

- [ ] Test in xterm, iTerm2, Windows Terminal, GNOME Terminal
- [ ] Test with different terminal widths (40, 80, 120, 200 columns)
- [ ] Test with terminal emulators that handle wide chars differently
- [ ] Test rapid resize during animation
- [ ] Test with screen readers (accessibility)
- [ ] Test with TERM=dumb (fallback behavior)
- [ ] Visual inspection of alignment at various progress percentages
- [ ] Verify no flickering or cursor artifacts during animation

---

## Summary

This testing strategy covers:

1. ✅ **UTF-8 correctness** - Comprehensive Unicode width validation
2. ✅ **Math precision** - Field widths, progress percentages, layout calculations
3. ✅ **Edge cases** - Division by zero, overflows, type mismatches
4. ✅ **ANSI codes** - Proper terminal control sequences
5. ✅ **Visual validation** - Golden file tests for complex layouts
6. ✅ **Performance** - Stress tests for rapid updates
7. ✅ **Cross-platform** - Testing on multiple compilers and OSes

**Key Testing Principles:**
- Mock the terminal output for reproducible tests
- Use property-based testing for invariants
- Validate visual output with golden files
- Test edge cases exhaustively (especially math boundaries)
- Fuzz UTF-8 parsing and layout code
- Manual testing in real terminals for final validation
