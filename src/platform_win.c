#include "platform.h"
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static char config_dir_buf[PLATFORM_PATH_MAX] = {0};
static char cache_dir_buf[PLATFORM_PATH_MAX] = {0};
static char home_dir_buf[PLATFORM_PATH_MAX] = {0};
static char exe_dir_buf[PLATFORM_PATH_MAX] = {0};

const char *platform_home_dir(void) {
  if (home_dir_buf[0] != '\0')
    return home_dir_buf;

  const char *home = getenv("USERPROFILE");
  if (home)
    snprintf(home_dir_buf, sizeof(home_dir_buf), "%s", home);

  return home_dir_buf;
}

const char *platform_config_dir(void) {
  if (config_dir_buf[0] != '\0')
    return config_dir_buf;

  const char *appdata = getenv("APPDATA");
  if (appdata) {
    snprintf(config_dir_buf, sizeof(config_dir_buf), "%s\\muslimtify", appdata);
  } else {
    const char *home = platform_home_dir();
    if (home[0] != '\0')
      snprintf(config_dir_buf, sizeof(config_dir_buf), "%s\\AppData\\Roaming\\muslimtify", home);
  }

  return config_dir_buf;
}

const char *platform_cache_dir(void) {
  if (cache_dir_buf[0] != '\0')
    return cache_dir_buf;

  const char *localappdata = getenv("LOCALAPPDATA");
  if (localappdata) {
    snprintf(cache_dir_buf, sizeof(cache_dir_buf), "%s\\muslimtify", localappdata);
  } else {
    const char *home = platform_home_dir();
    if (home[0] != '\0')
      snprintf(cache_dir_buf, sizeof(cache_dir_buf), "%s\\AppData\\Local\\muslimtify", home);
  }

  return cache_dir_buf;
}

const char *platform_exe_dir(void) {
  if (exe_dir_buf[0] != '\0')
    return exe_dir_buf;

  char exe_path[PLATFORM_PATH_MAX];
  DWORD len = GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
  if (len > 0 && len < sizeof(exe_path)) {
    /* Strip filename to get directory */
    char *last_sep = strrchr(exe_path, '\\');
    if (!last_sep)
      last_sep = strrchr(exe_path, '/');
    if (last_sep) {
      *last_sep = '\0';
      snprintf(exe_dir_buf, sizeof(exe_dir_buf), "%s", exe_path);
    }
  }

  return exe_dir_buf;
}

int platform_mkdir_p(const char *path) {
  char tmp[PLATFORM_PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s", path);

  for (char *p = tmp + 1; *p; p++) {
    if (*p == '\\' || *p == '/') {
      char saved = *p;
      *p = '\0';
      if (CreateDirectoryA(tmp, NULL) == 0) {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
          return -1;
      }
      *p = saved;
    }
  }
  if (CreateDirectoryA(tmp, NULL) == 0) {
    if (GetLastError() != ERROR_ALREADY_EXISTS)
      return -1;
  }

  return 0;
}

int platform_file_exists(const char *path) {
  DWORD attrs = GetFileAttributesA(path);
  return attrs != INVALID_FILE_ATTRIBUTES ? 1 : 0;
}

int platform_file_delete(const char *path) {
  return DeleteFileA(path) != 0 ? 0 : -1;
}

int platform_atomic_rename(const char *src, const char *dst) {
  return MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING) != 0 ? 0 : -1;
}

void platform_localtime(const time_t *t, struct tm *result) {
  localtime_s(result, t);
}

int platform_isatty(FILE *stream) {
  return _isatty(_fileno(stream));
}
