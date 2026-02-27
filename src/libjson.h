/*
 * Copyright (c) 2025 Rizki Rakasiwi <rizkirr.xyz@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef LIBJSON_H
#define LIBJSON_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Define restrict keyword for portability
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
// C99 or later - restrict is available
#define JSON_RESTRICT restrict
#elif defined(__GNUC__) || defined(__clang__)
// GCC/Clang extension
#define JSON_RESTRICT __restrict__
#elif defined(_MSC_VER)
// MSVC extension
#define JSON_RESTRICT __restrict
#else
// No restrict support
#define JSON_RESTRICT
#endif

typedef struct JsonContext JsonContext;

/**
 * Initialize a new JSON parsing context.
 * Returns a new JsonContext or NULL on failure.
 */
JsonContext *json_begin(void);

/**
 * Free the JSON context and all associated memory.
 * @param ctx The context to free
 */
void json_end(JsonContext *ctx);

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

/**
 * Extract a value for the given key from a JSON object.
 * @param ctx The JSON context
 * @param key The key to search for
 * @param raw_json The raw JSON object string
 * @return Pointer to the extracted value string, or NULL if not found
 */
char *get_value(JsonContext *JSON_RESTRICT ctx, const char *JSON_RESTRICT key,
                char *JSON_RESTRICT raw_json);

#ifdef LIBJSON_IMPLEMENTATION

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ARENA

#define DA_INIT_CAP 256

#define da_append(da, item)                                                    \
  do {                                                                         \
    if ((da)->capacity == 0) {                                                 \
      (da)->capacity = DA_INIT_CAP;                                            \
      (da)->items = malloc(sizeof((da)->items[0]) * (da)->capacity);           \
      if ((da)->items == NULL) {                                               \
        fprintf(stderr, "Buy more RAM LOL!\n");                                \
        exit(1);                                                               \
      }                                                                        \
    }                                                                          \
                                                                               \
    if ((da)->count == (da)->capacity) {                                       \
      (da)->capacity *= 2;                                                     \
      (da)->items =                                                            \
          realloc((da)->items, sizeof((da)->items[0]) * (da)->capacity);       \
      if ((da)->items == NULL) {                                               \
        fprintf(stderr, "Buy more RAM LOL!\n");                                \
        exit(1);                                                               \
      }                                                                        \
    }                                                                          \
    (da)->items[(da)->count++] = item;                                         \
  } while (0)

#define da_clear(da)                                                           \
  do {                                                                         \
    (da)->count = 0;                                                           \
    (da)->capacity = 0;                                                        \
    free((da)->items);                                                         \
  } while (0)

#if __STDC_VERSION__ >= 201112L
#include <stdalign.h>
#define ARENA_ALIGNOF(type) alignof(type)
#else
#define ARENA_ALIGNOF(type)                                                    \
  offsetof(                                                                    \
      struct {                                                                 \
        char c;                                                                \
        type d;                                                                \
      },                                                                       \
      d)
#endif

#define JSON_OBJECT_OPEN '{'
#define JSON_OBJECT_CLOSE '}'
#define JSON_ARRAY_OPEN '['
#define JSON_ARRAY_CLOSE ']'
#define JSON_STRING_QUOTE '"'
#define JSON_ESCAPE_CHAR '\\'
#define JSON_KEY_VALUE_SEP ':'
#define JSON_VALUE_SEP ','

#ifndef ARENA_BLOCK_SIZE
#define ARENA_BLOCK_SIZE (1024 * 4)
#endif

#define JSON_DEPTH_LIMIT 100

typedef struct {
  struct Region *next;
  size_t cap;
  size_t index;
  uint8_t data[];
} Region;

typedef struct {
  Region *head;
  Region *current;
  size_t block_size;
} JsonArena;

static size_t json_alloc_align(uintptr_t ptr, size_t alignment) {
  return (alignment - (ptr % alignment)) % alignment;
}

