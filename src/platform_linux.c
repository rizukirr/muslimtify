#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "../include/platform.h"
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char config_dir_buf[PLATFORM_PATH_MAX] = {0};
static char cache_dir_buf[PLATFORM_PATH_MAX] = {0};
static char home_dir_buf[PLATFORM_PATH_MAX] = {0};
static char exe_dir_buf[PLATFORM_PATH_MAX] = {0};

const char *platform_home_dir(void) {
  if (home_dir_buf[0] != '\0')
    return home_dir_buf;

  const char *home = getenv("HOME");
  if (!home) {
    struct passwd *pw = getpwuid(getuid());
    if (pw)
      home = pw->pw_dir;
  }
  if (home)
    snprintf(home_dir_buf, sizeof(home_dir_buf), "%s", home);

  return home_dir_buf;
}

const char *platform_config_dir(void) {
  if (config_dir_buf[0] != '\0')
    return config_dir_buf;

  const char *xdg = getenv("XDG_CONFIG_HOME");
  if (xdg) {
    snprintf(config_dir_buf, sizeof(config_dir_buf), "%s/muslimtify", xdg);
  } else {
    const char *home = platform_home_dir();
    if (home[0] != '\0')
      snprintf(config_dir_buf, sizeof(config_dir_buf), "%s/.config/muslimtify", home);
  }

  return config_dir_buf;
}

const char *platform_cache_dir(void) {
  if (cache_dir_buf[0] != '\0')
    return cache_dir_buf;

  const char *xdg = getenv("XDG_CACHE_HOME");
  if (xdg) {
    snprintf(cache_dir_buf, sizeof(cache_dir_buf), "%s/muslimtify", xdg);
  } else {
    const char *home = platform_home_dir();
    if (home[0] != '\0')
      snprintf(cache_dir_buf, sizeof(cache_dir_buf), "%s/.cache/muslimtify", home);
  }

  return cache_dir_buf;
}

const char *platform_exe_dir(void) {
  if (exe_dir_buf[0] != '\0')
    return exe_dir_buf;

  char exe_path[PLATFORM_PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
  if (len > 0) {
    exe_path[len] = '\0';
    /* Strip filename to get directory */
    char *last_slash = strrchr(exe_path, '/');
    if (last_slash) {
      *last_slash = '\0';
      snprintf(exe_dir_buf, sizeof(exe_dir_buf), "%s", exe_path);
    }
  }

  return exe_dir_buf;
}

int platform_mkdir_p(const char *path) {
  char tmp[PLATFORM_PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s", path);

  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
        return -1;
      *p = '/';
    }
  }
  if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
    return -1;

  return 0;
}

int platform_file_exists(const char *path) {
  return access(path, F_OK) == 0 ? 1 : 0;
}

int platform_file_delete(const char *path) {
  return unlink(path) == 0 ? 0 : -1;
}

int platform_atomic_rename(const char *src, const char *dst) {
  return rename(src, dst) == 0 ? 0 : -1;
}

void platform_localtime(const time_t *t, struct tm *result) {
  localtime_r(t, result);
}

int platform_isatty(FILE *stream) {
  return isatty(fileno(stream));
}
