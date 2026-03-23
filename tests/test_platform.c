#include "platform.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#endif

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

#ifndef _WIN32
static void test_linux_home_fallback(void) {
  printf("test_linux_home_fallback\n");

  struct passwd *pw = getpwuid(getuid());
  if (!pw || !pw->pw_dir || pw->pw_dir[0] == '\0') {
    printf("  SKIP: linux passwd home fallback unavailable\n");
    return;
  }

  char old_home[PLATFORM_PATH_MAX];
  char old_xdg_config[PLATFORM_PATH_MAX];
  char old_xdg_cache[PLATFORM_PATH_MAX];
  const char *home_env = getenv("HOME");
  const char *xdg_config_env = getenv("XDG_CONFIG_HOME");
  const char *xdg_cache_env = getenv("XDG_CACHE_HOME");
  bool had_home = home_env != NULL;
  bool had_xdg_config = xdg_config_env != NULL;
  bool had_xdg_cache = xdg_cache_env != NULL;

  if (had_home) {
    snprintf(old_home, sizeof(old_home), "%s", home_env);
  }
  if (had_xdg_config) {
    snprintf(old_xdg_config, sizeof(old_xdg_config), "%s", xdg_config_env);
  }
  if (had_xdg_cache) {
    snprintf(old_xdg_cache, sizeof(old_xdg_cache), "%s", xdg_cache_env);
  }

  setenv("HOME", "", 1);
  unsetenv("XDG_CONFIG_HOME");
  unsetenv("XDG_CACHE_HOME");

  const char *resolved_home = platform_home_dir();
  report_result("platform_home_dir() falls back when HOME is empty",
                resolved_home != NULL && strcmp(resolved_home, pw->pw_dir) == 0);

  if (had_xdg_cache) {
    setenv("XDG_CACHE_HOME", old_xdg_cache, 1);
  } else {
    unsetenv("XDG_CACHE_HOME");
  }
  if (had_xdg_config) {
    setenv("XDG_CONFIG_HOME", old_xdg_config, 1);
  } else {
    unsetenv("XDG_CONFIG_HOME");
  }
  if (had_home) {
    setenv("HOME", old_home, 1);
  } else {
    unsetenv("HOME");
  }
}
#endif

int main(void) {
  printf("=== platform boundary tests ===\n\n");

#ifndef _WIN32
  test_linux_home_fallback();
#endif
  test_platform_boundary();

  printf("\n%d/%d tests passed\n", total - failures, total);
  return failures > 0 ? 1 : 0;
}