static JsonArena *json_alloc_init(void) {
  JsonArena *arena = (JsonArena *)malloc(sizeof(JsonArena));
  if (arena == NULL) {
    fprintf(stderr, "Memory allocation fail, Buy more RAM LOL!\n");
    return NULL;
  }

  arena->block_size = ARENA_BLOCK_SIZE;
  arena->head = NULL;
  arena->current = NULL;
  return arena;
}

static void *json_alloc(JsonArena *arena, size_t size, size_t alignment) {
  if (!arena || size == 0 || alignment == 0) {
    fprintf(stderr, "Invalid arguments to json_alloc\n");
    return NULL;
  }

  // Check if alignment is a power of two (expected to be true)
#if defined(__GNUC__) || defined(__clang__)
  if (__builtin_expect((alignment & (alignment - 1)) != 0, 0))
#else
  if (alignment & (alignment - 1))
#endif
    return NULL;

  if (!arena->current) {
    size_t block_size = (size > arena->block_size) ? size : arena->block_size;
    Region *region = malloc(sizeof(Region) + block_size);
    if (region == NULL) {
      fprintf(stderr, "Memory allocation fail, Buy more RAM LOL!\n");
      return NULL;
    }

    region->next = NULL;
    region->cap = block_size;
    region->index = 0;
    arena->head = arena->current = region;
  }

  uintptr_t current_ptr =
      (uintptr_t)(arena->current->data + arena->current->index);
  size_t padding = json_alloc_align(current_ptr, alignment);

  if (arena->current->index + padding + size > arena->current->cap) {
    size_t next_cap = (size > arena->block_size) ? size : arena->block_size;
    Region *next = malloc(sizeof(Region) + next_cap);
    if (!next) {
      fprintf(stderr, "Memory allocation fail, Buy more RAM LOL!\n");
      return NULL;
    }

    next->next = NULL;
    next->cap = next_cap;
    next->index = 0;

    arena->current->next = (struct Region *)next;
    arena->current = next;

    current_ptr = (uintptr_t)next->data;
    padding = json_alloc_align(current_ptr, alignment);
  }

  arena->current->index += padding;
  void *ptr = arena->current->data + arena->current->index;
  arena->current->index += size;

  return ptr;
}

void json_alloc_free(JsonArena *arena) {
  if (!arena)
    return;
  Region *region = arena->head;
  while (region) {
    Region *next = (Region *)region->next;
    free(region);
    region = next;
  }
  free(arena);
}

// Json Parser
typedef struct {
  size_t count;
  size_t capacity;
  char **items;
} JsonArray;

struct JsonContext {
  JsonArena *arena;
  JsonArray *stack;
};

static inline const char *skip_whitespace(const char *cursor) {
  while (*cursor && isspace(*cursor))
    cursor++;
  return cursor;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static const char *skip_string(const char *cursor) {
  if (*cursor != JSON_STRING_QUOTE) {
    fprintf(stderr, "Expected quote got %s\n", cursor);
    return NULL;
  }

  cursor++; // Skip opening quote
  bool escaped = false;

  while (*cursor) {
    if (escaped) {
      escaped = false;
    } else if (*cursor == JSON_ESCAPE_CHAR) {
      escaped = true;
    } else if (*cursor == JSON_STRING_QUOTE) {
      return cursor + 1; // Return pointer after closing quote
    }
    cursor++;
  }

  fprintf(stderr, "Unterminated string %s\n", cursor);
  return NULL;
}

static const char *find_matching_bracket(const char *start, char open_bracket) {
  if (*start != open_bracket) {
    fprintf(stderr, "expected '%c'\n", open_bracket);
    return NULL;
  }

  char close_bracket =
      (open_bracket == JSON_OBJECT_OPEN) ? JSON_OBJECT_CLOSE : JSON_ARRAY_CLOSE;
  const char *cursor = start + 1;
  int depth = 1;
  bool in_string = false;
  bool escaped = false;

  while (*cursor && depth > 0) {
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (*cursor == JSON_ESCAPE_CHAR) {
        escaped = true;
      } else if (*cursor == JSON_STRING_QUOTE) {
        in_string = false;
      }
    } else {
      if (*cursor == JSON_STRING_QUOTE) {
        in_string = true;
      } else if (*cursor == open_bracket) {
        depth++;
        // Enforce depth limit
        if (depth > JSON_DEPTH_LIMIT) {
          fprintf(stderr, "depth exceeds limit %d\n", JSON_DEPTH_LIMIT);
          return NULL;
        }
      } else if (*cursor == close_bracket) {
        depth--;
        if (depth == 0) {
          return cursor;
        }
      }
    }
    cursor++;
  }

  fprintf(stderr, "no matching '%c'\n", close_bracket);
  return NULL;
}

