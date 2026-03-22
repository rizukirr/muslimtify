# json.h Cleanup and Tests Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rename `libjson.h` to `json.h`, strip unused code, fix string unescaping, silence noisy missing-key errors, and add comprehensive unit tests.

**Architecture:** Single header-only file cleanup. Rename + remove dead code first (verify existing tests pass), then fix correctness issues, then add new tests. Each task produces a working build.

**Tech Stack:** C99, header-only library, custom test framework

**Spec:** `docs/superpowers/specs/2026-03-22-libjson-cleanup-design.md`

---

## File Structure

| Action | File | Responsibility |
|--------|------|----------------|
| Rename | `src/libjson.h` → `src/json.h` | JSON parser header |
| Modify | `src/config.c` | Update include + implementation guard |
| Modify | `src/location.c` | Update include |
| Create | `tests/test_json.c` | Unit tests for json.h |
| Modify | `CMakeLists.txt` | Add test_json target |

---

### Task 1: Rename `libjson.h` to `json.h` and update all references

**Files:**
- Rename: `src/libjson.h` → `src/json.h`
- Modify: `src/config.c:1-2`
- Modify: `src/location.c:2`

- [ ] **Step 1: Rename the file**

```bash
cd /home/rizki/Project/muslimtify
git mv src/libjson.h src/json.h
```

- [ ] **Step 2: Update header guards in `src/json.h`**

Replace:
```c
#ifndef LIBJSON_H
#define LIBJSON_H
```
With:
```c
#ifndef JSON_H
#define JSON_H
```

Replace:
```c
#ifdef LIBJSON_IMPLEMENTATION
```
With:
```c
#ifdef JSON_IMPLEMENTATION
```

Replace:
```c
#endif /* LIBJSON_IMPLEMENTATION */
```
With:
```c
#endif /* JSON_IMPLEMENTATION */
```

Replace:
```c
#endif /* LIBJSON_H */
```
With:
```c
#endif /* JSON_H */
```

- [ ] **Step 3: Update `src/config.c`**

Replace lines 1-2:
```c
#define LIBJSON_IMPLEMENTATION
#include "libjson.h"
```
With:
```c
#define JSON_IMPLEMENTATION
#include "json.h"
```

- [ ] **Step 4: Update `src/location.c`**

Replace:
```c
#include "libjson.h"
```
With:
```c
#include "json.h"
```

- [ ] **Step 5: Verify build and tests**

```bash
cd /home/rizki/Project/muslimtify
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```
Expected: all 4 tests pass.

- [ ] **Step 6: Commit**

```bash
git add src/json.h src/config.c src/location.c
git commit -m "refactor: rename libjson.h to json.h"
```

---

### Task 2: Remove unused code from `json.h`

**Files:**
- Modify: `src/json.h`

Remove the following from `src/json.h`:

- [ ] **Step 1: Remove `get_array` declaration from public API**

In the public section (before `#ifdef JSON_IMPLEMENTATION`), remove the `get_array` declaration and its doc comment (lines 65-73 approximately):

```c
/**
 * Parse a JSON array and return pointers to its elements.
 * @param ctx The JSON context
 * @param key The key to search for (unused in current implementation)
 * @param raw_json The raw JSON array string
 * @param count Output parameter for the number of elements in the array
 * @return Array of JSON object strings, or NULL on error
 */
char **get_array(JsonContext *JSON_RESTRICT ctx, const char *JSON_RESTRICT key,
                 char *JSON_RESTRICT raw_json, size_t *JSON_RESTRICT count);
```

- [ ] **Step 2: Remove `da_append`, `da_clear` macros and `JsonArray` typedef**

Inside the `#ifdef JSON_IMPLEMENTATION` section, remove the `da_append` macro (lines 96-117), the `da_clear` macro (lines 119-124), and the `JsonArray` typedef (lines 255-259):

```c
#define DA_INIT_CAP 256

#define da_append(da, item) \
  ...

#define da_clear(da) \
  ...
```

And:
```c
typedef struct {
  size_t count;
  size_t capacity;
  char **items;
} JsonArray;
```

- [ ] **Step 3: Remove `stack` field from `JsonContext`**

Change:
```c
struct JsonContext {
  JsonArena *arena;
  JsonArray *stack;
};
```
To:
```c
struct JsonContext {
  JsonArena *arena;
};
```

- [ ] **Step 4: Remove `stack` allocation from `json_begin`**

