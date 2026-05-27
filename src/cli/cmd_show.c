#include "check_cycle.h"
#include "cli_internal.h"
#include "display.h"
#include <stdio.h>
#include <string.h>

int handle_show(int argc, char **argv) {
  PrayerSnapshot snap;
  if (cli_load_snapshot(&snap))
    return 1;

  bool json_format = false;
  bool no_header = false;

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--no-header") == 0) {
      no_header = true;
    } else if (i + 1 < argc && strcmp(argv[i], "--format") == 0 &&
               strcmp(argv[i + 1], "json") == 0) {
      json_format = true;
      i++;
    }
  }

  if (json_format) {
    display_prayer_times_json(&snap);
  } else if (no_header) {
    display_prayer_times_plain(&snap);
  } else {
    display_prayer_times_table(&snap);
  }

  return 0;
}

int handle_check(int argc, char **argv) {
  (void)argc;
  (void)argv;
  return run_check_cycle();
}