static char *json_find_key(const char *JSON_RESTRICT key, size_t key_len,
                           char *JSON_RESTRICT json) {
  if (!key || !json) {
    fprintf(stderr, "Invalid arguments, expected key and raw json\n");
    return NULL;
  }

  const char *cursor = json;
  int depth = 0;
  bool in_string = false;
  bool escaped = false;

  size_t target_len = key_len;

  while (*cursor) {
    char current_char = *cursor;

    if (in_string) {
      if (escaped)
        escaped = false; // this character is escaped, move on
      else if (current_char == '\\')
        escaped = true; // Next character will be escaped
      else if (current_char == '"')
        in_string = false; // end of string

      cursor++;
      continue;
    }

    switch (current_char) {
    case JSON_STRING_QUOTE:
      if (depth == 1) {
        const char *key_start = cursor + 1;
        const char *key_end = key_start;

        // Find the closing quote
        while (*key_end && *key_end != '"') {
          if (*key_end == '\\')
            key_end++; // Skip the escaped quote
          key_end++;   // Skip the quote
        }

        if (!key_end)
          continue;

        size_t key_len = key_end - key_start;

        if (key_len == target_len && strncmp(key_start, key, key_len) == 0) {
          // skip past the closing quote and find the colon
          cursor = skip_whitespace(key_end + 1);

          if (*cursor == ':') {
            // skip past the colon and find the value
            cursor = skip_whitespace(cursor + 1);
            return (char *)cursor;
          }
        }
      }
      in_string = true;
      break;
    case JSON_ARRAY_OPEN:
    case JSON_OBJECT_OPEN:
      depth++;
      if (depth > JSON_DEPTH_LIMIT) {
        fprintf(stderr, "depth exceeds limit %d\n", JSON_DEPTH_LIMIT);
        return NULL;
      }
      break;
    case JSON_ARRAY_CLOSE:
    case JSON_OBJECT_CLOSE:
      depth--;
      break;
    }

    cursor++;
  }

  fprintf(stderr, "key %s not found\n", key);
  return NULL;
}

static char *json_extract_value(JsonArena *JSON_RESTRICT arena,
                                const char *JSON_RESTRICT value_start) {

  if (!value_start || !arena) {
    fprintf(stderr, "Expected JsonArena and value_start");
    return NULL;
  }

  size_t value_len;
  const char *cursor = skip_whitespace(value_start);
  const char *value_end;

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

  } else if (*cursor == '{' || *cursor == '[') {
    // Object or array - use helper to find matching bracket
    const char *start = cursor;
    char open = *cursor;
    value_end = find_matching_bracket(cursor, open);

    if (!value_end)
      return NULL;

    value_len = value_end - start + 1; // Include closing bracket
    char *result = json_alloc(arena, value_len + 1, ARENA_ALIGNOF(char));
    if (!result) return NULL;
    memcpy(result, start, value_len);
    result[value_len] = '\0';
    return result;

  } else {
    // Number, boolean, or null - find end
    value_end = cursor;
    while (*value_end && !isspace(*value_end) && *value_end != ',' &&
           *value_end != '}' && *value_end != ']') {
      value_end++;
    }

    value_len = value_end - cursor;
    char *result = json_alloc(arena, value_len + 1, ARENA_ALIGNOF(char));
    if (!result) return NULL;
    memcpy(result, cursor, value_len);
    result[value_len] = '\0';
    return result;
  }
}