Remove these lines from `json_begin()`:
```c
  ctx->stack = json_alloc(arena, sizeof(JsonArray), ARENA_ALIGNOF(JsonArray));
  if (!ctx->stack) {
    fprintf(stderr, "Failed to allocate JsonArray\n");
    json_alloc_free(arena);
    return NULL;
  }
  ctx->stack->count = 0;
  ctx->stack->capacity = 0;
  ctx->stack->items = NULL;
```

- [ ] **Step 5: Remove `da_clear(ctx->stack)` from `json_end`**

Remove from `json_end()`:
```c
  if (ctx->stack) {
    da_clear(ctx->stack);
  }
```

- [ ] **Step 6: Remove `skip_string` function**

Remove the entire `skip_string` function and its unused attribute:
```c
#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static const char *skip_string(const char *cursor) {
  ...
}
```

- [ ] **Step 7: Remove `get_array` implementation**

Remove the entire `get_array` function implementation.

- [ ] **Step 8: Make `json_alloc_free` static**

Change:
```c
void json_alloc_free(JsonArena *arena) {
```
To:
```c
static void json_alloc_free(JsonArena *arena) {
```

- [ ] **Step 9: Remove `DA_INIT_CAP` define**

Remove:
```c
#define DA_INIT_CAP 256
```

- [ ] **Step 10: Verify build and tests**

```bash
cd /home/rizki/Project/muslimtify
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```
Expected: all 4 tests pass (config round-trip test is the key regression guard).

- [ ] **Step 11: Commit**

```bash
git add src/json.h
git commit -m "refactor: remove unused code from json.h"
```

---

### Task 3: Fix correctness issues in `json.h`

**Files:**
- Modify: `src/json.h`

- [ ] **Step 1: Add string unescaping in `json_extract_value`**

Replace the string extraction path in `json_extract_value`. The current code (inside `if (*cursor == '"')`) does a raw `memcpy`. Replace with an unescape loop:

Current:
```c
  if (*cursor == '"') {
    // String value
    cursor++;
    value_end = cursor;
    while (*value_end && *value_end != '"') {
      if (*value_end == '\\')
        value_end++; // Skip escaped chars
      value_end++;
    }
    value_len = value_end - cursor;
    char *result = json_alloc(arena, value_len + 1, ARENA_ALIGNOF(char));
    if (!result) return NULL;
    memcpy(result, cursor, value_len);
    result[value_len] = '\0';
    return result;
```

Replace with:
```c
  if (*cursor == '"') {
    // String value — find end first to determine max length
    cursor++;
    const char *scan = cursor;
    while (*scan && *scan != '"') {
      if (*scan == '\\')
        scan++; // Skip escaped char
      scan++;
    }
    size_t raw_len = scan - cursor;

    // Allocate raw_len + 1 (unescaped is always <= raw length)
    char *result = json_alloc(arena, raw_len + 1, ARENA_ALIGNOF(char));
    if (!result) return NULL;

    // Copy with unescape
    char *dst = result;
    const char *src = cursor;
    while (src < scan) {
      if (*src == '\\' && src + 1 < scan) {
        src++;
        switch (*src) {
        case '"':  *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; break;
        case '/':  *dst++ = '/'; break;
        case 'n':  *dst++ = '\n'; break;
        case 'r':  *dst++ = '\r'; break;
        case 't':  *dst++ = '\t'; break;
        case 'b':  *dst++ = '\b'; break;
        case 'f':  *dst++ = '\f'; break;
        case 'u':
          // \uXXXX — pass through as-is (6 chars)
          *dst++ = '\\';
          *dst++ = 'u';
          for (int i = 0; i < 4 && src + 1 < scan; i++) {
            src++;
            *dst++ = *src;
          }
          break;
        default:
          // Unknown escape — keep as-is
          *dst++ = '\\';
          *dst++ = *src;
          break;
        }
      } else {
        *dst++ = *src;
      }
      src++;
    }
    *dst = '\0';
    return result;
```

- [ ] **Step 2: Silence missing-key stderr in `json_find_key`**

In `json_find_key`, change the last lines from:
```c
  fprintf(stderr, "key %s not found\n", key);
  return NULL;
```
To:
```c
  return NULL;
```

- [ ] **Step 3: Silence missing-key stderr in `get_obj`**

In `get_obj`, change:
```c
  if (!value) {
    fprintf(stderr, "Key %s is not found", key);
    return NULL;
  }
```
To:
```c
  if (!value) {
    return NULL;
  }
```

