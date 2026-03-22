#include "../include/platform.h"
#include "cli_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static int run_schtasks(const char *args) {
  char cmd[PLATFORM_PATH_MAX * 2];
  snprintf(cmd, sizeof(cmd), "schtasks.exe %s", args);

  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    DWORD err = GetLastError();
    if (err == ERROR_ACCESS_DENIED) {
      fprintf(stderr,
              "Error: Access denied. Try running as administrator.\n");
    } else {
      fprintf(stderr, "Error: Failed to run schtasks (error %lu)\n", err);
    }
    return -1;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);

  DWORD exit_code;
  GetExitCodeProcess(pi.hProcess, &exit_code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return (int)exit_code;
}

int daemon_install_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  const char *exe = platform_exe_dir();
  char exe_path[PLATFORM_PATH_MAX];
  snprintf(exe_path, sizeof(exe_path), "%s\\muslimtify.exe", exe);

  if (!platform_file_exists(exe_path)) {
    fprintf(stderr, "Error: Cannot find muslimtify.exe at '%s'\n", exe_path);
    return 1;
  }

  char args[PLATFORM_PATH_MAX * 2];
  snprintf(args, sizeof(args),
           "/create /tn \"muslimtify\" /tr \"\\\"%s\\\" check\" /sc minute /mo 1 /f",
           exe_path);

  int result = run_schtasks(args);
  if (result == 0) {
    printf("Scheduled task 'muslimtify' created successfully.\n");
    printf("Prayer times will be checked every minute.\n");
  }
  return result;
}

int daemon_uninstall_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  int result = run_schtasks("/delete /tn \"muslimtify\" /f");
  if (result == 0) {
    printf("Scheduled task 'muslimtify' removed.\n");
  }
  return result;
}

int daemon_status_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  return run_schtasks("/query /tn \"muslimtify\"");
}
