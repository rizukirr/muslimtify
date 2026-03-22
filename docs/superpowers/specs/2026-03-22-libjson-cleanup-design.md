# json.h Cleanup and Tests

## Summary

Rename `src/libjson.h` to `src/json.h`, strip unused code, fix correctness issues, and add unit tests. No algorithmic changes — the linear-scan approach is appropriate for the use case (~1KB config files).

## Rename: `libjson.h` → `json.h`

Rename the file and update all references:

- `src/libjson.h` → `src/json.h`
- Header guard: `LIBJSON_H` → `JSON_H`
- Implementation guard: `LIBJSON_IMPLEMENTATION` → `JSON_IMPLEMENTATION`
- Update `#include "libjson.h"` in `src/config.c` and `src/location.c`
- Update `#define LIBJSON_IMPLEMENTATION` in consumers

## Cleanup: Remove unused code

Remove these unused components:

- `get_array()` function declaration and implementation — never called
- `skip_string()` function — marked `__attribute__((unused))`, never called
- `JsonArray` typedef — only used by `get_array`
- `da_append` / `da_clear` macros — only used by `get_array`
- `JsonContext.stack` field — only used by `get_array`
- `stack` allocation in `json_begin()` and `da_clear(ctx->stack)` in `json_end()`
- Make `json_alloc_free` `static` — it has no public declaration and is only called by `json_end`

Result: ~490 lines (down from ~620), containing only: arena allocator + `json_begin` / `json_end` / `get_value`.

## Correctness fixes

### 1. String unescaping in `json_extract_value`

Current behavior: `get_value` for JSON `"hello\"world"` returns `hello\"world` with backslash still present. The value is copied verbatim without processing escape sequences.

Fix: Add escape processing in the string extraction path of `json_extract_value`. Handle: `\"`, `\\`, `\/`, `\n`, `\r`, `\t`, `\b`, `\f`. For `\uXXXX`, pass through the 6 characters as-is (not needed for this project).

Buffer strategy: allocate `raw_len + 1` bytes (safe upper bound — unescaped output is always <= raw length). Write to a separate dst pointer. NUL-terminate at dst position.

### 2. Refactor: consistent string scanning in `json_find_key`

Current behavior: the key-end scanner uses a simple `while (*key_end && *key_end != '"')` loop with basic `\\` skip. This is functionally correct but inconsistent in style with the rest of the parser.

Refactor: unify string scanning approach for readability. Not a correctness fix — no known failure case.

### 3. Silent missing keys

Remove these two specific `fprintf(stderr, ...)` calls that fire on expected conditions:

- `json_find_key` (line 422): `"key %s not found\n"` — callers check `NULL` return
- `get_obj` (line 538): `"Key %s is not found"` — callers check `NULL` return

Keep all other stderr output (allocation failures in `json_alloc`, malformed JSON in `find_matching_bracket`, etc.) — these are genuine errors.

## Tests

New file: `tests/test_json.c`

Follows existing test pattern (custom pass/fail counter framework, same as `test_prayertimes.c`).

Test cases:

**Basic extraction:**
- String value: `{"name": "muslimtify"}` → `"muslimtify"`
- Number value: `{"count": 42}` → `"42"`
- Boolean value: `{"enabled": true}` → `"true"`
- Null value: `{"value": null}` → `"null"`
- Empty string: `{"key": ""}` → `""` (non-NULL pointer to empty string)

**Nested objects:**
- `{"location": {"lat": 1.5}}` → `get_value("location")` returns `{"lat": 1.5}`, then `get_value("lat", result)` returns `"1.5"`

**Missing keys:**
- `get_value("nonexistent", ...)` returns `NULL` (no crash, no stderr)

**Escaped strings:**
- `{"msg": "hello\"world"}` → `hello"world`
- `{"path": "c:\\dir"}` → `c:\dir`
- `{"text": "line1\nline2"}` → `line1` + newline + `line2`
- `{"u": "\u0041"}` → `\u0041` (6 literal chars, pass-through)

**Edge cases:**
- Empty object `{}` — `get_value("key", "{}")` returns `NULL`
- Deeply nested: verify depth limit works (>100 levels returns `NULL`)
- Key appearing inside a value doesn't confuse scanner: `{"city": "lat=1.0", "lat": "2.0"}` → `get_value("lat")` returns `"2.0"`

**Lifecycle:**
- `json_begin()` returns non-NULL
- `json_end(NULL)` doesn't crash (silent, no stderr)
- Multiple `get_value` calls on same context work — earlier results remain valid after later calls (arena does not overwrite)

**Regression guard:** `test_config`'s existing `test_round_trip` serves as regression for the unescaping change. Run before and after.

**CMake:** Add `test_json` executable and CTest entry, following `test_prayertimes` pattern (link only `-lm`, no system libraries). The test file defines `JSON_IMPLEMENTATION` directly.

## Out of scope

- Algorithmic changes (single-pass tokenizer, tree building)
- `\uXXXX` unicode escape processing (pass-through only)
- JSON generation/builder functions
- Performance optimization
