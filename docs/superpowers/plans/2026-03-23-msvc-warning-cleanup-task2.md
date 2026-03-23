# MSVC Warning Cleanup Task 2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Introduce the shared string utility module so every target can rely on the same safe helpers before refactoring callers in later tasks.

**Architecture:** A single internal module (`src/string_util.c/.h`) provides all helper logic, keeping platform-specific branches centralized while exposing portable prototypes to the rest of the codebase via `muslimtify_lib`.

**Tech Stack:** C99, CMake (muslimtify_lib), MSVC/GCC toolchains, standard C library string APIs.

---

## File Structure

- `src/string_util.h` (new): declares helper prototypes, callback typedef, and documents contracts.
- `src/string_util.c` (new): implements all helpers (`copy_string`, `append_string`, `bounded_strlen`, `errno_string`, `_WIN32` `copy_wstring`, and `parse_tokens`) with platform-specific branches and shared logging helpers.
- `CMakeLists.txt`: adds `src/string_util.c` to the `muslimtify_lib` target so every binary/test links the helper automatically.

---

### Task 1: Declare Shared Helper Prototypes

**Files:**
- Create: `src/string_util.h`

- [ ] **Step 1: Scaffold header file**

Create `src/string_util.h` with an include guard (`STRING_UTIL_H`). Include `<stddef.h>`, `<stdbool.h>`, and guard `<wchar.h>` usage with `#ifdef _WIN32`.

- [ ] **Step 2: Add documented prototypes**

Define prototypes per spec with brief contract comments so future callers know return semantics:

```c
bool copy_string(char *dst, size_t dst_size, const char *src);
bool append_string(char *dst, size_t dst_size, const char *src);
size_t bounded_strlen(const char *s, size_t maxlen);
bool errno_string(int err, char *buf, size_t buf_sz);
#ifdef _WIN32
bool copy_wstring(wchar_t *dst, size_t dst_len, const wchar_t *src);
#endif
typedef bool (*token_cb)(const char *token, void *user);
int parse_tokens(const char *input, char *scratch, size_t scratch_size,
                 const char *delims, token_cb cb, void *user);
```

- [ ] **Step 3: Run clang-format on header**

Command: `clang-format -i src/string_util.h`

- [ ] **Step 4: Stage/inspect header**

Command: `git add src/string_util.h && git diff --cached src/string_util.h`

### Task 2: Implement Helper Logic

**Files:**
- Create: `src/string_util.c`

- [ ] **Step 1: Include dependencies**

Add `#include <stdio.h>`, `<string.h>`, `<errno.h>`, `<stdint.h>`, `<wchar.h>` (guarded), and `src/string_util.h`.

- [ ] **Step 2: Implement `bounded_strlen` and validation helpers**

Provide a static `static bool validate_copy_args(const char *name, char *dst, size_t dst_size, const char *src);` that logs invalid arguments once per helper. Then implement `bounded_strlen` as a capped loop.

- [ ] **Step 3: Implement `copy_string`/`append_string`**

Use `bounded_strlen` to compute `src_len`, copy with `memcpy`, write the null terminator, and return false when `src_len >= dst_size`. For `append_string`, find `dst_len` first and bail if `dst_len >= dst_size`.

- [ ] **Step 4: Implement `errno_string`**

Handle `_WIN32` via `strerror_s`. Else branch handles GNU vs XSI `strerror_r`, copying results into the provided buffer and falling back to `snprintf(buf, buf_sz, "unknown error %d", err);` when all else fails.

- [ ] **Step 5: Implement `_WIN32` `copy_wstring`**

Mirror `copy_string` semantics but operate on `wchar_t`. Use `wcslen` guarded with manual loop to respect `dst_len`.

- [ ] **Step 6: Implement `parse_tokens`**

Copy `input` into `scratch` (calling `copy_string`); return `-1` if it fails. Tokenize using `strtok_s` + context pointer on Windows and `strtok_r` on other platforms. For each token, call the callback; stop early and return `-2` if it returns false. Return 0 on success, -1 on invalid args (null inputs, zero scratch size).

- [ ] **Step 7: Format and stage implementation**

Commands: `clang-format -i src/string_util.c` then `git add src/string_util.c && git diff --cached src/string_util.c`

### Task 3: Integrate Module Into Build

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add source file to muslimtify_lib**

Locate the `add_library(muslimtify_lib OBJECT ... )` block and append `src/string_util.c` to the source list.

- [ ] **Step 2: Save and stage**

Command: `git add CMakeLists.txt && git diff --cached CMakeLists.txt`

### Task 4: Build Debug Configuration

**Files:**
- Build artifacts in `build/`

- [ ] **Step 1: Configure (if needed)**

Command: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`

- [ ] **Step 2: Build muslimtify_lib dependents**

Command: `cmake --build build --config Debug`

- [ ] **Step 3: Capture results**

Document warnings/errors (expect clean build). If build fails, inspect logs and fix prior steps before rerunning.

---

Plan completion checklist: perform all tasks in order, keep work inside `.worktrees/msvc-warning-cleanup-task1`, and defer adding tests until Task 3 of the broader plan.
