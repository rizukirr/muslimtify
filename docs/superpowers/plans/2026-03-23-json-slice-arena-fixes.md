# JSON Slice/Arena Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make JSON key comparisons honor escape sequences and ensure arena allocations always leave enough room for alignment padding.

**Architecture:** Extend `json_slice_equals` to decode escape sequences on the fly while iterating through the slice and key, rejecting unsupported `\u` ranges above 0xFF. Update the arena allocator so every region request includes `alignment - 1` bytes of slack before sizing blocks, preventing overflow when padding is added.

**Tech Stack:** C99, custom JSON parser (`src/json.h`), unit tests under `tests/test_json.c`.

---

### Task 1: Decode JSON slice keys before comparison

**Files:**
- Modify: `src/json.h` (json_slice_equals implementation and related helpers)
- Modify: `tests/test_json.c` (add coverage for escaped keys)

- [ ] **Step 1: Add regression tests for escaped-key lookups**

  Update `tests/test_json.c` so at least one test feeds raw JSON containing escaped key sequences (e.g., `"line\nname"`, `"quote\"key"`, and `\u00E9`). Assert `get_value` returns expected values when queried with the unescaped key strings. Use existing helper macros/utilities for assertions.

- [ ] **Step 2: Run focused tests to confirm new case fails**

  ```bash
  cmake --build build --config Debug
  ctest --test-dir build -C Debug -R json --output-on-failure
  ```

  Expect the updated test to fail because escaped keys aren't matched yet.

- [ ] **Step 3: Implement escape-aware comparison**

  Inside `src/json.h`, rework `json_slice_equals` so it walks both the slice and `rhs` C-string simultaneously. Parse JSON escape sequences: simple escapes (`\"`, `\\`, `\/`, `\b`, `\f`, `\n`, `\r`, `\t`) translate to their control characters; `\uXXXX` parses four hex digits into a byte, rejecting values above `0xFF`. Compare each decoded byte to the current char from `rhs`, returning false on mismatch or malformed escapes. Confirm both strings terminate together. Keep runtime comments noting the `\u` >0xFF rejection.

- [ ] **Step 4: Re-run json tests**

  ```bash
  cmake --build build --config Debug
  ctest --test-dir build -C Debug -R json --output-on-failure
  ```

  Expect all json tests to pass.

### Task 2: Ensure arena regions include padding slack

**Files:**
- Modify: `src/json.h` (json_alloc/json_region_create logic)
- Modify: `tests/test_json.c` (add allocator regression test if coverage is light)

- [ ] **Step 1: Add/extend test covering large aligned allocation**

  In `tests/test_json.c`, create a scenario that requests an allocation whose size exceeds `ARENA_BLOCK_SIZE` and requires non-zero padding (e.g., align to `JSON_ALIGNOF(double)`), verifying the allocator returns non-NULL and doesn't overflow. If a helper already exercises the arena, extend it; otherwise, add a dedicated test guarded behind arena feature availability.

- [ ] **Step 2: Run json test suite to capture current failure**

  ```bash
  cmake --build build --config Debug
  ctest --test-dir build -C Debug -R json --output-on-failure
  ```

  Expect the new allocator test to fail, demonstrating insufficient padding.

- [ ] **Step 3: Update allocator to include slack**

  In `json_alloc`, when determining `block_size` for the first region and `next_cap` for subsequent regions, add `alignment - 1` to the requested `size` before taking the max with `arena->block_size`. Also adjust `json_region_create` callers so even oversized single allocations request `size + alignment - 1` capacity. This guarantees `padding + size` never exceeds `cap`. Document why the slack is necessary.

- [ ] **Step 4: Rebuild and rerun targeted tests**

  ```bash
  cmake --build build --config Debug
  ctest --test-dir build -C Debug -R json --output-on-failure
  ```

  Ensure every json test passes, proving allocator safety.

---

Plan complete and ready for execution. Choose an execution mode (subagent-driven or inline) before proceeding.
