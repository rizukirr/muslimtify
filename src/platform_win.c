#include "platform.h"
#include <direct.h>
#include <io.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static char config_dir_buf[PLATFORM_PATH_MAX] = {0};
static char cache_dir_buf[PLATFORM_PATH_MAX] = {0};
static char home_dir_buf[PLATFORM_PATH_MAX] = {0};
static char exe_dir_buf[PLATFORM_PATH_MAX] = {0};

static wchar_t *utf8_to_wide(const char *text) {
  int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
  if (len <= 0)
    return NULL;

  wchar_t *wide = (wchar_t *)malloc((size_t)len * sizeof(wchar_t));
  if (!wide)
    return NULL;

  if (MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, len) <= 0) {
    free(wide);
    return NULL;
  }

  return wide;
}

static bool wide_to_utf8(const wchar_t *wide, char *out, size_t out_size) {
  int len;

  if (!wide || !out || out_size == 0)
    return false;

  len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)out_size, NULL, NULL);
  if (len <= 0) {
    out[0] = '\0';
    return false;
  }

  return true;
}

static bool get_env_wide(const wchar_t *name, wchar_t *buffer, size_t buffer_size) {
  DWORD len = GetEnvironmentVariableW(name, buffer, (DWORD)buffer_size);
  return len > 0 && len < buffer_size;
}

static bool resolve_home_wide(wchar_t *buffer, size_t buffer_size) {
  wchar_t drive[PLATFORM_PATH_MAX];
  wchar_t path[PLATFORM_PATH_MAX];

  if (get_env_wide(L"USERPROFILE", buffer, buffer_size))
    return true;

  if (get_env_wide(L"HOMEDRIVE", drive, sizeof(drive) / sizeof(drive[0])) &&
      get_env_wide(L"HOMEPATH", path, sizeof(path) / sizeof(path[0]))) {
    int n = swprintf(buffer, buffer_size, L"%ls%ls", drive, path);
    return n > 0 && (size_t)n < buffer_size;
  }

  return false;
}

const char *platform_home_dir(void) {
  wchar_t wide_home[PLATFORM_PATH_MAX];

  if (home_dir_buf[0] != '\0')
    return home_dir_buf;

  if (resolve_home_wide(wide_home, sizeof(wide_home) / sizeof(wide_home[0]))) {
    if (!wide_to_utf8(wide_home, home_dir_buf, sizeof(home_dir_buf))) {
      home_dir_buf[0] = '\0';
    }
  }

  return home_dir_buf;
}

const char *platform_config_dir(void) {
  wchar_t wide_env[PLATFORM_PATH_MAX];
  wchar_t wide_dir[PLATFORM_PATH_MAX];
  wchar_t wide_home[PLATFORM_PATH_MAX];

  if (config_dir_buf[0] != '\0')
    return config_dir_buf;

  if (get_env_wide(L"APPDATA", wide_env, sizeof(wide_env) / sizeof(wide_env[0]))) {
    if (swprintf(wide_dir, sizeof(wide_dir) / sizeof(wide_dir[0]), L"%ls\\muslimtify",
                 wide_env) > 0) {
      if (!wide_to_utf8(wide_dir, config_dir_buf, sizeof(config_dir_buf))) {
        config_dir_buf[0] = '\0';
      }
    }
  } else if (resolve_home_wide(wide_home, sizeof(wide_home) / sizeof(wide_home[0]))) {
    if (swprintf(wide_dir, sizeof(wide_dir) / sizeof(wide_dir[0]),
                 L"%ls\\AppData\\Roaming\\muslimtify", wide_home) > 0) {
      if (!wide_to_utf8(wide_dir, config_dir_buf, sizeof(config_dir_buf))) {
        config_dir_buf[0] = '\0';
      }
    }
  }

  return config_dir_buf;
}