- [ ] **Step 4: Silence `json_end(NULL)` stderr**

In `json_end`, change:
```c
  if (!ctx) {
    fprintf(stderr, "Invalid context to json_end\n");
    return;
  }
```
To:
```c
  if (!ctx) {
    return;
  }
```

- [ ] **Step 5: Verify build and tests**

```bash
cd /home/rizki/Project/muslimtify
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```
Expected: all 4 tests pass. The `test_config` round-trip test is the key regression guard — config values don't contain escape sequences, so unescaping is a no-op for them.

- [ ] **Step 6: Commit**

```bash
git add src/json.h
git commit -m "fix: add string unescaping and silence missing-key noise in json.h"
```

---

### Task 4: Add unit tests for `json.h`

**Files:**
- Create: `tests/test_json.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create `tests/test_json.c`**

```c
#define JSON_IMPLEMENTATION
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;
static int total = 0;

static void check_str(const char *result, const char *expected,
                      const char *label) {
  total++;
  if (result == NULL && expected == NULL) {
    printf("  PASS: %s\n", label);
  } else if (result != NULL && expected != NULL && strcmp(result, expected) == 0) {
    printf("  PASS: %s\n", label);
  } else {
    printf("  FAIL: %s — got \"%s\", expected \"%s\"\n", label,
           result ? result : "(null)", expected ? expected : "(null)");
    failures++;
  }
}

static void check_null(const char *result, const char *label) {
  total++;
  if (result == NULL) {
    printf("  PASS: %s\n", label);
  } else {
    printf("  FAIL: %s — expected NULL, got \"%s\"\n", label, result);
    failures++;
  }
}

static void check_not_null(const void *result, const char *label) {
  total++;
  if (result != NULL) {
    printf("  PASS: %s\n", label);
  } else {
    printf("  FAIL: %s — expected non-NULL\n", label);
    failures++;
  }
}

/* ── Basic extraction ─────────────────────────────────────────────────────── */

