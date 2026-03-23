# JSON Task 1 Recheck Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reconfirm Task 1 changes so JSON key comparisons honor escapes and arena allocations always reserve padding slack per the libjson cleanup spec.

**Architecture:** Extend `json_slice_equals` to decode escapes while iterating over the slice and RHS key, and bump arena region sizes by `alignment - 1` bytes before allocation so later padding never spills beyond capacity.

**Tech Stack:** C99 (`src/json.h`), json unit tests in `tests/test_json.c`, CMake build with Debug configuration.

---

### Task 1A: Escape-aware key comparisons

**Files:**
- Modify: `src/json.h`
- Modify: `tests/test_json.c`
- Test: `ctest --test-dir build -C Debug -R json --output-on-failure`

- [ ] **Step 1: Add failing tests for escaped keys**

  Extend `tests/test_json.c` with a `test_escaped_keys` helper that loads JSON containing keys like `"line\nkey"`, `"quote\"key"`, and `"\u00E9"`. Assert `json_get_value` (or equivalent helper) can retrieve values using the unescaped key strings.

- [ ] **Step 2: Build + run json tests to observe failure**

  ```bash
  cmake --build build --config Debug
  ctest --test-dir build -C Debug -R json --output-on-failure
  ```

  Expect the new test to fail because `json_slice_equals` still compares raw escaped slices.

- [ ] **Step 3: Implement escape-aware `json_slice_equals`**

  Update `src/json.h` so `json_slice_equals` walks slice + RHS simultaneously, decoding `\"`, `\\`, `\/`, `\b`, `\f`, `\n`, `\r`, `\t`, and `\uXXXX`. For `\u`, parse four hex digits into a byte, reject values > `0xFF`, and ensure malformed escapes fail fast. Compare decoded chars against RHS bytes and ensure both strings end together before returning true.

- [ ] **Step 4: Rebuild + rerun json tests**

  ```bash
  cmake --build build --config Debug
  ctest --test-dir build -C Debug -R json --output-on-failure
  ```

  All json tests should now pass, proving escaped-key lookups work.

### Task 1B: Arena padding slack

**Files:**
- Modify: `src/json.h`
- Modify: `tests/test_json.c`
- Test: `ctest --test-dir build -C Debug -R json --output-on-failure`

- [ ] **Step 1: Add allocator regression test**

  In `tests/test_json.c`, add `test_arena_large_alignment` (or similar) that forces an allocation larger than `ARENA_BLOCK_SIZE` with `JSON_ALIGNOF(double)` alignment and asserts the arena returns non-NULL without overwriting guard bytes.

- [ ] **Step 2: Run tests to capture failure**

  ```bash
  cmake --build build --config Debug
  ctest --test-dir build -C Debug -R json --output-on-failure
  ```

  Expect the allocator test to fail because current regions lack slack for padding.

- [ ] **Step 3: Update allocator sizing**

  Modify `json_alloc` / region creation so requested `size` (including oversized single allocations) always adds `alignment - 1` before computing `block_size` / `next_cap`. This guarantees the subsequent padding added during allocation never exceeds the region capacity.

- [ ] **Step 4: Rebuild and rerun json tests**

  ```bash
  cmake --build build --config Debug
  ctest --test-dir build -C Debug -R json --output-on-failure
  ```

  All json tests should pass, confirming allocator slack matches spec.

---

Plan saved for Task 1 review. Choose an execution mode before implementing.
