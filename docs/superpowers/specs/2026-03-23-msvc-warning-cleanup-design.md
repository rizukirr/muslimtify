## Goal

Eliminate MSVC warning set (C4200, C4116, C4996, C4244, C4996 wide-char variants) while improving safety and preserving portability across Linux/MSVC builds.

## Scope

- `src/json.h` — arena `Region` struct (C4200) and anonymous helper type sites (C4116).
- `src/config.c` — all unsafe string APIs (`strcpy`, `strncpy`, `strncat`, `strtok`, `strerror`).
- `src/cache.c` — bounded path/city copying via helper layer.
- `src/location.c` — replace `strncpy` usages pulling data from JSON.
- `src/notification_win.c` — swap `wcscpy` calls for bounded equivalents.
- `tests/test_prayertimes.c` — replace `sscanf` with MSVC-safe macro.
- Verification targets: `muslimtify`, `test_json`, `test_prayertimes`, plus config/cache tests to ensure all call sites are covered.

## Constraints

- Remain C99 compliant (GCC/Clang/MSVC). No `_CRT_SECURE_NO_WARNINGS` or MSVC-only pragmas as the primary fix.
- Avoid behavior changes in parsing, config persistence, notifications, unless needed for safety.
- Keep allocations single-pass where possible (arena).

## Design

### 1. Arena Struct Cleanup (`json.h`)

- Replace zero-length trailing array with explicit pointer:
  - `typedef struct Region { struct Region *next; size_t cap; size_t index; uint8_t *data; } Region;`
  - Allocate `Region` plus block payload in one call, then set `region->data = (uint8_t *)(region + 1);`.
  - Update accesses (`arena->current->data`) to follow pointer.
- Result: no zero-sized members, consistent layout across compilers.
- `json_alloc_free` already walks each region and frees it; only the allocation path needs to switch to `sizeof(Region) + block_size`.

### 2. Named Helper Types / Functions (`json.h`)

- Replace each anonymous struct with a named typedef/function:
  - `Region` (arena blocks) → handled in §1.
  - `struct { char c; type d; }` in `ARENA_ALIGNOF` macro → introduce `typedef struct AlignProbe AlignProbe;` and use it.
  - `struct { const char *start; size_t len; }` slice returned from `json_extract_value` → create `typedef struct JsonSlice JsonSlice;`.
  - Any inline comparator or depth trackers in the header (lines ~373/441/457/473) → convert into dedicated static
    functions returning primitive values (e.g., `static bool json_is_depth_limit(...)`).
- Move their fields into named typedefs or convert logic into dedicated static functions returning primitives.
- Update function signatures to use `JsonSlice`, `JsonToken`, etc., instead of anonymous aggregates.
- Update `tests/test_json.c` as needed to assert helper conversions keep behavior (string unescape, nested object parsing, etc.).

### 3. Safe String Utility Layer

#### Helper API

- Provide a single helper module with the following exported contracts:

  ```c
  bool copy_string(char *dst, size_t dst_size, const char *src);
  // Preconditions: dst && src, dst_size > 0. Copies up to dst_size-1 bytes; always writes '\0'. Returns true if
  // the entire source fits, false if truncation occurred or args invalid.

  bool append_string(char *dst, size_t dst_size, const char *src);
  // Preconditions: dst && src, dst_size > 0. Appends to existing string (using a local `bounded_strlen` helper to clamp).
  // Returns true if everything fit, false when truncation occurred or args invalid. Caller logs on false.

  size_t bounded_strlen(const char *s, size_t maxlen);
  // Portable `strnlen` replacement compiled once so MSVC + C99 builds do not rely on extensions.

  bool errno_string(int err, char *buf, size_t buf_sz);
  // Caller supplies buffer (>=64 bytes). `_WIN32`: call `strerror_s`. Else if `defined(__GLIBC__) && defined(_GNU_SOURCE)`
  // (only when already defined by the toolchain) use GNU strerror_r returning `char *`. Otherwise fall back to XSI
  // strerror_r (int return). Never call plain strerror, and we do not add new `_GNU_SOURCE` defines.

  bool copy_wstring(wchar_t *dst, size_t dst_len, const wchar_t *src);
  // `_WIN32` only. dst_len counts wchar_t slots (including room for '\0'); returns false on truncation/invalid args.

  typedef bool (*token_cb)(const char *token, void *user);
  int parse_tokens(const char *input, char *scratch, size_t scratch_size, const char *delims,
                   token_cb cb, void *user);
  // Helper copies `input` into `scratch` via copy_string; if truncation occurs it returns -1 immediately. Tokenization
  // then operates on `scratch`. On `_WIN32`, call `strtok_s` with a local `char *context` variable (initialized to NULL)
  // and pass it in/out per invocation; elsewhere use `strtok_r`. Each token is passed to `cb`; returning false stops
  // parsing and yields -2. Successful full iteration returns 0.
  // Example for `config_parse_reminders`: define a small struct holding the `int *reminders`, `int max_reminders`, and
  // current count; the callback parses each token via `strtol`, appends to the array, and returns false if the array is
  // full so parse_tokens exits early with -2. `config_parse_reminders` should pass delimiters `", "` (comma + space)
  // and treat return codes as: 0 success, -1 invalid input (return -1 to caller), -2 meaning "array full" (return the
  // populated count as success).
  ```

