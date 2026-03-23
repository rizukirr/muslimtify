#ifdef _WIN32

#include "cmd_daemon_win.h"

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

static void test_hidden_task_action_builder(void) {
  char task_action[DAEMON_TASK_ACTION_MAX];
  int written = build_windows_task_action("C:\\Program Files\\Muslimtify\\muslimtify.exe",
                                          task_action, sizeof(task_action));

  report_result("builder returns success", written > 0);
  report_result("task action uses hidden powershell",
                strstr(task_action, "powershell.exe -NoProfile -WindowStyle Hidden") != NULL);
  report_result("task action invokes muslimtify check",
                strstr(task_action, "muslimtify.exe' check\\\"") != NULL);
  report_result("task action preserves spaced path",
                strstr(task_action, "'C:\\Program Files\\Muslimtify\\muslimtify.exe'") != NULL);
}

int main(void) {
  printf("test_hidden_task_action_builder\n");
  test_hidden_task_action_builder();

  if (failures == 0) {
    printf("All %d tests passed.\n", total);
    return 0;
  }

  printf("%d/%d tests failed.\n", failures, total);
  return 1;
}

#endif
