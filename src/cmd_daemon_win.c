#include "cli_internal.h"
#include "cmd_daemon_win.h"
#include "platform.h"
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
      fprintf(stderr, "Error: Access denied. Try running as administrator.\n");
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

static int append_text(char *buffer, size_t buffer_size, size_t *offset, const char *text) {
  int written;

  if (!buffer || !offset || !text)
    return -1;

  if (*offset >= buffer_size)
    return -1;

  written = snprintf(buffer + *offset, buffer_size - *offset, "%s", text);
  if (written < 0 || (size_t)written >= buffer_size - *offset)
    return -1;

  *offset += (size_t)written;
  return 0;
}

int build_windows_task_action(const char *exe_path, char *buffer, size_t buffer_size) {
  size_t offset = 0;

  if (!exe_path || !buffer || buffer_size == 0) {
    return -1;
  }

  buffer[0] = '\0';

  if (append_text(buffer, buffer_size, &offset,
                  "powershell.exe -NoProfile -WindowStyle Hidden -Command \\\"& '") != 0) {
    return -1;
  }

  for (const char *p = exe_path; *p; p++) {
    if (*p == '\'') {
      if (append_text(buffer, buffer_size, &offset, "''") != 0) {
        return -1;
      }
    } else {
      char ch[2] = {*p, '\0'};
      if (append_text(buffer, buffer_size, &offset, ch) != 0) {
        return -1;
      }
    }
  }

  if (append_text(buffer, buffer_size, &offset, "' check\\\"") != 0) {
    return -1;
  }

  return (int)offset;
}

static int daemon_install_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  const char *exe_path = platform_exe_path();
  if (!exe_path || exe_path[0] == '\0' || !platform_file_exists(exe_path)) {
    fprintf(stderr, "Error: Cannot find muslimtify.exe at '%s'\n",
            exe_path ? exe_path : "(unknown)");
    return 1;
  }

  char task_action[DAEMON_TASK_ACTION_MAX];
  if (build_windows_task_action(exe_path, task_action, sizeof(task_action)) < 0) {
    fprintf(stderr, "Error: Failed to build scheduled task action\n");
    return 1;
  }

  char args[PLATFORM_PATH_MAX * 3];
  snprintf(args, sizeof(args),
           "/create /tn \"muslimtify\" /tr \"%s\" /sc minute /mo 1 /f", task_action);

  int result = run_schtasks(args);
  if (result == 0) {
    printf("Scheduled task 'muslimtify' created successfully.\n");
    printf("Prayer times will be checked every minute.\n");
  }
  return result;
}

static int daemon_uninstall_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  int result = run_schtasks("/delete /tn \"muslimtify\" /f");
  if (result == 0) {
    printf("Scheduled task 'muslimtify' removed.\n");
  }
  return result;
}

static int daemon_status_handler(int argc, char **argv) {
  (void)argc;
  (void)argv;

  return run_schtasks("/query /tn \"muslimtify\"");
}

static const CommandEntry daemon_commands[] = {
    {"install", daemon_install_handler},
    {"uninstall", daemon_uninstall_handler},
    {"status", daemon_status_handler},
};

int handle_daemon(int argc, char **argv) {
  if (argc > 0) {
    const CommandEntry *sub =
        dispatch_lookup(daemon_commands, DISPATCH_N(daemon_commands), argv[0]);
    if (sub)
      return sub->handler(argc - 1, argv + 1);

    fprintf(stderr, "Usage: muslimtify daemon [install|uninstall|status]\n");
    return 1;
  }

  return daemon_status_handler(0, NULL);
}