- Helpers operate purely on buffers; arena allocations continue using existing `json_alloc` APIs directly.
- Implementation lives in a new private pair (`src/string_util.h` + `src/string_util.c`) that is added to
  `muslimtify_lib` in `CMakeLists.txt` (so all executables/tests already linking the lib inherit the helpers). The
  header only exposes prototypes; all logic (including platform-specific branches, `_WIN32` copy_wstring, bounded_strlen)
  lives in the `.c` file so we avoid duplicated inline implementations.

#### File-specific replacements

- `config.c`:
  - Replace `strcpy`/`strncpy` writes with `copy_string` (timezone, city, country, icon, etc.).
  - Replace `strncat` usage in `config_format_reminders` with `append_string`, logging when truncation occurs.
  - Use `parse_tokens` within `config_parse_reminders`; reuse the existing 256-byte stack buffer as `scratch`.
  - Replace `strerror` usage with `errno_string`, feeding the caller’s stack buffer to `fprintf`.
  - Resolve MSVC warning C4244 in `config_get_prayer` (line ~453) by storing `tolower` results in an `int tmp` before
    casting to `char`; no other C4244 sites are currently reported.

- `cache.c` / `location.c` / `notification_win.c`:
  - Mirror the helper usage to replace `strncpy`/`wcscpy` with bounded copies.
  - Introduce `bool copy_wstring(wchar_t *dst, size_t dst_len, const wchar_t *src);` inside `string_util.*`
    (declared in the header behind `_WIN32` guards) — `dst_len` counts `wchar_t` slots (including room for '\0').
    Requires `dst_len > 0`, writes terminator, returns false when truncation occurs so callers can log or fallback.

#### Truncation / Error Handling Policy

- `config.c` load/save paths: if `copy_string`/`append_string` return false, log `fprintf(stderr, "config: value '%s' truncated\n", key)`
  but continue with the clamped value (matches existing behavior of `strncpy`). `parse_tokens` returning -1 aborts parsing that field and leaves
  reminders unchanged; returning -2 stops early and uses the reminders collected so far.
- `config_format_reminders`: stop appending when `append_string` fails; resulting buffer is truncated representation.
- `cache.c` and `location.c`: log once when API data exceeds buffer, continue (values come from external services; better to have truncated strings than abort). Implement via static module-level flags (e.g., `static bool cache_trunc_logged`) that reset only when the process restarts; log messages should mention the trimmed JSON key (e.g., `"city"`).
- `notification_win.c`: treat `copy_wstring` failure as fatal for the toast build — abort notification and log error to avoid emitting malformed XML.

- `tests/test_prayertimes.c`:
  - Introduce wrapper macro to handle MSVC’s extra argument requirements:

    ```c
    #if defined(_MSC_VER)
    #define PARSE_TIME(h, m, str) sscanf_s((str), "%d:%d", &(h), &(m))
    #else
    #define PARSE_TIME(h, m, str) sscanf((str), "%d:%d", &(h), &(m))
    #endif
    ```

    - Replace direct `sscanf` call in `time_to_minutes` with `PARSE_TIME`.

#### Helper Tests

- Add `tests/test_string_util.c` (linked with `muslimtify_lib`) that exercises `copy_string`, `append_string`, `bounded_strlen`,
  and `parse_tokens`/`copy_wstring` happy-path + truncation scenarios. Register it in CMake/CTest so it runs cross-platform.

### 4. Verification

1. `cmake --build build --config Debug` on Windows (and `cmake --build build` on Linux) — confirm compiler/linker output is warning-free.
2. `cmake --build build --config Release` on Windows — ensure optimized builds are also warning-free.
3. `ctest --test-dir build -R json --output-on-failure` — covers parser changes.
4. `ctest --test-dir build -R prayertimes --output-on-failure` — validates `PARSE_TIME` macro.
5. `ctest --test-dir build -R config --output-on-failure` and `ctest --test-dir build -R cache --output-on-failure` (Linux-only per AGENTS.md) — exercise refactored string helpers. On Windows, rely on manual CLI validation instead.
6. Manual CLI run (`build/bin/Debug/muslimtify.exe location auto` on Windows or `build/bin/muslimtify location auto` on Linux) to sanity-check `location.c`; on Windows, trigger a toast notification (existing CLI verb) to cover `notification_win.c` end-to-end.
7. Confirm MSVC build logs show zero instances of C4200/C4116/C4996/C4244 (CI already uses `/WX`, so any warning reappearance fails the build).

## Risks & Mitigation

- **Behavior changes in string handling**: ensure helpers preserve existing truncation semantics (explicit tests / assertions).
- **Arena pointer math errors**: add asserts verifying `region->data` non-null; keep existing allocation sizes.
- **Token parsing**: need to confirm replacements maintain original functionality (add tests if necessary).

## Timeline

1. Implement arena + helper type updates (~1 day).
2. Add string utility layer + refactor config/cache/location (~1-2 days depending on scope).
3. Update `notification_win.c` and tests (~0.5 day).
4. Full build/test cycle.
