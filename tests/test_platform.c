#include "platform.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static int total = 0;
static int failures = 0;

static void report_result(const char *label, bool pass) {
  total++;
  if (pass) {
    printf("  PASS: %s\n", label);
  } else {
    printf("  FAIL: %s\n", label);
    failures++;
  }
}

static void check_path(const char *label, const char *first, const char *second) {
  char non_empty_label[96];
  char stable_label[96];

  snprintf(non_empty_label, sizeof(non_empty_label), "%s is non-empty", label);
  snprintf(stable_label, sizeof(stable_label), "%s pointer is stable", label);

  report_result(non_empty_label,
                first != NULL && first[0] != '\0' && second != NULL && second[0] != '\0');
  report_result(stable_label, first == second);
}

static void test_platform_boundary(void) {
  printf("test_platform_boundary\n");

  const char *config_first = platform_config_dir();
  const char *config_second = platform_config_dir();
  check_path("platform_config_dir()", config_first, config_second);

  const char *cache_first = platform_cache_dir();
  const char *cache_second = platform_cache_dir();
  check_path("platform_cache_dir()", cache_first, cache_second);

  const char *home_first = platform_home_dir();
  const char *home_second = platform_home_dir();
  check_path("platform_home_dir()", home_first, home_second);

  const char *exe_first = platform_exe_dir();
  const char *exe_second = platform_exe_dir();
  check_path("platform_exe_dir()", exe_first, exe_second);
}

int main(void) {
  printf("=== platform boundary tests ===\n\n");

  test_platform_boundary();

  printf("\n%d/%d tests passed\n", total - failures, total);
  return failures > 0 ? 1 : 0;
}
