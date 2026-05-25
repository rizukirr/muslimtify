#ifndef COUNTRY_H
#define COUNTRY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * One ISO 3166-1 alpha-2 country record.
 *
 * `code` is the uppercase two-letter code (NUL-terminated, e.g. "ID").
 * `name` is the English short name (e.g. "Indonesia"). `name` is provided for
 * a future name-lookup/display feature and is currently read by no caller.
 */
typedef struct {
  char code[3];
  const char *name;
} Country;

/**
 * Returns true iff `code` is exactly two letters that, uppercased, match a
 * known ISO 3166-1 alpha-2 code. Does not mutate `code`. Returns false for
 * NULL, empty, wrong length, or non-letter input.
 */
bool country_is_valid_alpha2(const char *code);

#ifdef __cplusplus
}
#endif

#endif // COUNTRY_H