const char *platform_cache_dir(void) {
  wchar_t wide_env[PLATFORM_PATH_MAX];
  wchar_t wide_dir[PLATFORM_PATH_MAX];
  wchar_t wide_home[PLATFORM_PATH_MAX];

  if (cache_dir_buf[0] != '\0')
    return cache_dir_buf;

  if (get_env_wide(L"LOCALAPPDATA", wide_env, sizeof(wide_env) / sizeof(wide_env[0]))) {
    if (swprintf(wide_dir, sizeof(wide_dir) / sizeof(wide_dir[0]), L"%ls\\muslimtify",
                 wide_env) > 0) {
      if (!wide_to_utf8(wide_dir, cache_dir_buf, sizeof(cache_dir_buf))) {
        cache_dir_buf[0] = '\0';
      }
    }
  } else if (resolve_home_wide(wide_home, sizeof(wide_home) / sizeof(wide_home[0]))) {
    if (swprintf(wide_dir, sizeof(wide_dir) / sizeof(wide_dir[0]),
                 L"%ls\\AppData\\Local\\muslimtify", wide_home) > 0) {
      if (!wide_to_utf8(wide_dir, cache_dir_buf, sizeof(cache_dir_buf))) {
        cache_dir_buf[0] = '\0';
      }
    }
  }

  return cache_dir_buf;
}

const char *platform_exe_dir(void) {
  wchar_t wide_exe[PLATFORM_PATH_MAX];

  if (exe_dir_buf[0] != '\0')
    return exe_dir_buf;

  DWORD len = GetModuleFileNameW(NULL, wide_exe, PLATFORM_PATH_MAX);
  if (len > 0 && len < PLATFORM_PATH_MAX) {
    /* Strip filename to get directory */
    wchar_t *last_sep = wcsrchr(wide_exe, L'\\');
    if (!last_sep)
      last_sep = wcsrchr(wide_exe, L'/');
    if (last_sep) {
      *last_sep = '\0';
      if (!wide_to_utf8(wide_exe, exe_dir_buf, sizeof(exe_dir_buf))) {
        exe_dir_buf[0] = '\0';
      }
    }
  }

  return exe_dir_buf;
}

int platform_mkdir_p(const char *path) {
  wchar_t *wide_path = utf8_to_wide(path);
  if (!wide_path)
    return -1;

  for (wchar_t *p = wide_path + 1; *p; p++) {
    if (*p == L'\\' || *p == L'/') {
      wchar_t saved = *p;
      *p = L'\0';
      if (CreateDirectoryW(wide_path, NULL) == 0) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
          free(wide_path);
          return -1;
        }
      }
      *p = saved;
    }
  }
  if (CreateDirectoryW(wide_path, NULL) == 0) {
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
      free(wide_path);
      return -1;
    }
  }

  free(wide_path);
  return 0;
}

int platform_file_exists(const char *path) {
  wchar_t *wide_path = utf8_to_wide(path);
  if (!wide_path)
    return 0;

  DWORD attrs = GetFileAttributesW(wide_path);
  free(wide_path);
  return attrs != INVALID_FILE_ATTRIBUTES ? 1 : 0;
}

FILE *platform_file_open(const char *path, const char *mode) {
  wchar_t *wide_path = utf8_to_wide(path);
  wchar_t *wide_mode = utf8_to_wide(mode);
  if (!wide_path || !wide_mode) {
    free(wide_path);
    free(wide_mode);
    return NULL;
  }

  FILE *f = _wfopen(wide_path, wide_mode);
  free(wide_path);
  free(wide_mode);
  return f;
}

int platform_file_delete(const char *path) {
  wchar_t *wide_path = utf8_to_wide(path);
  if (!wide_path)
    return -1;

  int result = DeleteFileW(wide_path) != 0 ? 0 : -1;
  free(wide_path);
  return result;
}

int platform_atomic_rename(const char *src, const char *dst) {
  wchar_t *wide_src = utf8_to_wide(src);
  wchar_t *wide_dst = utf8_to_wide(dst);
  if (!wide_src || !wide_dst) {
    free(wide_src);
    free(wide_dst);
    return -1;
  }

  int result = MoveFileExW(wide_src, wide_dst, MOVEFILE_REPLACE_EXISTING) != 0 ? 0 : -1;
  free(wide_src);
  free(wide_dst);
  return result;
}

void platform_localtime(const time_t *t, struct tm *result) {
  localtime_s(result, t);
}

int platform_isatty(FILE *stream) {
  return _isatty(_fileno(stream));
}