static void test_basic_string(void) {
  printf("test_basic_string\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"name\": \"muslimtify\"}";
  char *val = get_value(ctx, "name", json);
  check_str(val, "muslimtify", "string value");
  json_end(ctx);
}

static void test_basic_number(void) {
  printf("test_basic_number\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"count\": 42}";
  char *val = get_value(ctx, "count", json);
  check_str(val, "42", "number value");
  json_end(ctx);
}

static void test_basic_boolean(void) {
  printf("test_basic_boolean\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"enabled\": true}";
  char *val = get_value(ctx, "enabled", json);
  check_str(val, "true", "boolean value");
  json_end(ctx);
}

static void test_basic_null(void) {
  printf("test_basic_null\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"value\": null}";
  char *val = get_value(ctx, "value", json);
  check_str(val, "null", "null value");
  json_end(ctx);
}

static void test_empty_string_value(void) {
  printf("test_empty_string_value\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"key\": \"\"}";
  char *val = get_value(ctx, "key", json);
  check_not_null(val, "empty string is non-NULL");
  check_str(val, "", "empty string value");
  json_end(ctx);
}

/* ── Nested objects ───────────────────────────────────────────────────────── */

static void test_nested_object(void) {
  printf("test_nested_object\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"location\": {\"lat\": 1.5, \"lon\": 2.0}}";
  char *loc = get_value(ctx, "location", json);
  check_not_null(loc, "nested object extracted");

  char *lat = get_value(ctx, "lat", loc);
  check_str(lat, "1.5", "nested lat value");

  char *lon = get_value(ctx, "lon", loc);
  check_str(lon, "2.0", "nested lon value");
  json_end(ctx);
}

/* ── Missing keys ─────────────────────────────────────────────────────────── */

static void test_missing_key(void) {
  printf("test_missing_key\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"name\": \"test\"}";
  char *val = get_value(ctx, "nonexistent", json);
  check_null(val, "missing key returns NULL");
  json_end(ctx);
}

static void test_empty_object(void) {
  printf("test_empty_object\n");
  JsonContext *ctx = json_begin();
  char json[] = "{}";
  char *val = get_value(ctx, "key", json);
  check_null(val, "empty object returns NULL");
  json_end(ctx);
}

/* ── Escaped strings ──────────────────────────────────────────────────────── */

static void test_escaped_quote(void) {
  printf("test_escaped_quote\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"msg\": \"hello\\\"world\"}";
  char *val = get_value(ctx, "msg", json);
  check_str(val, "hello\"world", "escaped quote");
  json_end(ctx);
}

static void test_escaped_backslash(void) {
  printf("test_escaped_backslash\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"path\": \"c:\\\\dir\"}";
  char *val = get_value(ctx, "path", json);
  check_str(val, "c:\\dir", "escaped backslash");
  json_end(ctx);
}

static void test_escaped_newline(void) {
  printf("test_escaped_newline\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"text\": \"line1\\nline2\"}";
  char *val = get_value(ctx, "text", json);
  check_str(val, "line1\nline2", "escaped newline");
  json_end(ctx);
}

static void test_escaped_tab(void) {
  printf("test_escaped_tab\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"text\": \"col1\\tcol2\"}";
  char *val = get_value(ctx, "text", json);
  check_str(val, "col1\tcol2", "escaped tab");
  json_end(ctx);
}

static void test_unicode_passthrough(void) {
  printf("test_unicode_passthrough\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"u\": \"\\u0041\"}";
  char *val = get_value(ctx, "u", json);
  check_str(val, "\\u0041", "unicode passthrough");
  json_end(ctx);
}

/* ── Edge cases ───────────────────────────────────────────────────────────── */

static void test_key_inside_value(void) {
  printf("test_key_inside_value\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"city\": \"lat=1.0\", \"lat\": \"2.0\"}";
  char *val = get_value(ctx, "lat", json);
  check_str(val, "2.0", "key inside value not confused");
  json_end(ctx);
}

static void test_multiple_get_value(void) {
  printf("test_multiple_get_value\n");
  JsonContext *ctx = json_begin();
  char json[] = "{\"a\": \"first\", \"b\": \"second\", \"c\": \"third\"}";

  char *a = get_value(ctx, "a", json);
  char *b = get_value(ctx, "b", json);
  char *c = get_value(ctx, "c", json);

  // Earlier results must remain valid (arena doesn't overwrite)
  check_str(a, "first", "first value still valid");
  check_str(b, "second", "second value still valid");
  check_str(c, "third", "third value still valid");
  json_end(ctx);
}

/* ── Lifecycle ────────────────────────────────────────────────────────────── */

static void test_json_begin(void) {
  printf("test_json_begin\n");
  JsonContext *ctx = json_begin();
  check_not_null(ctx, "json_begin returns non-NULL");
  json_end(ctx);
}

static void test_json_end_null(void) {
  printf("test_json_end_null\n");
  // Should not crash or print to stderr
  json_end(NULL);
  total++;
  printf("  PASS: json_end(NULL) does not crash\n");
}

/* ── Main ─────────────────────────────────────────────────────────────────── */

int main(void) {
  printf("=== json.h tests ===\n\n");

  test_json_begin();
  test_json_end_null();

  test_basic_string();
  test_basic_number();
  test_basic_boolean();
  test_basic_null();
  test_empty_string_value();

  test_nested_object();

  test_missing_key();
  test_empty_object();

  test_escaped_quote();
  test_escaped_backslash();
  test_escaped_newline();
  test_escaped_tab();
  test_unicode_passthrough();

  test_key_inside_value();
  test_multiple_get_value();

  printf("\n%d/%d tests passed\n", total - failures, total);
  return failures > 0 ? 1 : 0;
}
```

- [ ] **Step 2: Add test target to `CMakeLists.txt`**

Add after the existing test targets (inside the `if(BUILD_TESTING)` block):

```cmake
    add_executable(test_json tests/test_json.c)
    muslimtify_set_target_defaults(test_json)
    target_link_libraries(test_json m)
    add_test(NAME json COMMAND test_json)
```

- [ ] **Step 3: Build and run all tests**

```bash
cd /home/rizki/Project/muslimtify
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```
Expected: all 5 tests pass (4 existing + 1 new `json` test).

- [ ] **Step 4: Commit**

```bash
git add tests/test_json.c CMakeLists.txt
git commit -m "test: add unit tests for json.h"
```

---

### Task 5: Update CLAUDE.md

**Files:**
- Modify: `CLAUDE.md`

- [ ] **Step 1: Update architecture section**

Replace:
```
- `src/libjson.h` — Header-only JSON parser/generator
```
With:
```
- `src/json.h` — Header-only JSON parser
```

- [ ] **Step 2: Commit**

```bash
git add CLAUDE.md
git commit -m "docs: update CLAUDE.md for json.h rename"
```