JsonContext *json_begin(void) {
  JsonArena *arena = json_alloc_init();
  if (!arena) {
    fprintf(stderr, "Cannot allocate memory arena for json_begin\n");
    return NULL;
  }

  JsonContext *ctx =
      json_alloc(arena, sizeof(JsonContext), ARENA_ALIGNOF(JsonContext));
  if (!ctx) {
    fprintf(stderr, "Cannot create JsonContext, please check your RAM usage\n");
    json_alloc_free(arena);
    return NULL;
  }

  ctx->stack = json_alloc(arena, sizeof(JsonArray), ARENA_ALIGNOF(JsonArray));
  if (!ctx->stack) {
    fprintf(stderr, "Failed to allocate JsonArray\n");
    json_alloc_free(arena);
    return NULL;
  }
  ctx->stack->count = 0;
  ctx->stack->capacity = 0;
  ctx->stack->items = NULL;
  ctx->arena = arena;
  return ctx;
}

void json_end(JsonContext *ctx) {
  if (!ctx) {
    fprintf(stderr, "Invalid context to json_end\n");
    return;
  }

  if (ctx->stack) {
    da_clear(ctx->stack);
  }
  json_alloc_free(ctx->arena);
}

static inline char *get_obj(JsonArena *JSON_RESTRICT arena,
                            char *JSON_RESTRICT json,
                            const char *JSON_RESTRICT key) {
  if (!json || !arena || !key) {
    fprintf(stderr, "Invalid arguments to get_obj\n");
    return NULL;
  }

  size_t keylen = strlen(key);
  char *value = json_find_key(key, keylen, json);
  if (!value) {
    fprintf(stderr, "Key %s is not found", key);
    return NULL;
  }

  char *result = json_extract_value(arena, value);
  return result;
}

char **get_array(JsonContext *JSON_RESTRICT ctx, const char *JSON_RESTRICT key,
                 char *JSON_RESTRICT raw_json, size_t *JSON_RESTRICT count) {
  if (!ctx || !ctx->arena || !key) {
    fprintf(stderr, "Invalid arguments to get_array\n");
    return NULL;
  }

  char *array_start = raw_json;

  // Skip whitespace
  array_start = (char *)skip_whitespace(array_start);

  // Verify it's an array
  if (*array_start != '[') {
    fprintf(stderr, "value is not an array");
    return NULL;
  }

  // Start parsing after '['
  const char *cursor = array_start + 1;

  while (*cursor) {
    cursor = skip_whitespace(cursor);

    // Check for end of array
    if (*cursor == ']')
      break;

    // Skip commas
    if (*cursor == ',') {
      cursor++;
      continue;
    }

    // Found an object - extract from { to }
    if (*cursor == '{') {
      const char *obj_start = cursor;
      const char *obj_end = find_matching_bracket(cursor, '{');

      if (!obj_end)
        return NULL;

      // Extract and append the object
      size_t len = obj_end - obj_start + 1; // Include closing brace
      char *str = json_alloc(ctx->arena, len + 1, ARENA_ALIGNOF(char));
      memcpy(str, obj_start, len);
      str[len] = '\0';
      da_append(ctx->stack, str);

      cursor = obj_end + 1;
    } else {
      // Unexpected character
      fprintf(stderr, "expected '{' but found '%c'", *cursor);
      return NULL;
    }
  }
  *count = ctx->stack->count;
  return ctx->stack->items;
}

char *get_value(JsonContext *JSON_RESTRICT ctx, const char *JSON_RESTRICT key,
                char *JSON_RESTRICT raw_json) {
  if (!ctx || !ctx->arena)
    return NULL;

  return get_obj(ctx->arena, raw_json, key);
}

#endif /* LIBJSON_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

#endif /* LIBJSON_H */
