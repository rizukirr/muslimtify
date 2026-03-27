#ifndef METHOD_DETECT_H
#define METHOD_DETECT_H

#include "prayertimes.h"
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char *country_code;
  CalcMethod method;
} CountryMethodEntry;

static const CountryMethodEntry COUNTRY_METHOD_MAP[] = {
    /* Indonesia */
    {"ID", CALC_KEMENAG},
    /* Malaysia */
    {"MY", CALC_JAKIM},
    /* Singapore */
    {"SG", CALC_SINGAPORE},
    /* Saudi Arabia, Yemen */
    {"SA", CALC_MAKKAH},
    {"YE", CALC_MAKKAH},
    /* North America */
    {"US", CALC_ISNA},
    {"CA", CALC_ISNA},
    /* Egypt, Libya, Sudan */
    {"EG", CALC_EGYPT},
    {"LY", CALC_EGYPT},
    {"SD", CALC_EGYPT},
    /* South Asia */
    {"PK", CALC_KARACHI},
    {"IN", CALC_KARACHI},
    {"BD", CALC_KARACHI},
    {"AF", CALC_KARACHI},
    /* Turkey */
    {"TR", CALC_TURKEY},
    /* UAE */
    {"AE", CALC_DUBAI},
    /* Qatar */
    {"QA", CALC_QATAR},
    /* Kuwait */
    {"KW", CALC_KUWAIT},
    /* Jordan, Palestine */
    {"JO", CALC_JORDAN},
    {"PS", CALC_JORDAN},
    /* Gulf region */
    {"BH", CALC_GULF},
    {"OM", CALC_GULF},
    /* North Africa */
    {"TN", CALC_TUNISIA},
    {"DZ", CALC_ALGERIA},
    {"MA", CALC_MOROCCO},
    /* Europe */
    {"PT", CALC_PORTUGAL},
    {"FR", CALC_FRANCE},
    {"RU", CALC_RUSSIA},
};

static inline int strcasecmp_portable(const char *a, const char *b) {
  while (*a && *b) {
    char ca = *a >= 'a' && *a <= 'z' ? (char)(*a - 32) : *a;
    char cb = *b >= 'a' && *b <= 'z' ? (char)(*b - 32) : *b;
    if (ca != cb)
      return ca - cb;
    a++;
    b++;
  }
  return (unsigned char)*a - (unsigned char)*b;
}

static inline CalcMethod method_detect_by_country(const char *country_code) {
  if (!country_code || country_code[0] == '\0')
    return CALC_MWL;

  size_t count = sizeof(COUNTRY_METHOD_MAP) / sizeof(COUNTRY_METHOD_MAP[0]);
  for (size_t i = 0; i < count; i++) {
    if (strcasecmp_portable(country_code, COUNTRY_METHOD_MAP[i].country_code) == 0)
      return COUNTRY_METHOD_MAP[i].method;
  }

  return CALC_MWL;
}

#ifdef __cplusplus
}
#endif

#endif /* METHOD_DETECT_H */
